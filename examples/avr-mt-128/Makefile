TARGET		= $(CURDIR)
OS		= $(shell cd ../..; pwd)
include target.cfg

TESTS		= test_debug.srec test_task.srec test_timer.srec \
		  test_uart.srec test_lcd.srec test_olimex.srec \
		  test_nvram.srec philosophers.srec
OUTS		= $(TESTS:%.srec=%.elf)

# Using avrdude programmer with STK200 LPT adapter.
#PROG		= avrdude -cstk200 -Enoreset -patmega128

# Using avrdude programmer with JTAGICE USB adapter.
PROG		= avrdude -cjtag1 -P /dev/ttyUSB? -patmega128

# Using avrdude programmer with AVRISP USB adapter.
#PROG		= avrdude -cstk500v2 -P /dev/ttyUSB? -patmega128

FUSES		= -Ulfuse:w:0xFE:m -Uhfuse:w:0x1F:m -Uefuse:w:0xFF:m

all:		$(OUTS) $(TESTS)
		$(SIZE) $(OUTS)

$(OUTS) $(TESTS): startup.o libuos.a

clean:
		rm -rf *.elf *.hex *.srec *.[oasi] *.lst *.dis *~ .deps $(MODULES)

probe:
		$(PROG) -v

load:		philosophers.srec
		$(PROG) -e -U$<	#$(FUSES)

olimex:		mt-128.olimex
		$(PROG) -e -U$< $(FUSES)

include $(OS)/sources/rules.mak

###
