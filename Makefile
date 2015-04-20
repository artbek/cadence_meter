DEVICE  = attiny13a
DEVICE_DUDE  = t13
F_CPU   = 128000	# Hz

FUSE_L  = 0x7B
FUSE_H  = 0XFF

AVRDUDE = avrdude -c dragon_isp -B 50 -p $(DEVICE_DUDE) -e
OBJECTS = main.o
CFLAGS  = -I.
COMPILE = avr-gcc -g -Wall -Os -DF_CPU=$(F_CPU) $(CFLAGS) -mmcu=$(DEVICE)


help:
	@echo "This Makefile has no default rule. Use one of the following:"
	@echo "make clean ..... to delete objects and hex file"
	@echo "make flash ..... to flash the firmware"
	@echo "make fuse ...... to flash the fuses"
	@echo "make hex ....... to build main.hex"


clean:
	rm -f \
		main.hex \
		main.elf \
		*.o


all: hex flash


flash: main.hex
	$(AVRDUDE) -U flash:w:main.hex:i


fuse:
	@[ "$(FUSE_H)" != "" -a "$(FUSE_L)" != "" ] || \
		{ echo "*** Edit Makefile and choose values for FUSE_L and FUSE_H!"; exit 1; }
	$(AVRDUDE) -U hfuse:w:$(FUSE_H):m -U lfuse:w:$(FUSE_L):m


hex: main.hex


main.hex: main.elf
	rm -f main.hex main.eep.hex
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex
	avr-size main.hex
	avr-size --mcu=$(DEVICE) -C main.elf

# The $(OBJECT) dependency is necessary to automagically compile
# the object files (*.o) using generic rules defined below.
main.elf: $(OBJECTS)
	$(COMPILE) -o main.elf $(OBJECTS) -nostartfiles /usr/lib/avr/lib/avr25/crttn13a.o


# Generic rule for compiling C files:
.c.o:
	echo $< $@
	$(COMPILE) -c $< -o $@

