SHELL = /bin/bash
CC = gcc
CFLAGS += -std=c99 -pedantic -Wall
LDFLAGS +=

HDR = *.h
DST = yaft
SRC = yaft.c
DESTDIR =
PREFIX = $(DESTDIR)/usr
TERMCAP = /usr/share/misc/termcap

all: $(DST)

$(DST): $(SRC) $(HDR)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

install:
	cat ../terminfo/yaft.cap >> $(TERMCAP)
	cap_mkdb $(TERMCAP)
	install -m755 {./,$(PREFIX)/bin/}yaft
	install -m755 {../,$(PREFIX)/bin/}yaft_wall
	install -d $(PREFIX)/share/yaft/fonts
	install -d $(PREFIX)/share/yaft/alias
	install -m644 {../fonts/,$(PREFIX)/share/yaft/fonts/}milkjf-iso8859.yaft
	install -m644 {../fonts/,$(PREFIX)/share/yaft/fonts/}milkjf-jis0201.yaft
	install -m644 {../fonts/,$(PREFIX)/share/yaft/fonts/}milkjf-jis0208.yaft
	install -m644 {../fonts/,$(PREFIX)/share/yaft/fonts/}ambiguous-half.yaft
	install -m644 {../fonts/,$(PREFIX)/share/yaft/alias/}ambiguous-wide.alias
	install -m644 {../fonts/,$(PREFIX)/share/yaft/alias/}ambiguous-half.alias

uninstall:
	rm -rf $(PREFIX)/bin/yaft
	rm -rf $(PREFIX)/bin/yaft_wall
	rm -rf $(PREFIX)/share/yaft

clean:
	rm -f $(DST)
