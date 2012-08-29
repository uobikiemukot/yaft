CC = gcc
CFLAGS = -std=c99 -pedantic -Os
	-march=native -mtune=native \
	-Ofast -flto -pipe -Wall -I/usr/X11R6/include -L/usr/X11R6/lib
	#-pg
LDFLAGS = -lutil -lm -lX11 -I/usr/X11R6/include -L/usr/X11R6/lib

HDR = *.h
DST = yaftx
SRC = yaftx.c

all: $(DST)

$(DST): $(SRC) $(HDR)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)
	strip $@

clean:
	rm -f $(DST)
