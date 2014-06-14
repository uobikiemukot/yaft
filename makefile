CC = gcc
#CC = clang
CFLAGS += -std=c99 -pedantic -Wall \
-Os -s -pipe
LDFLAGS +=

XCFLAGS += -I/usr/include/X11/
XLDFLAGS += -lX11

HDR = *.h
DST = yaft
SRC = yaft.c
DESTDIR =
PREFIX = $(DESTDIR)/usr

all: $(DST)

$(DST): mkfont_bdf

yaftx: mkfont_bdf

mkfont_bdf: tools/mkfont_bdf.c tools/font.h tools/bdf.h
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

$(DST): $(SRC) $(HDR)
	./mkfont_bdf table/alias fonts/milkjf_k16.bdf fonts/milkjf_8x16.bdf fonts/milkjf_8x16r.bdf > glyph.h
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

yaftx: yaftx.c $(HDR)
	./mkfont_bdf table/alias fonts/milkjf_k16.bdf fonts/milkjf_8x16.bdf fonts/milkjf_8x16r.bdf > glyph.h
	$(CC) -o $@ $< $(XCFLAGS) $(XLDFLAGS)

install:
	mkdir -p $(PREFIX)/share/terminfo
	tic -o $(PREFIX)/share/terminfo info/yaft.src
	install -Dm755 {./,$(PREFIX)/bin/}yaft
	install -Dm755 {./,$(PREFIX)/bin/}yaft_wall

uninstall:
	rm -rf $(PREFIX)/bin/yaft
	rm -rf $(PREFIX)/bin/yaft_wall

clean:
	rm -f $(DST) yaftx mkfont_bdf glyph.h
