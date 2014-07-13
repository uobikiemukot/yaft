CC ?= gcc
#CC ?= clang

CFLAGS  ?= -std=c99 -pedantic -Wall -Wextra -O3 -s -pipe
LDFLAGS ?=

XCFLAGS  ?= -std=c99 -pedantic -Wall -Wextra -I/usr/include/X11/ -O3 -s -pipe
XLDFLAGS ?= -lX11

HDR = color.h conf.h dcs.h draw.h function.h osc.h parse.h terminal.h util.h yaft.h glyph.h \
	fb/linux.h fb/freebsd.h fb/netbsd.h fb/openbsd.h x/x.h
DESTDIR =
PREFIX  = $(DESTDIR)/usr

all: yaft

yaft: mkfont_bdf

yaftx: mkfont_bdf

glyph.h: mkfont_bdf
	# please change following line, if you want to use your favorite fonts
	./mkfont_bdf table/alias fonts/milkjf_k16.bdf fonts/milkjf_8x16r.bdf fonts/milkjf_8x16.bdf > glyph.h

mkfont_bdf: tools/mkfont_bdf.c tools/font.h tools/bdf.h
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

yaft: yaft.c $(HDR)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

yaftx: x/yaftx.c $(HDR)
	$(CC) -o $@ $< $(XCFLAGS) $(XLDFLAGS)

install:
	mkdir -p $(PREFIX)/share/terminfo
	tic -o $(PREFIX)/share/terminfo info/yaft.src
	mkdir -p $(PREFIX)/bin/
	install -m755 ./yaft $(PREFIX)/bin/yaft
	install -m755 ./yaft_wall $(PREFIX)/bin/yaft_wall

uninstall:
	rm -rf $(PREFIX)/bin/yaft
	rm -rf $(PREFIX)/bin/yaft_wall

clean:
	rm -f yaft yaftx mkfont_bdf glyph.h
