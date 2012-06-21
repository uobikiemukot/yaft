CC = gcc
CFLAGS = -march=native -mtune=native -O3 -pipe
	#-pg
LDFLAGS = -lutil
PREFIX = /usr/bin
FONTDIR = ~/.fonts

HDR = *.h
DST = yaft
SRC = yaft.c

all: $(DST)

install-font:
	mkdir -p $(FONTDIR)
	cp -f fonts/shinm.yaft $(FONTDIR)

install:
	tic info/yaft.src
	cp -f yaft $(PREFIX)

uninstall-font:
	rm -f $(FONTDIR)/shinm.yaft

uninstall:
	rm -f $(PREFIX)/yaft

$(DST): $(SRC) $(HDR)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)
	strip $@

clean:
	rm -f $(DST)
