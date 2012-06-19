CC = gcc
CFLAGS = -march=native -mtune=native \
	-Ofast -flto -msse2 -pipe \
	-ffast-math -fno-strict-aliasing
	#-pg
LDFLAGS = -lutil

HDR = *.h
DST = yaft
SRC = yaft.c

all: $(DST)

$(DST): $(SRC) $(HDR)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(DST)
