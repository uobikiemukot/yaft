CC = gcc
CFLAGS = -std=c99 -pedantic -Os -march=native -pipe -Wall -I/usr/X11R6/include
LDFLAGS = -lX11 -L/usr/X11R6/lib

HDR = *.h
DST = yaftx
SRC = yaft.c

all: $(DST)

$(DST): $(SRC) $(HDR)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(DST)
