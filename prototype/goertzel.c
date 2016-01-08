#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <math.h>

#define PORT "/dev/ttyMI1"



void die(const char *format, ...)
{
	va_list arg;

	va_start(arg, format);
	vfprintf(stderr, format, arg);
	exit(1);
}


//#define SAMPLING_RATE       8000
#define SAMPLING_RATE       (4629.63 * 2.0)
#define GOERTZEL_N          100

#define MAX_BINS            8

//#define SAMPLING_RATE       6000
//#define GOERTZEL_N          75

int         sample_count;
double      q1[MAX_BINS];
double      q2[MAX_BINS];
double      r[MAX_BINS];
double      freqs[MAX_BINS] = {697, 770, 852, 941, 1209, 1336, 1477, 1633};
double      coefs[MAX_BINS] ;

uint16_t coef16[MAX_BINS];


void calc_coeffs()
{
	int n;
 
	for(n = 0; n < MAX_BINS; n++) {
		coefs[n] = 2.0 * cos(2.0 * 3.141592654 * freqs[n] / SAMPLING_RATE);
		//coef16[n] = rint(coefs[n] * 16384);
		coef16[n] = rint(coefs[n] * 128);
		printf("coef %d is %.5f, int = %u\n", n, coefs[n], coef16[n]);
	}
}

// http://en.wikipedia.org/wiki/Goertzel_algorithm
void post_testing()
{
	int         row, col, see_digit=0;
	int         peak_count, max_index;
	double      maxval, t;
	int         i;
	char *  row_col_ascii_codes[4][4] = {
		{"1", "2", "3", "A"},
		{"4", "5", "6", "B"},
		{"7", "8", "9", "C"},
		{"*", "0", "#", "D"}};
 
 
	/* Find the largest in the row group. */
	row = 0;
	maxval = 0.0;
	for ( i=0; i<4; i++ ) {
		if ( r[i] > maxval ) {
			maxval = r[i];
			row = i;
		}
	}
 
	/* Find the largest in the column group. */
	col = 4;
	maxval = 0.0;
	for ( i=4; i<8; i++ ) {
		if ( r[i] > maxval ) {
			maxval = r[i];
			col = i;
		}
	}
	/* Check for minimum energy */

	printf("   ");
	for (i=0; i<8; i++) {
		printf("%8.0f ", r[i]);
	}
	printf("\n");
 
	//printf("col:%d=%.2f, row=%d=%.2f\n", col, r[col], row, r[row]);

	if ( r[row] < 4.0e5 ) {
		/* 2.0e5 ... 1.0e8 no change */
		/* energy not high enough */
	} else if ( r[col] < 4.0e5 ) {
		/* energy not high enough */
	} else {
		see_digit = 1;
 
		/* Twist check
		* CEPT => twist < 6dB
		* AT&T => forward twist < 4dB and reverse twist < 8dB
		*  -ndB < 10 log10( v1 / v2 ), where v1 < v2
		*  -4dB < 10 log10( v1 / v2 )
		*  -0.4  < log10( v1 / v2 )
		*  0.398 < v1 / v2
		*  0.398 * v2 < v1
		*/
		if ( r[col] > r[row] ) {
			/* Normal twist */
			max_index = col;
			if ( r[row] < (r[col] * 0.398) )    /* twist > 4dB, error */
				see_digit = 0;
		} else {
			/* if ( r[row] > r[col] ) */
			/* Reverse twist */
			max_index = row;
			if ( r[col] < (r[row] * 0.158) )    /* twist > 8db, error */
				see_digit = 0;
		}
 
		/* Signal to noise test
		* AT&T states that the noise must be 16dB down from the signal.
		* Here we count the number of signals above the threshold and
		* there ought to be only two.
		*/

		t = r[max_index] * 0.158;
		peak_count = 0;
		for ( i=0; i<8; i++ ) {
			if ( r[i] > t )
			peak_count++;
		}
		if ( peak_count > 2 )
			see_digit = 0;
 
		if ( see_digit ) {
			printf( "%s", row_col_ascii_codes[row][col-4] );
			fflush(stdout);
		}
	}
}
 


