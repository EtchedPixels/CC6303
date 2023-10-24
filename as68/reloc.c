/*
 *	Do relocation merging for Fuzix style binaries. The relocations
 *	are added to the end of data and the data extended and bss shrunk
 *	to match. If we extend too far we just grow the bss. That's a real
 *	corner case and a bit of extra bss won't matter (plus we brk back
 *	anyway after load).
 *
 *	TODO:
 *	ZP
 *	support for binaries with debug info
 *	Big endian support (host and target)
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

/* 16 byte header for current style binary. We try to reflect the general
   pattern of naming in classic Unixlike systems */

/* Do not change these values. They are (or will be) shared with asm stubs
   and the toolchain */
struct exec {
	uint16_t a_magic;
#define EXEC_MAGIC	0x80A8		/* Just need something to id with */
	uint8_t a_cpu;
	uint8_t a_cpufeat;
	uint8_t a_base;			/* Load address page */
	uint8_t a_hints;
	uint16_t a_text;
	uint16_t a_data;
	uint16_t a_bss;
	uint8_t a_entry;		/* Entry point - 0-255 bytes in only */
	/* These are kept in pages */
	uint8_t a_size;			/* Binary memory request 0 = all */
	uint8_t a_stack;		/* Stack size hint (not yet used) */
	uint8_t a_zp;			/* Zero/Direct page space required */

	/* This isn't really part of the header but a location fixed after
	   it */
	/* uint16_t a_sigvec; */
} __attribute((packed));

static struct exec hdr;

int main(int argc, char *argv[])
{
    uint8_t ar[3];
    uint16_t addr;
    uint16_t laddr;
    uint16_t raddr;
    int efd, rfd;
    static const uint8_t cff = 0xFF;
    uint8_t d;

    if (argc != 3) {
        fprintf(stderr, "%s binary relocs\n", argv[0]);
        exit(1);
    }
    efd = open(argv[1], O_RDWR);
    if (efd == -1) {
        perror(argv[1]);
        exit(1);
    }
    rfd = open(argv[2], O_RDONLY);
    if (rfd == -1) {
        perror(argv[2]);
        exit(1);
    }
    if (read(efd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
        fprintf(stderr, "%s: not valid.\n", argv[1]);
        exit(1);
    }
    raddr = hdr.a_text + hdr.a_data;		/* End of data */
    if (lseek(efd, raddr, SEEK_SET) < 0) {
        perror("lseek");
        exit(1);
    }
    printf("Text %x bytes data %x bytes bss %x bytes relocs at %x\n",
        hdr.a_text, hdr.a_data, hdr.a_bss, raddr);
    /* TODO: buffer output */
    if (hdr.a_zp) {
        laddr = 0x00;
        while(read(rfd, ar, 3) == 3) {
            if (ar[0])
                break;
            addr = (ar[1] << 8) | ar[2];
            if (addr < laddr) {
                fprintf(stderr, "Mis-sorted relocations.\n");
                exit(1);
            }
            while(laddr - addr > 254) {
                write(efd, &cff, 1);
                laddr += 254;
                raddr++;
            }
            d = addr - laddr;
            write(efd, &d, 1);
            raddr++;
        }
        d = 0;
        write(efd, &d, 1);
    }
    if (lseek(rfd, 0, SEEK_SET) < 0) {
        perror("lseek");
        exit(1);
    }
    laddr = 0x0000;
    /* Non ZP block */
    while(read(rfd, ar, 3) == 3) {
        if (ar[0] == 0)
            continue;
        addr = (ar[1] << 8) | ar[2];
        if (addr < laddr) {
            fprintf(stderr, "Mis-sorted relocations.\n");
            exit(1);
        }
        while(addr - laddr > 254) {
            write(efd, &cff, 1);
            laddr += 254;
            raddr++;
        }
        d = addr - laddr;
        write(efd, &d, 1);
        raddr++;
        laddr = addr;
    }
    d = 0;
    write(efd, &d, 1);
    raddr++;

    /* Now fix up the header */
    raddr -= hdr.a_text + hdr.a_data;
    printf("relocations occupied %x bytes\n", raddr);
    if (raddr <= hdr.a_bss) {
        /* Expansion fits (usual case) */
        hdr.a_bss -= raddr;
        hdr.a_data += raddr;
    } else {
        /* Need to grow */
        hdr.a_data += raddr;
        hdr.a_bss = 0;
    }
    if (lseek(efd, 0, SEEK_SET) < 0) {
        perror("lseek");
        exit(1);
    }
    if (write(efd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
        fprintf(stderr, "%s: unable to update header.\n", argv[2]);
        exit(1);
    }
    printf("Header adjusted to data %x bss %x\n", hdr.a_data,hdr.a_bss);
    close(efd);
    close(rfd);
    return 0;
}
