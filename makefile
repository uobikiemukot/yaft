CC ?= gcc
#CC ?= clang

CFLAGS  ?= -std=c99 -pedantic -Wall -Wextra -O3 -s -pipe
LDFLAGS ?=

XCFLAGS  ?= -std=c99 -pedantic -Wall -Wextra -I/usr/include/X11/ -O3 -s -pipe
XLDFLAGS ?= -lX11

HDR = glyph.h yaft.h conf.h color.h parse.h terminal.h util.h \
	ctrlseq/esc.h ctrlseq/csi.h ctrlseq/osc.h ctrlseq/dcs.h \
	fb/common.h fb/linux.h fb/freebsd.h fb/netbsd.h fb/openbsd.h \
	x/x.h

prefix ?= $(DESTDIR)/usr/local
mandir ?= $(prefix)/share/man
#terminfo ?= $(prefix)/share/terminfo		# Not used in Debian, instead
terminfo ?= /etc/terminfo			# store terminfo in /etc.

all: yaft

yaft: mkfont_bdf

yaftx: mkfont_bdf

mkfont_bdf: tools/mkfont_bdf.c tools/mkfont_bdf.h tools/bdf.h tools/util.h
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)


# This makefile creates a default glyph.h, if you haven't created one using ./glyphbuilder.sh.
glyph.h: mkfont_bdf
	# If you want to use your favorite fonts, please run ./glyphbuilder.sh.
	# Or, you can change the following line and delete glyph.h.
	# USAGE: mkfont_bdf ALIAS BDF1 BDF2 BDF3... > glyph.h
	# ALIAS: glyph substitution rule file (see table/alias for more detail)
	# BDF1 BDF2 BDF3...: monospace bdf files (must be the same size)
	# If there is more than one glyph of the same codepoint, the glyph included in the first bdf file is choosen.
	./mkfont_bdf table/alias fonts/milkjf/milkjf_k16.bdf fonts/milkjf/milkjf_8x16r.bdf fonts/milkjf/milkjf_8x16.bdf fonts/terminus/ter-u16n.bdf > glyph.h

yaft: yaft.c $(HDR)
	# If you want to change configuration, please modify conf.h before make (see conf.h for more detail)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

yaftx: x/yaftx.c $(HDR)
	# If you want to change configuration, please modify conf.h before make (see conf.h for more detail)
	$(CC) -o $@ $< $(XCFLAGS) $(XLDFLAGS)

install: installshare
	mkdir -p $(prefix)/bin/
	install -m755 ./yaft $(prefix)/bin/yaft
	install -m755 ./yaft_wall $(prefix)/bin/yaft_wall

installx: installshare
	mkdir -p $(prefix)/bin/
	install -m755 ./yaftx $(prefix)/bin/yaftx
	mkdir -p $(terminfo)
	tic -o $(terminfo) info/yaft.src

installshare:
	mkdir -p $(mandir)/man1/
	install -m644 ./man/yaft.1 $(mandir)/man1/yaft.1
	mkdir -p $(terminfo)
	tic -o $(terminfo) info/yaft.src

uninstall:
	rm -f $(prefix)/bin/yaft
	rm -f $(prefix)/bin/yaft_wall

uninstallx:
	rm -f $(prefix)/bin/yaftx

clean:
	rm -f yaft yaftx mkfont_bdf glyph.h
