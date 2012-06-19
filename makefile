CC = g++
CFLAGS = -march=native -mtune=native -O3 -pipe
	#-pg
#LDFLAGS = -lutil

HDR =
DST = bdf2yaft yaftmerge
SRC = bdf2yaft.cpp yaftmerge.cpp

all: $(DST)

$(DST): $(SRC) $(HDR)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)
	strip $@

clean:
	rm -f $(DST)
