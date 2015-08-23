#
# Copyright (C) 2011-2012 B.C. <bcc24x7@gmail.com>
#

VERS=0.2
CC=gcc
DEBUG=1
CFLAGS=	-Wall
#CFLAGS += -Wno-pointer-sign
ifeq ($(DEBUG),1)
CFLAGS += -g
else
CFLAGS += -O3 -Wuninitialized
endif
ifeq ($(OSTYPE),darwin)
LDFLAGS +=	-arch i386 -arch x86_64
endif

ALL=	radeon_bios_decode
RADEON_BIOS_DECODE= radeon_bios_decode.c
INCLUDES= radeon_bios_decode.h atombios.h radeon.h

all:	$(ALL)

clean:
	rm -f *.o $(ALL)

radeon_bios_decode:	$(RADEON_BIOS_DECODE)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

.c.o:
	$(CC) -c $(CFLAGS) $<

cscope:
	cscope -b *.c *.h

ship:
	zip -r radeon_bios_decode.$(VERS).src.zip $(RADEON_BIOS_DECODE) $(INCLUDES) Makefile
