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

install-font:
	mkdir -p $(RCDIR)
	cp -f fonts/shinm.yaft $(RCDIR)
	cp -f wall/karmic-gray.ppm $(RCDIR)

install:
	tic info/yaft.info
	cp -f yaft $(PREFIX)

uninstall-font:
	rm -rf $(RCDIR)

uninstall:
	rm -f $(PREFIX)/yaft

$(DST): $(SRC) $(HDR)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)
	strip $@

clean:
	rm -f $(DST)
