CC ?= gcc
#CC ?= clang

CFLAGS  ?= -std=c99 -pedantic -Wall -Wextra -O3 -s -pipe
LDFLAGS ?=

XCFLAGS  ?= -std=c99 -pedantic -Wall -Wextra -I/usr/include/X11/ -O3 -s -pipe
XLDFLAGS ?= -lX11

HDR = glyph.h yaft.h conf.h color.h parse.h terminal.h util.h \
	ctrlseq/esc.h ctrlseq/csi.h \
	fb/common.h fb/linux.h fb/freebsd.h fb/netbsd.h fb/openbsd.h \
	x/x.h

DESTDIR   =
PREFIX    = $(DESTDIR)/usr
MANPREFIX = $(DESTDIR)/usr/share/man

all: yaft

yaft: mkfont_bdf

yaftx: mkfont_bdf

mkfont_bdf: tools/mkfont_bdf.c tools/mkfont_bdf.h tools/bdf.h tools/util.h
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

glyph.h: mkfont_bdf
	# If you want to use your favorite fonts, please change following line
	# usage: mkfont_bdf ALIAS BDF1 BDF2 BDF3... > glyph.h
	# ALIAS: glyph substitution rule file (see table/alias for more detail)
	# BDF1 BDF2 BDF3...: monospace bdf files (must be the same size)
	# If there is more than one glyph of the same codepoint, the glyph included in the first bdf file is choosed
	./mkfont_bdf table/alias fonts/milkjf_k16.bdf fonts/milkjf_8x16r.bdf fonts/milkjf_8x16.bdf > glyph.h

yaft: yaft.c $(HDR)
	# If you want to change configuration, please modify conf.h before make (see conf.h for more detail)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

yaftx: x/yaftx.c $(HDR)
	# If you want to change configuration, please modify conf.h before make (see conf.h for more detail)
	$(CC) -o $@ $< $(XCFLAGS) $(XLDFLAGS)

install:
	mkdir -p $(PREFIX)/share/terminfo
	tic -o $(PREFIX)/share/terminfo info/yaft.src
	mkdir -p $(PREFIX)/bin/
	install -m755 ./yaft $(PREFIX)/bin/yaft
	install -m755 ./yaft_wall $(PREFIX)/bin/yaft_wall
	mkdir -p $(MANPREFIX)/man1/
	install -m644 ./man/yaft.1 $(MANPREFIX)/man1/yaft.1

installx:
	mkdir -p $(PREFIX)/share/terminfo
	tic -o $(PREFIX)/share/terminfo info/yaft.src
	mkdir -p $(PREFIX)/bin/
	install -m755 ./yaftx $(PREFIX)/bin/yaftx

uninstall:
	rm -f $(PREFIX)/bin/yaft
	rm -f $(PREFIX)/bin/yaft_wall

uninstallx:
	rm -f $(PREFIX)/bin/yaftx

clean:
	rm -f yaft yaftx mkfont_bdf glyph.h
