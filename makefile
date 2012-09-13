CC = gcc
CFLAGS += -std=c99 -pedantic -Wall -Os
LDFLAGS += -lutil -lm

HDR = *.h
DST = yaft
SRC = yaft.c
PREFIX = /usr

all: $(DST)

$(DST): $(SRC) $(HDR)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

install:
	tic yaft.src
	install -Dm755 {./,$PREFIX/bin/}yaft
	install -Dm755 {./,$PREFIX/bin/}yaft_wall
	install -Dm644 {./fonts/,$PREFIX/share/yaft/fonts/}milkjf-iso8859.yaft
	install -Dm644 {./fonts/,$PREFIX/share/yaft/fonts/}milkjf-jis0201.yaft
	install -Dm644 {./fonts/,$PREFIX/share/yaft/fonts/}milkjf-jis0208.yaft
	install -Dm644 {./fonts/,$PREFIX/share/yaft/fonts/}ambiguous-half.yaft
	install -Dm644 {./fonts/,$PREFIX/share/yaft/alias/}ambiguous-half.alias

uninstall:
	rm -rf $PREFIX/bin/yaft
	rm -rf $PREFIX/bin/yaft_wall
	rm -rf $PREFIX/share/yaft

clean:
	rm -f $(DST)
