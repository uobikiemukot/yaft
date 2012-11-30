#define _XOPEN_SOURCE 600
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <pty.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <curses.h>
#include <term.h>

enum ctrl_chars {
	LF = 0x0A, /* Ctrl-J */
	SPACE = 0x20,
	DEL = 0x7F,
};

enum misc {
	RESET = 0x00,
	BUFSIZE = 1024,
	SELECT_TIMEOUT = 20000,
	MAX_PARAMS = 256,
};

enum mode {
	MODE_ASCII = 1,
	MODE_HIRAGANA,
	MODE_KATAKANA,
	MODE_ZENEI,
	MODE_HANKANA,
	MODE_HENKAN,
};

enum hash_parm {
	NHASH = 512,
	//MULTIPLIER = 31,
	MULTIPLIER = 37,
	KEYSIZE = 8,
	VALSIZE = 16,
};

struct parm_t {
	int argc;
	char *argv[MAX_PARAMS];
};

struct map_t {
	char key[KEYSIZE], val[VALSIZE];
	struct map_t *next;
};

struct list_t {
	char c;
	struct list_t *next;
};

/*
struct entry_t {
	char *key;
	char *entry;
	long offset;
	struct parm_t parm;
};
*/

struct entry_t {
	char *key;
	long offset;
};

struct entry_table_t {
	struct entry_t *entries;
	int count;
};

struct map_t *r2h[NHASH], *r2k[NHASH];

#include "conf.h"
