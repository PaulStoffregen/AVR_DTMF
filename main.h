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


#ifndef main_h_included__
#define main_h_included__

#define GOERTZEL_N      100   /* 12.5 ms with 8 kHz sample rate */

#ifndef __ASSEMBLER__

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

// dtmf.c
void dtmf_init(void);
void dtmf_disable(void);
char dtmf_digit(void);
void dtmf_enable_debug(uint16_t num);

// uart.c
void cout(char c);
char cin(void);
void phex(unsigned char c);
void phex1(unsigned char c);
void phex16(uint16_t n);
void phex32(uint32_t n);
#define print(s) print_P(PSTR(s))
#define print_hex(s, n) print_P_hex(PSTR(s), (n))
#define print_hex16(s, n) print_P_hex16(PSTR(s), (n))
#define print_hex32(s, n) print_P_hex32(PSTR(s), (n))
void print_P(const char *s);
void print_P_hex(const char *s, uint8_t n);
void print_P_hex16(const char *s, uint16_t n);
void print_P_hex32(const char *s, uint32_t n);
void uart_init(void);

// goetzel.S
uint8_t goetzel_asm(uint8_t coef);

//register uint8_t isr_status asm("r2");

#else // __ASSEMBLER__

//#define isr_status	r2

#endif
#endif
