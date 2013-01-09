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

#include "conf.h"
#include "util.h"
#include "list.h"
#include "hash.h"
#include "args.h"

enum ctrl_chars {
	BS = 0x08, /* backspace */
	LF = 0x0A, /* LF */
	ESC = 0x1B,
	SPACE = 0x20,
	DEL = 0x7F,
};

enum misc {
	RESET = 0x00,
	BUFSIZE = 1024,
	SELECT_TIMEOUT = 20000,
};

enum mode {
	MODE_ASCII = 0x01,
	MODE_HIRA = 0x02,
	MODE_KATA = 0x04,
	MODE_COOK = 0x08,
	MODE_SELECT = 0x10,
	MODE_APPEND = 0x20,
};

enum select {
	SELECT_EMPTY = -1,
	SELECT_LOADED = 0,
};

struct entry_t {
	char *key;
	long offset;
};

struct table_t {
	struct entry_t *entries;
	int count;
};

struct triplet_t {
	char *key, *hira, *kata;
};

struct map_t {
	struct triplet_t *triplets;
	int count;
};

struct candidate_t {
	FILE *fp;            /* dict fp */
	char entry[BUFSIZE]; /* buffer for entry buffer */
	struct parm_t parm;  /* candidate of table entry */
};

struct skk_t {
	int fd;                               /* master of pseudo terminal */
	int mode;                             /* input mode */
	int pwrote, kwrote;                   /* count of wroted character count */
	int select;                           /* candidate select status */
	struct map_t rom2kana;                /* romaji to kana map */
	struct table_t okuri_ari, okuri_nasi; /* convert table */
	struct list_t *key;                   /* keyword string for table lookup */
	struct list_t *append;
	struct list_t *preedit;               /* preedit string for map lookup */
	struct candidate_t candidate;
};
