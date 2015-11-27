/*
Efficient AVR DTMF Decoding
Copyright (c) 2015, Paul Stoffregen

I originally developed this 8 bit AVR-based DTMF decoding code in 2009 for a
special one-off project.  More recently, I created a superior implementation
for 32 bit ARM Cortex-M4 in the Teensy Audio Library.

http://www.pjrc.com/teensy/td_libs_Audio.html
https://github.com/PaulStoffregen/Audio/blob/master/examples/Analysis/DialTone_Serial/DialTone_Serial.ino

I highly recommend using the 32 bit version for new projects.  However, this
old 8 bit code may still be useful for some projects.  If you use this code,
I only ask that you preserve this info and links to the newer library.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice, development history, 32 bit audio library links,
and this permission notice shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


#include "main.h"

// Tone duration tuning.  Larger numbers reject more noise, but
// at the expense of requiring longer tone & silence duration.
// Units are number of 12.5 ms tone detection frames
#define MIN_TONE		3  // time the tone must be present to be recognized
#define MAX_TONE_DROPOUT	2  // allowable dropout while hearing tone
#define MIN_SILENCE		4  // silence required after tone

// Tone thresholds.  Larger numbers reject more noise, but
// these don't want to be too large because noise during only
// part of a 12.5 ms window can lower the amount of a valid
// tone.  The dropout parameter above is intended to handle
// noise or dropout causing small portions of a tone to go
// below the threshold.
#define MIN_ROW_VALUE		60  // detection threshold for row tone
#define MIN_COL_VALUE		60  // detection threshold for column tone



uint16_t print_raw_dtmf=0;

#define ADC_SAMPLE_RATE 8000

#define COEF_697	219
#define COEF_770	211
#define COEF_852	201
#define COEF_941	189
#define COEF_1209	149
#define COEF_1336	128
#define COEF_1477	102


static uint8_t adc_count=0;
static int8_t adc_buffer[GOERTZEL_N];
static volatile uint8_t detected_digit=0;
int8_t out_buffer[GOERTZEL_N];

static inline uint8_t dtmf_analysis(void);
static inline void dtmf_duration_check(uint8_t n);


void dtmf_init(void)
{
	cli();
	TCCR1B = 0;				// make sure timer1 is stopped
	TIFR1 = (1<<ICF1)|(1<<OCF1B)|(1<<OCF1A)|(1<<TOV1); // clear any pending interrupt
	//DIDR0 = 128;
	ADMUX = (1<<REFS0)|(1<<ADLAR)|7;	// 8 bit result, Vcc ref, channel 7
	ADCSRB = 0x07;				// trigger on timer1 icp interrupt flag
	ADCSRA = (1<<ADIF)|6;			// clear any previous interrupt
	ADCSRA = (1<<ADEN)|(1<<ADATE)|(1<<ADIE)|6; // enable adc, auto trigger, enable interrupt
	TCNT1H = 0;		// reset timer to 0
	TCNT1L = 0;
	// 16-bit write, the high byte must be written before the low byte.
	// 16-bit read, the low byte must be read before the high byte.
	ICR1H = ((F_CPU / ADC_SAMPLE_RATE - 1) >> 8);
	ICR1L = ((F_CPU / ADC_SAMPLE_RATE - 1) & 255);
	TCCR1A = 0;				// mode 12
	TCCR1B = (1<<WGM13)|(1<<WGM12)|(1<<CS10);  // CTC using ICR1 as TOP
	adc_count = 0;
	detected_digit = 0;
	sei();
}


void dtmf_disable(void)
{
	cli();
	ADCSRA = 0;
	TCCR1B = 0;
	detected_digit = 0;
	sei();
}


ISR(ADC_vect)
{
	uint8_t i;
	int8_t num;

	TIFR1 |= (1<<ICF1);
	num = ADCH - 0x80;
	if (num == -128) num = -127;
	adc_buffer[adc_count++] = num;
	if (adc_count == GOERTZEL_N) {
		for (i=0; i<GOERTZEL_N; i++) {
			out_buffer[i] = adc_buffer[i];
		}
		adc_count = 0;
		sei();
		dtmf_duration_check(dtmf_analysis());
		cli();
	}
}



static inline uint8_t dtmf_analysis(void)
{
	uint8_t r1=0, r2=0, r3=0, r4=0, c1, c2, c3;
	uint8_t out=0, noise_threshold;
	uint8_t row_signal, col_signal;
	//uint8_t row_noise, col_noise;

	c1 = goetzel_asm(COEF_1209);
	c2 = goetzel_asm(COEF_1336);
	c3 = goetzel_asm(COEF_1477);
	if (print_raw_dtmf) {
		print_raw_dtmf--;
		r1 = goetzel_asm(COEF_697);
		r2 = goetzel_asm(COEF_770);
		r3 = goetzel_asm(COEF_852);
		r4 = goetzel_asm(COEF_941);
		phex(r1);
		cout(' ');
		phex(r2);
		cout(' ');
		phex(r3);
		cout(' ');
		phex(r4);
		cout(' ');
		cout(' ');
		phex(c1);
		cout(' ');
		phex(c2);
		cout(' ');
		phex(c3);
		cout('\r');
		cout('\n');
	}
	if (c1 > c2) {
		if (c1 > c3) goto col1;
		else goto col3;
	} else {
		if (c2 > c3) goto col2;
		else goto col3;
	}
col1:
	if (c1 < MIN_COL_VALUE) return 0;
	noise_threshold = c1 / 3;
	col_signal = c1;
	if (c2 > noise_threshold) return 0;
	if (c3 > noise_threshold) return 0;
	//col_noise = c2 + c3;
	goto row;
col2:
	if (c2 < MIN_COL_VALUE) return 0;
	noise_threshold = c2 / 3;
	col_signal = c2;
	if (c1 > noise_threshold) return 0;
	if (c3 > noise_threshold) return 0;
	//col_noise = c1 + c3;
	out = 1;
	goto row;
col3:
	if (c3 < MIN_COL_VALUE) return 0;
	noise_threshold = c3 / 3;
	col_signal = c3;
	if (c1 > noise_threshold) return 0;
	if (c2 > noise_threshold) return 0;
	//col_noise = c1 + c2;
	out = 2;
row:
	if (!print_raw_dtmf) {
		r1 = goetzel_asm(COEF_697);
		r2 = goetzel_asm(COEF_770);
		r3 = goetzel_asm(COEF_852);
		r4 = goetzel_asm(COEF_941);
	}
	if (r1 > r2) {
		if (r1 > r3) {
			if (r1 > r4) goto row1;
			else goto row4;
		} else {
			if (r3 > r4) goto row3;
			else goto row4;
		}
	} else {
		if (r2 > r3) {
			if (r2 > r4) goto row2;
			else goto row4;
		} else {
			if (r3 > r4) goto row3;
			else goto row4;
		}
	}
row1:
	if (r1 < MIN_ROW_VALUE) return 0;
	noise_threshold = r1 / 3;
	row_signal = r1;
	if (r2 > noise_threshold) return 0;
	if (r3 > noise_threshold) return 0;
	if (r4 > noise_threshold) return 0;
	//row_noise = r2 + r3 + r4;
	out += 1;
	goto relative;
row2:
	if (r2 < MIN_ROW_VALUE) return 0;
	noise_threshold = r2 / 3;
	row_signal = r2;
	if (r1 > noise_threshold) return 0;
	if (r3 > noise_threshold) return 0;
	if (r4 > noise_threshold) return 0;
	//row_noise = r1 + r3 + r4;
	out += 4;
	goto relative;
row3:
	if (r3 < MIN_ROW_VALUE) return 0;
	noise_threshold = r3 / 3;
	row_signal = r3;
	if (r1 > noise_threshold) return 0;
	if (r2 > noise_threshold) return 0;
	if (r4 > noise_threshold) return 0;
	//row_noise = r1 + r2 + r4;
	out += 7;
	goto relative;
row4:
	if (r4 < MIN_ROW_VALUE) return 0;
	noise_threshold = r4 / 3;
	row_signal = r4;
	if (r1 > noise_threshold) return 0;
	if (r2 > noise_threshold) return 0;
	if (r3 > noise_threshold) return 0;
	//row_noise = r1 + r2 + r3;
	out += 10;
relative:
	if (col_signal > row_signal) {
		if (((uint16_t)col_signal * 50) > ((uint16_t)row_signal << 8)) return 0;
	} else {
		if (((uint16_t)row_signal * 38) > ((uint16_t)col_signal << 8)) return 0;
	}
	return out;
}



static inline void dtmf_duration_check(uint8_t n)
{
	static uint8_t digit=0;
	static uint8_t digit_count=0;
	static uint8_t silent_count=0;

	if (n) {
		// we are hearing a tone right now
		if (n == digit && silent_count <= MAX_TONE_DROPOUT) {
			// if it's the same tone
			if (digit_count < 255) digit_count++;
		} else {
			// otherwise, begin new tone
			digit = n;
			digit_count = 1;
		}
		silent_count = 0;
	} else {
		// we are hearing silence right now
		if (silent_count < 255) silent_count++;
		if (digit) {
			// if we previously heard a tone
			if (silent_count >= MIN_SILENCE) {
				if (digit_count >= MIN_TONE) {
					// valid silence following valid tone
					detected_digit = digit;
					digit = 0;
					digit_count = 0;
				}
				// valid silence following surrious tone
				digit = 0;
				digit_count = 0;
			}
		}
	}
}

const char PROGMEM code_to_digit[] = {
	0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '*', '0', '#'
};

char dtmf_digit(void)
{
	uint8_t n;

	cli();
	n = detected_digit;
	detected_digit = 0;
	sei();
	return pgm_read_byte(code_to_digit + n);
}

void dtmf_enable_debug(uint16_t num)
{
	print_raw_dtmf = num;
}

