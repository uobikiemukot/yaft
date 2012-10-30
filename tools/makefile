CC = g++
CFLAGS = -march=native -mtune=native -O3 -pipe
	#-pg
#LDFLAGS = -lutil

DST = bdf2yaft yaftmerge mkblank
SRC = bdf2yaft.cpp yaftmerge.cpp mkblank.c

all: $(DST)

bdf2yaft: bdf2yaft.cpp
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)
	strip $@

yaftmerge: yaftmerge.cpp
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)
	strip $@

mkblank: mkblank.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)
	strip $@


clean:
	rm -f $(DST)
