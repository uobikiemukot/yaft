CC = gcc
CFLAGS = -std=c99 -pedantic -Os
	-march=native -mtune=native \
	-Ofast -flto -pipe -Wall
	#-pg
LDFLAGS = -lutil -lm -lX11

HDR = *.h
DST = yaftx
SRC = yaftx.c

all: $(DST)

$(DST): $(SRC) $(HDR)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)
	strip $@

clean:
	rm -f $(DST)
