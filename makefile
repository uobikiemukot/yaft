CC = gcc
CFLAGS = -march=native -mtune=native -O3 -pipe
	#-pg
LDFLAGS = -lutil

HDR = *.h
DST = yaft
SRC = yaft.c

all: $(DST)

install:
	tic info/yaft.info
	mkdir -p ~/.yaft
	cp -n fonts/efont.yaft ~/.yaft/
	cp -n wall/karmic-gray.ppm ~/.yaft/
	cp yaft /usr/bin/

$(DST): $(SRC) $(HDR)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)
	strip $@

clean:
	rm -f $(DST)
