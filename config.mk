# PWman version
VERSION = 0.4.4

# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# includes and libs
INCS = -I. -I/usr/include -I/usr/pkg/include `xml2-config --cflags` `libgcrypt-config --cflags`
LIBS = -L/usr/lib -lc -lcurses `xml2-config --libs` `libgcrypt-config --libs`

# flags
CPPFLAGS = -DVERSION=\"${VERSION}\" -D_GNU_SOURCE
CFLAGS = -std=c99 -pedantic -Wall -Os ${INCS} ${CPPFLAGS}
LDFLAGS = -s ${LIBS}

# compiler and linker
CC = cc
