# BubbleFishyMon with gkrellm support
# Type 'make' to build bubblefishymon
# Type 'make gkrellm' to build gkrellm-bfm.so

# bubblemon configuration
EXTRA = -DENABLE_DUCK
EXTRA += -DENABLE_CPU
EXTRA += -DENABLE_MEMSCREEN
EXTRA += -DENABLE_FISH
EXTRA += -DENABLE_TIME
# EXTRA += -DUPSIDE_DOWN_DUCK

# where to install this program
PREFIX = /usr/local

# no user serviceable parts below
EXTRA += $(WMAN)


# optimization cflags
CFLAGS = -O3 -Wall `gtk-config --cflags` ${EXTRA}

# profiling cflags
# CFLAGS = -ansi -Wall -pg -O3 `gtk-config --cflags` ${EXTRA} -DPRO
# test coverage cflags
# CFLAGS = -fprofile-arcs -ftest-coverage -Wall -ansi -g `gtk-config --cflags` ${EXTRA} -DPRO


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



# special things for Linux
ifeq ($(OS), Linux)
	SRCS += sys_linux.c
    OBJS += sys_linux.o
    LIBS = `gtk-config --libs | sed "s/-lgtk//g"`
    INSTALL = -m 755
endif

# special things for FreeBSD
ifeq ($(OS), FreeBSD)
	SRCS += sys_freebsd.c
    OBJS += sys_freebsd.o
    LIBS = `gtk-config --libs | sed "s/-lgtk//g"` -lkvm
    INSTALL = -c -g kmem -m 2755 -o root
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
    CFLAGS +=`gtk-config --cflags` ${EXTRA}
	SRCS += sys_sunos.c
    OBJS += sys_sunos.o
    LIBS = `gtk-config --libs` -lkstat -lm
    INSTALL = -m 755
endif

all: $(BUBBLEFISHYMON)

gkrellm: clean_obj
	$(CC) -DGKRELLM_BFM $(CFLAGS) -c $(SRCS) $(GKRELLM_SRCS)
	$(CC) $(LDFLAGS) -o $(GKRELLM_BFM) $(OBJS) $(GKRELLM_OBJS)
	$(STRIP) $(GKRELLM_BFM)

bubblefishymon: clean_obj $(OBJS)
	$(CC) $(CFLAGS) -o $(BUBBLEFISHYMON) $(OBJS) $(LIBS)
	$(STRIP) $(BUBBLEFISHYMON)

clean_obj:
	rm -rf *.o

clean:
	rm -f bubblefishymon *.o *.bb* *.gcov gmon.* *.da *~ *.so

install:
	install $(INSTALL) $(BUBBLEFISHYMON) $(PREFIX)/bin

