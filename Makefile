# BubbleFishyMon with gkrellm support
# Type 'make' or 'make bubblefishymon' to build bubblefishymon
# Type 'make bubblefishymon1' to build bubblefishymon for gtk1
# Type 'make gkrellm' to build gkrellm-bfm.so for gkrellm2
# Type 'make gkrellm1' to build gkrellm-bfm.so for gkrellm

# where to install this program
PREFIX ?= /usr/local

# bubblemon configuration
EXTRA = -DENABLE_DUCK
EXTRA += -DENABLE_CPU
EXTRA += -DENABLE_MEMSCREEN
EXTRA += -DENABLE_FISH
EXTRA += -DENABLE_TIME
# EXTRA += -DUPSIDE_DOWN_DUCK
# EXTRA += -DKDE_DOCKAPP

# If building for Linux define the network device to monitor.
NET_DEVICE = ppp0


###############################################################################
# no user serviceable parts below                                             #
###############################################################################
EXTRA += $(WMAN)

# gtk cflags and gtk lib flags
GTK_CFLAGS = $(shell gtk-config --cflags)
GTK_LIBS = $(shell gtk-config --libs)

GTK2_CFLAGS = $(shell pkg-config gtk+-2.0 --cflags)
GTK2_LIBS = $(shell pkg-config gtk+-2.0 --libs)


# optimization cflags
#CFLAGS = -O3 -Wall ${EXTRA}
CFLAGS = ${EXTRA}

# profiling cflags
# CFLAGS = -ansi -Wall -pg -O3 ${EXTRA} -DPRO
# test coverage cflags
# CFLAGS = -fprofile-arcs -ftest-coverage -Wall -ansi -g ${EXTRA} -DPRO


SHELL = sh
OS = $(shell uname -s)
SRCS = fishmon.c bubblemon.c
OBJS = fishmon.o bubblemon.o
BUBBLEFISHYMON = bubblefishymon

# Some stuffs for building gkrellm-bfm
GKRELLM_SRCS = gkrellm-bfm.c
GKRELLM_OBJS = gkrellm-bfm.o
GKRELLM_BFM = gkrellm-bfm.so
LDFLAGS = -shared -Wl

STRIP = strip

CC = gcc

INSTALLMAN = -m 644



# special things for Linux
ifeq ($(OS), Linux)
ifeq "$(NET_DEVICE)" ""
	CFLAGS+=-DNET_DEVICE=\"eth0\"
else
	CFLAGS+=-DNET_DEVICE=\"$(NET_DEVICE)\"
endif
	SRCS += sys_linux.c
	OBJS += sys_linux.o
	INSTALL = -m 755
	INSTALLMAN = -m 644
endif

# special things for FreeBSD
ifeq ($(OS), FreeBSD)
	SRCS += sys_freebsd.c
    OBJS += sys_freebsd.o
    LIBS = -lkvm
    INSTALL = -c -g kmem -m 2755 -o root
endif

# special things for OpenBSD
ifeq ($(OS), OpenBSD)
    OBJS += sys_openbsd.o
    LIBS = `gtk-config --libs | sed "s/-lgtk//g"`
endif

#special things for SunOS
ifeq ($(OS), SunOS)
    # try to detect if gcc is available (also works if you call gmake CC=cc to
    # select the sun compilers on a system with both)
    COMPILER=$(shell \
        if [ `$(CC) -v 2>&1 | egrep -c '(gcc|egcs|g\+\+)'` = 0 ]; then \
	    echo suncc; else echo gcc; fi)

    # if not, fix up CC and the CFLAGS for the Sun compiler
    ifeq ($(COMPILER), suncc)
	CC=cc
	CFLAGS=-v -xO3
    endif

    ifeq ($(COMPILER), gcc)
	CFLAGS=-O3 -Wall
    endif
    CFLAGS += ${EXTRA}
	SRCS += sys_sunos.c
    OBJS += sys_sunos.o
    LIBS = -lkstat -lm
    INSTALL = -m 755
endif

all: $(BUBBLEFISHYMON)

gkrellm: clean_obj
	$(CC) -DGKRELLM2 -DGKRELLM_BFM -fPIC $(GTK2_CFLAGS) $(CFLAGS) -c $(SRCS) \
		$(GKRELLM_SRCS)
	$(CC) $(GTK2_LIBS) $(LDFLAGS) -o $(GKRELLM_BFM) $(OBJS) $(GKRELLM_OBJS)
	$(STRIP) $(GKRELLM_BFM)

gkrellm1: clean_obj
	$(CC) -DGKRELLM_BFM -fPIC $(GTK_CFLAGS) $(CFLAGS) -c $(SRCS) \
		$(GKRELLM_SRCS)
	$(CC) $(GTK_LIBS) $(LDFLAGS) -o $(GKRELLM_BFM) $(OBJS) $(GKRELLM_OBJS)
	$(STRIP) $(GKRELLM_BFM)

bubblefishymon: clean_obj
	$(CC) $(GTK2_CFLAGS) $(CFLAGS) -o $(BUBBLEFISHYMON) \
		$(LIBS) $(GTK2_LIBS) $(SRCS)
	$(STRIP) $(BUBBLEFISHYMON)

bubblefishymon1: clean_obj
	$(CC) $(GTK_CFLAGS) $(CFLAGS) -o $(BUBBLEFISHYMON) \
		$(LIBS) $(GTK_LIBS) $(SRCS)
	$(STRIP) $(BUBBLEFISHYMON)

clean_obj:
	rm -rf *.o

clean:
	rm -f bubblefishymon *.o *.bb* *.gcov gmon.* *.da *~ *.so

install: 
	install -d $(DESTDIR)/$(PREFIX)/bin $(DESTDIR)/$(PREFIX)/share/man/man1
	install $(INSTALL) $(BUBBLEFISHYMON) $(DESTDIR)/$(PREFIX)/bin
	install $(INSTALL_MAN) doc/bubblefishymon.1 $(DESTDIR)/$(PREFIX)/share/man/man1
