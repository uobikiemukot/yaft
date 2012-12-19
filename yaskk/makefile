CC = gcc
CFLAGS = -Wall -std=c99 -pedantic \
	-march=native -Os -pipe -Wall
	#-march=native -Ofast -flto -pipe -s
	#-pg -g -rdynamic
LDFLAGS =

HDR = *.h
DST = yaskk
SRC = yaskk.c

all: $(DST)

$(DST): $(SRC) $(HDR)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@

clean:
	rm -f $(DST)
