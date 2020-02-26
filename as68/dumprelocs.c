#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/stat.h>

#include "obj.h"

static int nextbyte_eof(int fd)
{
    uint8_t c;
    int n = read(fd,&c, 1);
    if (n == 1)
        return c;
    if (n == -1) {
        perror("read");
        return 0xFF;
    }
    return -1;
}

static int nextbyte(int fd)
{
    int n = nextbyte_eof(fd);
    if (n == -1)
        fprintf(stderr, "Unexpected EOF in relocation stream.\n");
    return n;
}

static int bytect;
static char relbuf[20];
static char *relptr;

static void byte(uint8_t v)
{
    if (bytect == 0)
        printf("\t");
    printf("%02X ", v);
    bytect++;
    if (bytect == 16) {
        bytect = 0;
        printf("\n");
    }
}
    
static void reloc_tag(const char *p)
{
    *relptr++ = *p;
}

static void reloc_type(const char *p)
{
    memcpy(relbuf + 8, p, strlen(p));
}

static void reloc_seg(int n)
{
    relbuf[6] = "ACDBZXS7???????U"[n];
}

static void reloc_value(int fd, int size)
{
    int n;
    if (size == 1)
        n = nextbyte(fd);
    else if (size == 2){
        n = nextbyte(fd);
        n |= (nextbyte(fd) << 8);
    } else
        fprintf(stderr, "Invalid size %d.\n", size);
        
    sprintf(relbuf + 12, "%-5d", n);
}

static void reloc_init(void)
{
    memset(relbuf, ' ', 20);
    relbuf[20] = 0;
    relptr = relbuf;
}

static void reloc_end(void)
{
    if (bytect) {
        printf("\n");
        bytect = 0;
    }
    printf("%s\n", relbuf);
} 

static int dump_data(const char *p, int fd)
{
    int c;
    int size;
    bytect = 0;

    while((c = nextbyte_eof(fd)) != -1) {
        int high = 0;

        if (c != REL_ESC) {
            byte(c);
            continue;
        }
        c = nextbyte(fd);
        if (c == REL_REL) {
            byte(c);
            continue;
        }
        reloc_init();
        if (c == REL_EOF) {
            reloc_type("END");
            reloc_end();
            printf("\n\n");
            return 0;
        }
        /* Ok an actual relocation */
        if (c == REL_ORG) {
            reloc_type("ORG");
            reloc_value(fd, 2);
            reloc_end();
            continue;
        }
        if (c == REL_OVERFLOW) {
            reloc_tag("O");
            c = nextbyte(fd);
        }
        if (c == REL_HIGH) {
            high = 1;
            reloc_tag("H");
            c = nextbyte(fd);
        }
        size = ((c & S_SIZE) >> 4) + 1;
        if (c & REL_SIMPLE) {
            reloc_type("SEG");
            reloc_seg(c & S_SEGMENT);
            reloc_value(fd, size);
            reloc_end();
            continue;
        }
        switch(c & REL_TYPE) {
        case REL_PCREL:
            reloc_tag("R");
        case REL_SYMBOL:
            reloc_type("SYM");
            reloc_value(fd, 2);
            reloc_end();
            continue;  
        }         
        
    }
    fprintf(stderr, "%s: Unexpected EOF.\n", p);
    return 1;
}

static int process(const char *p)
{
    int fd = open(p, O_RDONLY);
    struct objhdr hdr;
    int err = 0;
    int i;

    if (fd == -1) {
        perror(p);
        return 1;
    }
    if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr) || hdr.o_magic != MAGIC_OBJ) {
        fprintf(stderr, "%s: not a valid object file.\n", p);
        close(fd);
        return 1;
    }
    for (i = 0; i < OSEG; i++) {
        printf("Segment %d:\n\tSize: %u\n\tOffset: %lu\n", i, hdr.o_size[i],
            (long)hdr.o_segbase[i]);
        if (i == 3) {	/* BSS */
            printf("\n\n");
            continue;
        }
        if (lseek(fd, hdr.o_segbase[i], SEEK_SET) < 0) {
            perror(p);
            continue;
        }
        err |= dump_data(p, fd);
    }
    close(fd);
    return err;
}

int main(int argc, char *argv[])
{
    int err = 0;
    while(--argc)
        err |= process(*++argv);
    return err;
}