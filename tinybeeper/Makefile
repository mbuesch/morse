# The architecture configuration
ARCH		= attiny13
AVRDUDE_ARCH	= t13
AVRDUDE_SPEED	= 1
PROGRAMMER	= avrisp2
PROGPORT	= usb
TOPRAMMER_ID	= attiny13dip8

# The toolchain definitions
CC		= avr-gcc
OBJCOPY		= avr-objcopy
SIZE		= avr-size
READELF		= avr-readelf
SPARSE		= sparse
TOPRAMMER	= toprammer
AVRDUDE		= avrdude

DEBUG		= 0		# Debug build:  make DEBUG=1

V		= @		# Verbose build:  make V=1
C		= 0		# Sparsechecker build:  make C=1
Q		= $(V:1=)
QUIET_CC	= $(Q:@=@echo '     CC       '$@;)$(CC)
QUIET_DEPEND	= $(Q:@=@echo '     DEPEND   '$@;)$(CC)
QUIET_OBJCOPY	= $(Q:@=@echo '     OBJCOPY  '$@;)$(OBJCOPY)
QUIET_SIZE	= $(Q:@=@echo '     SIZE     '$@;)$(SIZE)
QUIET_READELF	= $(Q:@=@echo '     READELF  '$@;)$(READELF)
ifeq ($(C),1)
QUIET_SPARSE	= $(Q:@=@echo '     SPARSE   '$@;)$(SPARSE)
else
QUIET_SPARSE	= @/bin/true
endif

CFLAGS		= -mmcu=$(ARCH) -std=gnu99 -g0 -Os -Wall -DDEBUG=$(DEBUG) \
		  "-Dinline=inline __attribute__((__always_inline__))"

SPARSEFLAGS	= $(CFLAGS) -I "/usr/lib/avr/include" -D__AVR_ARCH__=4 \
		  -D__AVR_ATmega8__=1 -D__ATTR_PROGMEM__="" -Dsignal=dllexport \
		  -Dexternally_visible=dllexport


# The fuse bits
LFUSE	= F1
HFUSE	= FF

F_CPU	= 4800000

SRCS	= main.c
NAME	= tinybeeper
ELF	= $(NAME).elf
BIN	= $(NAME).bin
HEX	= $(NAME).hex
EEP	= $(NAME).eep.hex

CFLAGS	+= -DF_CPU=$(F_CPU)

.SUFFIXES:
.PHONY: all avrdude install_flash install_eeprom install reset writefuse clean distclean
.DEFAULT_GOAL := all

DEPS = $(sort $(patsubst %.c,dep/%.d,$(1)))
OBJS = $(sort $(patsubst %.c,obj/%.o,$(1)))

# Generate dependencies
$(call DEPS,$(SRCS)): dep/%.d: %.c 
	@mkdir -p $(dir $@)
	$(QUIET_DEPEND) -o $@.tmp -MM -MG -MT "$@ $(patsubst dep/%.d,obj/%.o,$@)" $(CFLAGS) $< && mv -f $@.tmp $@

-include $(call DEPS,$(SRCS))

# Generate object files
$(call OBJS,$(SRCS)): obj/%.o:
	@mkdir -p $(dir $@)
	$(QUIET_SPARSE) $(SPARSEFLAGS) $<
	$(QUIET_CC) -o $@ -c $(CFLAGS) $<

all: $(HEX)

%.s: %.c
	$(QUIET_CC) $(CFLAGS) -S $*.c

$(ELF): $(call OBJS,$(SRCS))
	$(QUIET_CC) $(CFLAGS) -o $(ELF) $(call OBJS,$(SRCS)) $(LDFLAGS)

$(HEX): $(ELF)
	$(QUIET_OBJCOPY) -R.eeprom -O ihex $(ELF) $(HEX)
	$(QUIET_OBJCOPY) -R.eeprom -O binary $(ELF) $(BIN)
	$(QUIET_OBJCOPY) -j.eeprom --set-section-flags=.eeprom="alloc,load" \
			 --change-section-lma .eeprom=0 -O ihex $(ELF) $(EEP)
	$(QUIET_SIZE) $(ELF)
	$(QUIET_READELF) -S $(ELF) | egrep '(Name|text|eeprom|data|bss)'

avrdude:
	$(AVRDUDE) -B $(AVRDUDE_SPEED) -p $(AVRDUDE_ARCH) \
	 -c $(PROGRAMMER) -P $(PROGPORT) -t

install_flash:
	$(AVRDUDE) -B $(AVRDUDE_SPEED) -p $(AVRDUDE_ARCH) \
	 -c $(PROGRAMMER) -P $(PROGPORT) -U flash:w:$(HEX)

install_eeprom:
	$(AVRDUDE) -B $(AVRDUDE_SPEED) -p $(AVRDUDE_ARCH) \
	 -c $(PROGRAMMER) -P $(PROGPORT) -U eeprom:w:$(EEP)

install: all install_flash install_eeprom

toprammer_install: all
	$(TOPRAMMER) -c $(TOPRAMMER_ID) -x
	echo "0x0000: $(LFUSE)$(HFUSE)" | $(TOPRAMMER) -c $(TOPRAMMER_ID) -I hex -F -
	$(TOPRAMMER) -c $(TOPRAMMER_ID) -I ihex -P $(HEX)

# Reset the microcontroller through avrdude
reset:
	$(AVRDUDE) -B $(AVRDUDE_SPEED) -p $(AVRDUDE_ARCH) \
	 -c $(PROGRAMMER) -P $(PROGPORT) \
	 -U signature:r:/dev/null:i -q -q

writefuse:
	$(AVRDUDE) -B $(AVRDUDE_SPEED) -p $(AVRDUDE_ARCH) \
	 -c $(PROGRAMMER) -P $(PROGPORT) -q -q \
	 -U lfuse:w:0x$(LFUSE):m \
	 -U hfuse:w:0x$(HFUSE):m

clean:
	-rm -Rf dep obj *~ *.pyc $(ELF)

distclean: clean
	-rm -f *.s $(HEX) $(BIN) $(EEP)
