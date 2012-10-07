CC = gcc
CFLAGS = -std=c99 -pedantic -Os -march=native -pipe -Wall -I/usr/X11R6/include -L/usr/X11R6/lib
LDFLAGS = -lutil -lX11 -I/usr/X11R6/include -L/usr/X11R6/lib

HDR = *.h
DST = yaftx
SRC = yaftx.c

all: $(DST)

$(DST): $(SRC) $(HDR)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)
	strip $@

clean:
	rm -f $(DST)
