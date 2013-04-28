# The architecture configuration
ARCH			:= atmega8
AVRDUDE_ARCH		:= m8
AVRDUDE_SPEED		:= 1
AVRDUDE_SLOW_SPEED	:= 100
PROGRAMMER		:= avr910
#PROGRAMMER		:= avrisp2
PROGPORT		:= /dev/ttyUSB0
#PROGPORT		:= usb

# The fuse bits
LFUSE		:= 0xFB
HFUSE		:= 0xD9

# Hardware parameters
F_CPU		:= 3686400

BINEXT		:=
NODEPS		:=

# The toolchain definitions
CC		:= avr-gcc$(BINEXT)
OBJCOPY		:= avr-objcopy$(BINEXT)
SIZE		:= avr-size$(BINEXT)
MKDIR		:= mkdir$(BINEXT)
MV		:= mv$(BINEXT)
RM		:= rm$(BINEXT)
CP		:= cp$(BINEXT)
ECHO		:= echo$(BINEXT)
AVRDUDE		:= avrdude$(BINEXT)
MYSMARTUSB	:= mysmartusb.py
DOXYGEN		:= doxygen$(BINEXT)

V		:= @		# Verbose build:  make V=1
O		:= s		# Optimize flag
Q		:= $(V:1=)
QUIET_CC	= $(Q:@=@$(ECHO) '     CC       '$@;)$(CC)
QUIET_DEPEND	= $(Q:@=@$(ECHO) '     DEPEND   '$@;)$(CC)
QUIET_OBJCOPY	= $(Q:@=@$(ECHO) '     OBJCOPY  '$@;)$(OBJCOPY)
QUIET_SIZE	= $(Q:@=@$(ECHO) '     SIZE     '$@;)$(SIZE)

CFLAGS		:= -mmcu=$(ARCH) -std=c99 -g -O$(O) -Wall \
		  "-Dinline=inline __attribute__((__always_inline__))" \
		  -fshort-enums \
		  -DF_CPU=$(F_CPU)
LDFLAGS		:=

# Application code
SRCS		:= main.c lcd.c morse.c buzzer.c
NAME		:= morsedec
BIN		:= $(NAME).bin
HEX		:= $(NAME).hex
EEP		:= $(NAME).eep.hex

.SUFFIXES:
.PHONY: all avrdude install_flash install_eeprom install reset writefuse doxygen clean distclean
.DEFAULT_GOAL := all

ifeq ($(BINEXT),.exe)
HAVE_MYSMARTUSB	:=
else
HAVE_MYSMARTUSB	:= $(shell $(MYSMARTUSB) -h >/dev/null 2>&1 && $(ECHO) 1)
endif

MYSMARTUSB_PWRCYCLE := \
	$(if $(HAVE_MYSMARTUSB), \
		$(MYSMARTUSB) -p0 $(PROGPORT) && \
		sleep 1 && \
		$(MYSMARTUSB) -p1 $(PROGPORT) \
	)

MYSMARTUSB_PROGMODE := \
	$(if $(HAVE_MYSMARTUSB), \
		$(MYSMARTUSB) -mp $(PROGPORT) \
	)

MYSMARTUSB_DATAMODE := \
	$(if $(HAVE_MYSMARTUSB), \
		$(MYSMARTUSB) -md $(PROGPORT) \
	)

DEPS = $(sort $(patsubst %.c,dep/%.d,$(1)))
OBJS = $(sort $(patsubst %.c,obj/%.o,$(1)))

# Generate dependencies
$(call DEPS,$(SRCS)): dep/%.d: %.c 
	@$(MKDIR) -p $(dir $@)
	@$(MKDIR) -p obj
	$(QUIET_DEPEND) -o $@.tmp -MM -MT "$@ $(patsubst dep/%.d,obj/%.o,$@)" $(CFLAGS) $<
	@$(MV) -f $@.tmp $@

ifeq ($(NODEPS),)
-include $(call DEPS,$(SRCS))
endif

# Generate object files
$(call OBJS,$(SRCS)): obj/%.o: %.c
	@$(MKDIR) -p $(dir $@)
	$(QUIET_CC) -o $@ -c $(CFLAGS) $<

all: $(HEX)

%.s: %.c
	$(QUIET_CC) $(CFLAGS) -S $*.c

$(BIN): $(call OBJS,$(SRCS))
	$(QUIET_CC) $(CFLAGS) -o $(BIN) $(call OBJS,$(SRCS)) $(LDFLAGS) $(LDFLAGS)

$(HEX): $(BIN)
	$(QUIET_OBJCOPY) -R.eeprom -O ihex $(BIN) $(HEX)
#	$(QUIET_OBJCOPY) -j.eeprom --set-section-flags=.eeprom="alloc,load" \
#			 --change-section-lma .eeprom=0 -O ihex $(BIN) $(EEP)
	$(QUIET_SIZE) $(BIN)

avrdude:
	$(call MYSMARTUSB_PROGMODE)
	$(AVRDUDE) -B $(AVRDUDE_SPEED) -p $(AVRDUDE_ARCH) \
	 -c $(PROGRAMMER) -P $(PROGPORT) -t
	$(call MYSMARTUSB_PWRCYCLE)
	$(call MYSMARTUSB_DATAMODE)

install_flash: all
	$(call MYSMARTUSB_PROGMODE)
	$(AVRDUDE) -B $(AVRDUDE_SPEED) -p $(AVRDUDE_ARCH) \
	 -c $(PROGRAMMER) -P $(PROGPORT) \
	 -U flash:w:$(HEX)
	$(call MYSMARTUSB_DATAMODE)

install_eeprom: all
	$(call MYSMARTUSB_PROGMODE)
	$(AVRDUDE) -B $(AVRDUDE_SPEED) -p $(AVRDUDE_ARCH) \
	 -c $(PROGRAMMER) -P $(PROGPORT) -U eeprom:w:$(EEP)
	$(call MYSMARTUSB_DATAMODE)

install: install_flash
	$(call MYSMARTUSB_PWRCYCLE)

# Reset the microcontroller through avrdude
reset:
	$(call MYSMARTUSB_PROGMODE)
	$(AVRDUDE) -B $(AVRDUDE_SPEED) -p $(AVRDUDE_ARCH) \
	 -c $(PROGRAMMER) -P $(PROGPORT) \
	 -U signature:r:/dev/null:i -q -q
	$(call MYSMARTUSB_PWRCYCLE)

writefuse:
	$(call MYSMARTUSB_PROGMODE)
	$(AVRDUDE) -B $(AVRDUDE_SLOW_SPEED) -p $(AVRDUDE_ARCH) \
	 -c $(PROGRAMMER) -P $(PROGPORT) -q -q \
	 -U lfuse:w:$(LFUSE):m \
	 -U hfuse:w:$(HFUSE):m
	$(call MYSMARTUSB_DATAMODE)
	$(call MYSMARTUSB_PWRCYCLE)

doxygen:
	-$(RM) -Rf doc
	$(MKDIR) doc
	$(DOXYGEN) Doxyfile
	$(CP) *.jpg doc/latex/
	$(CP) *.jpg doc/html/
	$(MAKE) -C doc/latex pdf
	$(MV) doc/latex/refman.pdf doc/README-morsedecoder.pdf

clean:
	-$(RM) -rf obj dep $(BIN) doc/latex

distclean: clean
	-$(RM) -rf $(patsubst %.c,%.s,$(SRCS)) $(HEX) $(EEP) doc