void goertzel(int sample)
{
	double q0;
	int i;
 
	sample_count++;
	/*q1[0] = q2[0] = 0;*/
	for ( i=0; i<MAX_BINS; i++ ) {
		q0 = coefs[i] * q1[i] - q2[i] + sample;
		q2[i] = q1[i];
		q1[i] = q0;
	}
	if (sample_count == GOERTZEL_N) {
		for ( i=0; i<MAX_BINS; i++ ) {
			r[i] = (q1[i] * q1[i]) + (q2[i] * q2[i]) - (coefs[i] * q1[i] * q2[i]);
			printf("%.0f %.0f %.0f   ", q1[i], q2[i], r[i]);
			q1[i] = 0.0;
			q2[i] = 0.0;
		}
		printf("\n");
		//post_testing();
		sample_count = 0;
	}
}


void my_goertzel(int8_t *buf)
{
	int n, i;
	int min=1000, max=-1000;
	uint8_t s[7];
	int16_t q0, q1, q2;

	for (n=0; n<7; n++) {
		q0 = q1 = q2 = 0;
		for (i=0; i<GOERTZEL_N; i++) {
			q0 = buf[i] + (q1 * coef16[n] / 128) - q2;
			q2 = q1;
			q1 = q0;
			//printf("      in=%d, q0=%d, q1=%d\n", buf[i], q0, q1);
		}
		q0 = ((q1 * q1) + (q2 * q2) - (q1 * q2 / 128 * coef16[n])) / 32768;
		if (q0 < 0) q0 = 0;
		s[n] = q0 < 255 ? q0 : 255;
		//printf("%7d %7d %7d  .  ", q1, q2, s[n]);
		printf(" %4d ", s[n]);
	}
	min = 1000;
	max = -1000;
	for (i=0; i<GOERTZEL_N; i++) {
		if (buf[i] > max) max = buf[i];
		if (buf[i] < min) min = buf[i];
	}
	printf("    %4d to %3d\n", min, max);
}




void rx(const uint8_t *data, int num)
{
	static int8_t c, buf[GOERTZEL_N];
	static int count=0;
	int i;

	for (i=0; i<num; i++) {
		c = (int)(data[i]) - 0x80;
		if (c == -128) c = -127;
		//goertzel(c);
		buf[count++] = c;
		if (count >= GOERTZEL_N) {
			my_goertzel(buf);
			count = 0;
		}
	}
}


int main()
{
	int fd, num, errcount=0;
	uint8_t buf[512];
	struct termios t;
	fd_set rdfs;

	fd = open(PORT, O_RDWR | O_NONBLOCK);
	if (fd < 0) die("unable to open %s\n", PORT);
	tcgetattr(fd, &t);
	t.c_iflag = IGNBRK | IGNPAR | IXANY;
	t.c_cflag = B115200 | CS8 | CREAD | HUPCL | CLOCAL;
	t.c_oflag = 0;
	t.c_lflag = 0;
	tcsetattr(fd, TCSANOW, &t);
	calc_coeffs();

	while (1) {
		num = read(fd, buf, sizeof(buf));
		//printf("num = %d\n", num);
		if (num < 0 && errno != EAGAIN) {
			if (++errcount > 20) die("error reading\n");
			continue;
		}
		errcount=0;
		if (num > 0) {
			rx(buf, num);
		} else {
			FD_ZERO(&rdfs);
			FD_SET(fd, &rdfs);
			num = select(fd+1, &rdfs, NULL, NULL, NULL);
			if (num < 0) {
				if (++errcount > 20) die("error waiting\n");
				continue;
			}
		}
	}
	return 0;
}

