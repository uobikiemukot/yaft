CC = gcc
CFLAGS = -march=native -Os -pipe -Wall
	#-Wall -std=c99 -pedantic \
	#-march=native -Ofast -flto -pipe -s
	#-march=native -Os -pipe -Wall
	#-pg -g -rdynamic
LDFLAGS = -lutil -lncurses

HDR = *.h
DST = yaskk
SRC = yaskk.c

all: $(DST)

$(DST): $(SRC) $(HDR)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@

clean:
	rm -f $(DST)
