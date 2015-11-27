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


void uart_init(void)
{
        UBRR0L = 11;     // 115.2 kbps @ 11.0592 MHz clock
        UBRR0H = 0;
        UCSR0A = (1<<U2X0);
        UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
        UCSR0B = (1<<TXEN0)|(1<<RXEN0);
}

void cout(char c)
{
        while (!(UCSR0A & (1<<UDRE0)));
        UCSR0A |= (1<<TXC0);
        UDR0 = c;
}

char cin(void)
{
	if (UCSR0A & (1<<RXC0)) return UDR0;
	return 0;
}

void print_P(const char *s)
{
	char c;

        while (1) {
                c = pgm_read_byte(s++);
                if (!c) break;
                if (c == '\n') cout('\r');
                cout(c);
        }
}

void print_P_hex(const char *s, uint8_t n)
{
	print_P(s);
	phex(n);
	cout('\r');
	cout('\n');
}

void print_P_hex16(const char *s, uint16_t n)
{
	print_P(s);
	phex16(n);
	cout('\r');
	cout('\n');
}

void print_P_hex32(const char *s, uint32_t n)
{
	print_P(s);
	phex32(n);
	cout('\r');
	cout('\n');
}

void phex1(unsigned char c)
{
        cout (c + ((c < 10) ? '0' : 'A' - 10));
}

void phex(unsigned char c)
{
        phex1(c >> 4);
        phex1(c & 15);
}

void phex16(uint16_t n)
{
	phex(n >> 8);
	phex(n & 255);
}

void phex32(uint32_t n)
{
	phex16(n >> 16);
	phex16(n & 0xFFFF);
}

