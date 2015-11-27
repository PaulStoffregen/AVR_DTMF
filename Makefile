
all: main.hex

CC = avr-gcc
CFLAGS = -Os -Wall -g3 -mmcu=atmega168 -DF_CPU=11059200UL -fpack-struct
OBJS = dtmf.o goetzel.o uart.o main.o

main.hex: $(OBJS)
	$(CC) $(CFLAGS) -o main.elf $(OBJS) 
	avr-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature main.elf main.hex

goetzel.o: goetzel.S
	$(CC) $(CFLAGS) -c -o goetzel.o goetzel.S

clean:
	rm -f *.lst *.hex *.o *.obj *.elf *.bin
