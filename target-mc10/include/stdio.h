#ifndef _STDIO_H
#define _STDIO_H

#include <stdint.h>

/* We don't really have stdio just a couple of helpers for now */

struct __file {
    uint8_t dummy;
};

typedef struct __file FILE;
typedef struct __file DIR;

extern int getchar(void);
extern int putchar(int);
extern int puts(const char *);

extern int gets(char *);

#endif
