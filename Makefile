CSOURCES = $(wildcard *.c)
ASOURCES = $(wildcard *.s)

PROGRAM = prog6502
CC65_TARGET = none
CC65_LIB = sbc/sbc.lib
CC65_CFG = sbc/sbc.cfg

CC      = cl65
DA      = da65
BIN2HEX = srec_cat
CFLAGS  = --cpu 6502 -t $(CC65_TARGET) --create-dep $(<:.c=.d) -O
AFLAGS  = --cpu 6502 -t $(CC65_TARGET) --create-dep $(<:.s=.d) -O
LDFLAGS = -t $(CC65_TARGET) -vm -m $(PROGRAM).map -C $(CC65_CFG) 
DAFLAGS = --comments 4 -vvvvvv -g -S 0xD000


########################################

.SUFFIXES:
.PHONY: all clean
#all: $(PROGRAM)
all: hex

ifneq ($(MAKECMDGOALS),clean)
-include $(CSOURCES:.c=.d)
-include $(ASOURCES:.s=.d)
endif

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.s
	$(CC) -c $(AFLAGS) -o $@ $<

$(PROGRAM): $(CSOURCES:.c=.o) $(ASOURCES:.s=.o)
	$(CC) $(LDFLAGS) -o $@ $^ $(CC65_LIB)

hex: $(PROGRAM)
	$(BIN2HEX) $(PROGRAM) -binary -o $(PROGRAM).hex -intel

a65: $(PROGRAM)
	$(DA) $(DAFLAGS) $(PROGRAM) -o $(PROGRAM).a65

clean:
	$(RM) $(CSOURCES:.c=.o) $(CSOURCES:.c=.d) \
	      $(ASOURCES:.s=.o) $(ASOURCES:.s=.d) \
	      $(PROGRAM) $(PROGRAM).map $(PROGRAM).hex

