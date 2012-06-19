CC = g++
CFLAGS = -march=native -mtune=native \
	-Ofast -flto -msse2 -pipe \
	-ffast-math -fno-strict-aliasing
	#-pg
#LDFLAGS = -lutil

HDR =
DST = bdf2yaft yaftmerge
SRC = bdf2yaft.cpp yaftmerge.cpp

all: $(DST)

$(DST): $(SRC) $(HDR)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(DST)
