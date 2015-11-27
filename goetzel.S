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

.global out_buffer


#define r_coef	r18
#define r_zero	r19
#define q1_lsb  r20
#define q1_msb  r21
#define q2_lsb  r22
#define q2_msb  r23
#define q0_lsb  r24
#define q0_msb  r25
#define out0	r24
#define out1	r25
#define out2	r26
#define out3	r27


; uint8_t goetzel_asm(uint8_t coef)
.global goetzel_asm
goetzel_asm:
	;coef in r24
	;clobber registers: r0,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r30,r31
	ldi	r30, lo8(out_buffer)
	ldi	r31, hi8(out_buffer)
	mov	r_coef, r24
	clr	r_zero
	clr	q1_lsb			;q1 = 0
	clr	q1_msb
	movw	q2_lsb, q1_lsb		;q2 = 0
goetzel_loop:
	mulsu	q1_msb, r_coef
	movw	q0_lsb, r0
	mul	q1_lsb, r_coef
	add	q0_lsb, r1		;q0+r0 = q1 * coef
	adc	q0_msb, r_zero
	rol	r0
	rol	q0_lsb			;q0 = q1 * coef / 128
	rol	q0_msb
	sub	q0_lsb, q2_lsb		;q0 -= q2
	sbc	q0_msb, q2_msb
	ld	r0, Z+			;fetch out_buffer[i]
	clr	r1
	sbrc	r0, 7			;sign extend to 16 bits
	com	r1
	add	q0_lsb, r0		;q0 += (int16_t)out_buffer[i]
	adc	q0_msb, r1
	movw	q2_lsb, q1_lsb		;q2 = q1
	movw	q1_lsb, q0_lsb		;q1 = q0
	cpi	r30, lo8(out_buffer + GOERTZEL_N)
	brne	goetzel_loop
	muls	q1_msb, q2_msb
	movw	out2, r0
	mul	q1_lsb, q2_lsb
	movw	out0, r0
	mulsu	q1_msb, q2_lsb
	sbc	out3, r_zero
	add	out1, r0
	adc	out2, r1
	adc	out3, r_zero
	mulsu	q2_msb, q1_lsb
	sbc	out3, r_zero
	add	out1, r0		;out = q1 * q2
	adc	out2, r1
	adc	out3, r_zero
	rol	out0
	rol	out1
	rol	out2
	rol	out3			;out /= 128
	movw	r30, out2
	mul	out1, r_coef		;out *= coef
	movw	out0, r0
	mov	r_zero, r31
	mulsu	r_zero, r_coef
	clr	r_zero
	movw	out2, r0
	mul	r30, r_coef
	add	out1, r0
	adc	out2, r1
	adc	out3, r_zero
	com	out0
	com	out1
	com	out2
	com	out3
	subi	out0, 0xFF
	sbci	out1, 0xFF
	sbci	out2, 0xFF
	sbci	out3, 0xFF
	muls	q1_msb, q1_msb
	add	out2, r0
	adc	out3, r1
	mulsu	q1_msb, q1_lsb
	sbc	out3, r_zero
	add	out1, r0
	adc	out2, r1
	adc	out3, r_zero
	mulsu	q1_msb, q1_lsb
	sbc	out3, r_zero
	add	out1, r0
	adc	out2, r1
	adc	out3, r_zero
	mul	q1_lsb, q1_lsb
	add	out0, r0
	adc	out1, r1
	adc	out2, r_zero
	adc	out3, r_zero
	muls	q2_msb, q2_msb
	add	out2, r0
	adc	out3, r1
	mulsu	q2_msb, q2_lsb
	sbc	out3, r_zero
	add	out1, r0
	adc	out2, r1
	adc	out3, r_zero
	mulsu	q2_msb, q2_lsb
	sbc	out3, r_zero
	add	out1, r0
	adc	out2, r1
	adc	out3, r_zero
	mul	q2_lsb, q2_lsb
	add	out0, r0
	adc	out1, r1
	adc	out2, r_zero
	adc	out3, r_zero
	rol	out1
	rol	out2
	rol	out3
	clr	r24
	brcs	goetzel_end
	ldi	r24, 255
	tst	out3
	brne	goetzel_end
	mov	r24, out2
goetzel_end:
	clr	r1
	ret
