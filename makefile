CC = gcc
CFLAGS += -std=c99 -pedantic -Wall
LDFLAGS +=

HDR = *.h
DST = yaft
SRC = yaft.c
DESTDIR =
PREFIX = $(DESTDIR)/usr

all: $(DST)

$(DST): mkfont_bdf

mkfont_bdf: tools/mkfont_bdf.c tools/font.h tools/bdf.h
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

$(DST): $(SRC) $(HDR)
	./mkfont_bdf table/alias fonts/milkjf_k16.bdf fonts/milkjf_8x16.bdf fonts/milkjf_8x16r.bdf > glyph.h
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

install:
	mkdir -p $(PREFIX)/share/terminfo
	tic -o $(PREFIX)/share/terminfo info/yaft.src
	install -Dm755 {./,$(PREFIX)/bin/}yaft
	install -Dm755 {./,$(PREFIX)/bin/}yaft_wall

uninstall:
	rm -rf $(PREFIX)/bin/yaft
	rm -rf $(PREFIX)/bin/yaft_wall

clean:
	rm -f $(DST) mkfont_bdf glyph.h
