CC = avr-gcc
EXEC = firmware.bin
CFLAGS = -Wall -Wl,-Map,usio.map -DF_CPU=8000000 -fpack-struct  -I/usr/lib/avr/include \
 -mmcu=atmega128 -Os -fpack-struct -fshort-enums -std=gnu99 \
 -funsigned-char -funsigned-bitfields 

OBJ = board.o gpio.o gpio_keys.o gpio_debouncer.o \
 idle.o leds.o list.o sys_timer.o uart_atmega128.o \
 eeprom_fs.o nmea0183.o module_io.o module_wdt.o

.PHONY: all clean program fuse

all: $(EXEC)

$(EXEC): $(OBJ) 
	$(CC) $(CFLAGS) $(OBJ) -o $(EXEC)

$(OBJ): config.h types.h gpio.h gpio_debouncer.h \
 gpio_keys.h uart.h list.h sys_timer.h idle.h board.h \
 types.h eeprom_fs.h nmea0183.h module_io.h module_wdt.h

clean:
	rm -rf *.o $(EXEC)

program: $(EXEC)
	sudo avrdude -p m128 -c usbasp -e -U flash:w:$(EXEC)

fuse:
	sudo avrdude -p m128 -c usbasp -U lfuse:w:0x8a:m -U hfuse:w:0xd9:m -U efuse:w:0xff:m 

info: $(EXEC)
	avr-size --mcu=atmega128 --format=avr $(EXEC)
