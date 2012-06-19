CC = g++
CFLAGS = -march=native -mtune=native -O3 -pipe
	#-pg
#LDFLAGS = -lutil

DST = bdf2yaft yaftmerge
SRC = bdf2yaft.cpp yaftmerge.cpp

all: $(DST)

bdf2yaft: bdf2yaft.cpp
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)
	strip $@

yaftmerge: yaftmerge.cpp
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)
	strip $@

clean:
	rm -f $(DST)
