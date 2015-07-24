CC ?= gcc
CFLAGS = -Wall -std=c99 -pedantic \
	-Ofast -flto -msse4.2 -pipe -s \
	-march=native -Ofast -flto

DEBUG_CFLAGS = -Wall -Wextra -std=c99 -pedantic -msse4.2 \
	-Og -g -rdynamic -pg \
	-Wformat=2 -D_FORTIFY_SOURCE=2 --param=ssp-buffer-size=4 \
	-fstack-protector-all -Wstack-protector \
	-fsanitize=address -ftrapv \
	-fstrict-overflow -Wstrict-overflow=5 \
	-fstrict-aliasing -Wstrict-aliasing \

LDFLAGS =

NAME = yaftm

HDR = *.h ../*.h ../fb/*.h ../ctrlseq/*.h
DST = $(NAME)
DST_DEBUG = $(NAME)_debug

SRC = $(NAME).c

all: $(DST)

$(DST): $(SRC) $(HDR)
	$(CC) -o $@ $< $(DEBUG_CFLAGS) $(LDFLAGS)

debug: $(SRC) $(HDR)
	$(CC) -o $(DST_DEBUG) $< $(DEBUG_CFLAGS) $(LDFLAGS)

clean:
	rm -f $(DST) $(DST_DEBUG)
