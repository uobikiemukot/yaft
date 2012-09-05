CC = g++
CFLAGS = -Wall -std=c++98 -pedantic \
	-march=native -O3 -pipe -msse2
LDFLAGS =

HDR = *.h
DST = nakshogi
SRC = nakshogi.cpp

all: $(DST)

$(DST): $(SRC) $(HDR)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(DST)
