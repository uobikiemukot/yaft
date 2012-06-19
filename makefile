CC = gcc
CFLAGS = -march=native -mtune=native -O3 -pipe
	#-pg
LDFLAGS = -lutil
PREFIX = /usr/bin
RCDIR = ~/.yaft

HDR = *.h
DST = yaft
SRC = yaft.c

all: $(DST)

install:
	tic info/yaft.info
	mkdir -p $(RCDIR)
	cp -f fonts/efont.yaft $(RCDIR)
	cp -f wall/karmic-gray.ppm $(RCDIR)
	cp yaft $(PREFIX)

uninstall:
	rm -rf $(RCDIR)
	rm -f $(PREFIX)/yaft

$(DST): $(SRC) $(HDR)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)
	strip $@

clean:
	rm -f $(DST)
