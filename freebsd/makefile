CC = gcc
CFLAGS = -std=c99 -pedantic -Os -march=native -pipe -Wall
LDFLAGS =

HDR = *.h
DST = yaft
SRC = yaft.c

all: $(DST)

$(DST): $(SRC) $(HDR)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(DST)
