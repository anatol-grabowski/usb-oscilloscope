MCU    = atmega328p
F_CPU  = 12800000
OSCCAL_VALUE ?= 223
TARGET = main
FORMAT = ihex
OBJS   = usbdrv/usbdrv.o usbdrv/usbdrvasm.o usbdrv/oddebug.o

#====================== COMPILER ================================
CFLAGS  = -I. -I$(TOT_LIB)\avr
CFLAGS += -DF_CPU=$(F_CPU)UL
CFLAGS += -DOSCCAL_VALUE=$(OSCCAL_VALUE)
CFLAGS += -I.\usbdrv
#CFLAGS += -DDEBUG_LEVEL=0
CFLAGS += -Os
CFLAGS += -gdwarf-2
CFLAGS += -funsigned-char
CFLAGS += -funsigned-bitfields
CFLAGS += -fpack-struct
CFLAGS += -fshort-enums
CFLAGS += -Wall
#CFLAGS += -Wstrict-prototypes
CFLAGS += -mmcu=$(MCU)

#======================= LINKER =================================
LFLAGS  =
#LFLAGS += -lm -Wl,-u,vfprintf -Wl,-lprintf_flt

#====================== PROGRAMMER ==============================
PFLAGS =  -p $(MCU)
PFLAGS += -c usbasp
PFLAGS += -B 10
#PFLAGS += -D
PFLAGS += -q
#PFLAGS += -q

#======================= TOOLS ==================================
RM      = rm -f
CC      = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE    = avr-size
AVRDUDE = avrdude

MSG_CREATED = @echo "    "Created: "'"$@"'" from $(patsubst %,"'"%"'",$^).

#========================= TARGETS =====================================
build: $(TARGET).hex
	$(SIZE) --target=$(FORMAT) --format=berkley $(TARGET).hex

clean:
	$(RM) $(TARGET).hex $(TARGET).lss $(TARGET).elf $(TARGET).o
	$(RM) $(TARGET).i $(TARGET).s $(TARGET).lst $(TARGET).dasm
	$(RM) usbdrv/*.o usbdrv/oddebug.s usbdrv/usbdrv.s

flash: build
	$(AVRDUDE) $(PFLAGS) -U flash:w:$(TARGET).hex

program:
	flash

avrdude-terminal:
	$(AVRDUDE) $(PFLAGS) -t

.SECONDARY: 
.PRECIOUS:  %.c %.o %.elf %.hex %.lss %.i %.s
.PHONY:     build clean flash program avrdude-terminal

#-----------------------------------------------------------
%.hex: %.elf
	$(OBJCOPY) -O $(FORMAT) -R .eeprom -R .fuse -R .lock $< $@
	$(MSG_CREATED)

%.lss: %.elf
	$(OBJDUMP)      $< -S  > $@
	$(MSG_CREATED)

%.elf: $(OBJS) %.o
	$(CC) $^ \
$(CFLAGS) \
$(LFLAGS) \
-o $@
	$(MSG_CREATED)

%.o  : %.c
	$(CC) $< \
$(CFLAGS) \
-c -o $@
	$(MSG_CREATED)

%.o  : %.S
	$(CC) $< \
$(CFLAGS) \
-c -o $@
	$(MSG_CREATED)

%.s  : %.c
	$(CC) $(CFLAGS) $< -S -o $@
	$(MSG_CREATED)

%.i  : %.c
	$(CC) $(CFLAGS) $< -E -o $@
	$(MSG_CREATED)

%.lst: %.c
	$(CC) -c -Wa,-a,-ad $(CFLAGS) $< > $@
	$(MSG_CREATED)

%.dasm: %.hex
	$(OBJDUMP) -m avr $< -D > $@
	$(MSG_CREATED)

