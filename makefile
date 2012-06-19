CC = gcc
CFLAGS = -march=native -mtune=native #-O3 -pipe
	#-pg
LDFLAGS = -lutil

HDR = *.h
DST = yaft
SRC = yaft.c

all: $(DST)

$(DST): $(SRC) $(HDR)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)
	#strip $@

clean:
	rm -f $(DST)
