/******************************************************************************/
__asm__ (
	".code16gcc"
);

/******************************************************************************/
#include "include/types.h"
#include "include/fat.h"
#include "include/keyboard.h"
#include "include/shell.h"

extern int keyboard_readpos;
extern int keyboard_writepos;

static void syscall_09_write(uint16_t, uint16_t);
static void syscall_4C_exit(void);

/******************************************************************************/
static void halt(void) {
	__asm__ volatile (
		"cli\n"
		"hlt"
		: /* no output */
		: /* no input */
		: /* no globber */);
}

static void enable_irq(void) {
	__asm__ volatile (
		"sti"
		: /* no output */
		: /* no input */
		: /* no globber */);
}

/******************************************************************************/
void sys_putchar(char c) {
	__asm__ volatile (
		"    mov  %0, %%al\n"
		"    mov  $0x0e, %%ah\n"
		"    int  $0x10"
		: /* no output */
		: "d" (c)
		: "ax");

	return;
}

void sys_puthex(uint8_t i) {
	uint8_t c, d;

	d = i;
	d = d >> 4;
	d = d & 0x0f;
	if (d < 10) {
		c = 0x0e00| (d+0x30);
	} else {
		c = 0x0e00| (d+0x41-10);
	}
	sys_putchar(c);

	d = i;
	d = d & 0x0f;
	if (d < 10) {
		c = (0x0e00| (d+0x30));
	} else {
		c = (0x0e00| (d+0x41-10));
	}
	sys_putchar(c);

	return;
}

void sys_putdec(uint32_t n) {
	char c;
	uint32_t d = 0;

	if (n == 0) {
		sys_putchar('0');
	}

	while (n > 0) {
		d = d*10;
		d = d + n%10;
		n = n/10;
	}

	while (d > 0) {
		c = d%10 + 0x30;
		sys_putchar(c);
		d = d/10;
	}

	return;
}

static void sys_print(char *s) {
	char *c;

	while (*(c = s++) != 0) {
		sys_putchar(*c);
	}

	return;
}

static void floppy_reset(void) {
	__asm__ volatile (				/* reset floppy controller */
		"    mov  $0x00, %%ah\n"
		"    int  $0x13"
		: /* no output */
		: /* no input */
		: "%ax", "%dx");

	return;
}

static void floppy_read(char *buffer, int sector, int track, int head, int drive) {
	int ax, cx, dx;
	int command, ndata;

	command = 2;
	ndata = 1;

	ax = (command << 8) | ndata;
	cx = (track << 8) | sector;
	dx = (head << 8) | drive;

	__asm__ volatile (
		"    int  $0x13"
		: /* no output */
		: "b"(buffer), "a"(ax), "c"(cx), "d"(dx)
		: "memory");

	return;
}

char *strncpy(char *dest, const char * src, int n) {
	int i;
	for (i=0; i<n; i++) {
		dest[i] = src[i];
	}
	return dest;
}

char *itoa(char *dest, int src) {
	int i, n;

	if (src == 0) {
		dest[0] = '0';
		dest[1] = 0;
	} else {
		n=0;
		dest[n] = 0;

		while (src > 0) {
			for (i=++n; i>0; i--) {
				dest[i] = dest[i-1];
			}
			dest[0] = (src%10)+0x30;
			src = src/10;
		}
	}

	return dest;
}

void dir(char *buffer) {
	struct FAT_ENTRY *entry;
	static char s[32];

	int n, b, used;

	n = 0;
	used = 0;

	entry = (struct FAT_ENTRY *)buffer;
	while (entry->attr != 0) {
		if (entry->attr & ATTR_VOLUMELABEL) {
			sys_print("\r\n");
			sys_print(" Volume in drive A is ");
			strncpy(s, (char*)entry->filename, 8);
			s[8] = 0;
			sys_print(s);
			sys_print("\r\n");
			sys_print(" Volume Serial Number is ");
			/* printf("%04X-%04X", (bsext.serial & 0xffff0000)/0x10000, bsext.serial & 0xffff); */
			sys_print("\r\n");
			sys_print(" Directory of A:\\");
			sys_print("\r\n");
			sys_print("\r\n");
		} else if (entry->attr & ATTR_ARCHIVE) {
			strncpy(s, (char*)entry->filename, 8);
			s[8] = 0;
			sys_print(s);
			sys_print(" ");
			strncpy(s, (char*)entry->filename+8, 3);
			s[3] = 0;
			sys_print(s);
			sys_print("            ");
			b = entry->size;							/* file size */
			used = used + b;
			sys_print(itoa(s, b));
			b = entry->mdate & 0x001f;					/* day */
			sys_print(" ");
			sys_print(itoa(s, b));
														/* month */
														/* year */
			sys_print("\r\n");

			/* printf("%13d ", entry.size); */
			/* printf("%02d.%02d.%02d   ", entry.mdate & 0x001f, (entry.mdate & 0x1e0) / 32, (80 + (entry.mdate & 0xf700) / 512)%100); */
			/* printf("%2d:%02d", (entry.mtime & 0xf800) / 2048, (entry.mtime & 0x07e0) / 32); */

			n++;
		}

		buffer = buffer + 32;
		entry = (struct FAT_ENTRY *)buffer;
	}

	sys_print("        ");
	sys_print(itoa(s, n));
	sys_print(" files");
	sys_print("        ");
	sys_print(itoa(s, used));
	sys_print(" bytes\r\n");

	sys_print("        ");
	sys_print(" bytes free\r\n");


	return;
}

void sys_halt(void) {
	sys_print("system halted\n\r");
	halt();
	return;
}

void sys_syscall(void) {
	uint16_t ds;
	uint32_t nr, edx;
	int irq = 0x21;

	__asm__ volatile (
		""
		: "=a"(nr), "=d"(edx)
		: /* no input */
		: /* no globber */);
	/* sys_puthex((edx >> 8) & 0xff); */
	/* sys_puthex(edx & 0xff); */

	__asm__ volatile (
		"    shl $2, %%bx\n"
		"    mov %%ax, %%gs\n"
		"    mov %%ds, %%ax\n"
		"    mov %%gs:2(%%bx), %%ds"
		: "=a"(ds)
		: "ax"(0), "bx"(irq)
		: /* no globber */);

	sys_print("System Call\n\r");
	sys_puthex((nr >> 8) & 0xff);
	sys_puthex(nr & 0xff);
	sys_print("\n\r");

	if ((nr & 0xff00) == 0x0900) {
		syscall_09_write(ds-0x1000, (uint16_t)(edx & 0xffff));
	} else if ((nr & 0xff00) == 0x4c00) {
		syscall_4C_exit();
	}

	__asm__ volatile (
		"    mov %%ax, %%ds\n"
		"    mov $0x41, %%al\n"
		"    mov $0x0e, %%ah\n"
		"    int $0x10\n"
		"    mov  $0x20, %%al\n"
		"    out  %%al, $0x20\n"
		"    leave\n"
		"    iretw"
		: /* no output */
		: "ax"(ds)
		: /* no globber */);

	return;
}

void registerinterrupt(int irq, void* handler) {
	sys_print("Register Interrupt Handler\n\r");

	__asm__ volatile (
		"    cli\n"
		"    shl $2, %%bx\n"
		"    xor %%ax, %%ax\n"
		"    mov %%gs, %%ax\n"
		"    mov %%cx, %%gs:(%%bx)\n"
		"    mov %%ds, %%gs:2(%%bx)\n"
		"    sti"
		: /* no output */
		: "bx"(irq), "cx"(handler)
		: "%ax");

	return;
}

void test_syscall(void) {
	sys_print("Execute System Call\n\r");

	__asm__ volatile (
		"    int $0x21"
		: /* no output */
		: /* no intput */
		: /* no globber */);

	return;
}

int strcmp(const char *s1, const char*s2) {
	char c;
	int rc = 0;
	int i=0;

	while((c = s1[i]) != '\0') {
		if (s2[i] == '\0') {
			rc = -1;
			break;
		} else if (s2[i] != c) {
			rc = -1;
			break;
		}
		i++;
	}

	if (s2[i] != '\0') {
		rc = 1;
	}

	return rc;
}

void sys_load(uint32_t start, uint32_t size) {
	static char buffer[512];
	volatile unsigned char *mem = (unsigned char *)0x4100;
	sys_print("load\n\r");
	int addr;
	int i;

	int sector, track, head, drive;

	sys_print("start = ");
	sys_putdec(start);
	sys_print("\n\r");
	sys_print("size  = ");
	sys_putdec(size);
	sys_print("\n\r");

	addr = (1+9+9)*512 + 32*224 + (start-2)*512;
	sys_print("@  = ");
	sys_puthex((addr >> 8) & 0xff);
	sys_puthex(addr & 0xff);
	sys_print("\n\r");

	sector = (addr/512)%18 + 1;		/* sector, numbering starts with 1 */
	track  = (addr/512)/(2*18);		/* track, alternating between heads */
	head   = ((addr/512)/18)%2;
	drive  = 1;						/* drive B: */
	sys_putdec(sector);
	sys_print(":");
	sys_putdec(track);
	sys_print(":");
	sys_putdec(head);
	sys_print("\n\r");

	floppy_reset();
	floppy_read(buffer, sector, track, head, drive);

	for (i=0; i<32; i++) {
		mem[i] = buffer[i];
	}

	return;
}

void sys_start(void) {
	/* uint16_t addr; */

	sys_print("start\n\r");

	/* addr = 0x4000; */

	__asm__ volatile (
		"    mov  $0x1400, %%eax\n"
		"    mov  %%ax, %%ds\n"
		"    pushl %%eax\n"
		"    mov  $0x0100, %%eax\n"
		"    pushl %%eax\n"
		"    retf"
		: /* no output */
		: /* no input */
		: "%eax" );

	return;
}

void sys_exec(char *prg) {
	static char buffer[512];
	char *p;
	struct FAT_ENTRY *entry;
	static char s[32];

	floppy_reset();
	floppy_read(buffer, 2, 0, 1, 1);

	sys_print("execute ");
	sys_print(prg);
	sys_print("\n\r");

	p = buffer;
	entry = (struct FAT_ENTRY *)p;
	while (entry->attr != 0) {
		if (entry->attr & ATTR_ARCHIVE) {
			strncpy(s, (char*)entry->filename, 11);
			s[11] = '\0';
			sys_print(s);
			sys_print("\n\r");

			if (strcmp(prg, s) == 0) {
				sys_load((entry->starthi) << 16 | entry->start, entry->size);
				sys_start();
				break;
			}
		}
		p = p + 32;
		entry = (struct FAT_ENTRY *)p;
	}

	return;
}

/******************************************************************************/
/* Kernel */

void kmain(void) {
	static char buffer[512];

	sys_print("Start Kernel\n\r");
	floppy_reset();
	floppy_read(buffer, 2, 0, 1, 1);

	registerinterrupt(9, keyboard_handler);
	registerinterrupt(0x21, sys_syscall);

/*
	dir(buffer);
	sys_exec("09WRITE COM");

	m = 0;
	for (i=m; i<(m+0x10); i++) {
		sys_puthex(mem[i]);
		sys_print(" ");
	}
	sys_print("\n\r");

	m = 0x1400;
	for (i=m; i<(m+0x10); i++) {
		sys_puthex(mem[i]);
		sys_print(" ");
	}
	sys_print("\n\r");

	m = 0x4100;
	for (i=m; i<(m+0x10); i++) {
		sys_puthex(mem[i]);
		sys_print(" ");
	}
	sys_print("\n\r");
*/

	shell();
	sys_print("Stop Kernel\n\r");

	return;
}

/******************************************************************************/
/* System Calls */

/* 09 - WRITE STRING TO STANDARD OUTPUT */
static void syscall_09_write(uint16_t ds, uint16_t dx) {
	char *p;
	char s;

	p = (char*)(uint16_t*)((ds << 4) + dx);
	while ((s=*p) != '$') {
		sys_putchar(s);
		p++;
	}

	return;
}

/* 4C - EXIT - TERMINATE WITH RETURN CODE */
static void syscall_4C_exit(void) {
	/* enable interrupts */

	/* should we worry about the stack ? */

	__asm__ volatile (
		"    mov  $0x20, %%al\n"
		"    out  %%al, $0x20\n"
		"    leave"
		: /* no output */
		: /* no input */
		: /* no globber */);

	/* reenable interrupts */
	enable_irq();

	/* return to shell */
	shell();

	return;
}

/******************************************************************************/
/* Shell */

void ls(void) {
	static char buffer[512];
	char *p;
	struct FAT_ENTRY *entry;
	static char s[32];
	int i;

	floppy_reset();
	floppy_read(buffer, 2, 0, 1, 1);

	p = buffer;
	entry = (struct FAT_ENTRY *)p;

	for (i=0; i<(0x10); i++) {
		sys_puthex(p[i]);
		sys_print(" ");
	}
	sys_print("\n\r");

	while (entry->attr != 0) {
		for (i=0; i<(0x10); i++) {
			sys_puthex(p[i]);
			sys_print(" ");
		}
		sys_print("\n\r");

		if (entry->attr & ATTR_ARCHIVE) {
			strncpy(s, (char*)entry->filename, 11);
			s[11] = '\0';
			sys_print(s);
			sys_print("\n\r");
		}
		p = p + 32;
		entry = (struct FAT_ENTRY *)p;
	}

	return;
}

void evaluate(char *cmd) {
	static char buffer[512];

	if (strcmp(cmd, "dir") == 0) {
		sys_print("CMD: dir\n\r");
		floppy_reset();
		floppy_read(buffer, 2, 0, 1, 1);
		dir(buffer);
	} else if (strcmp(cmd, "ls") == 0) {
		sys_print("CMD: ls\n\r");
		ls();
	} else if (strcmp(cmd, "halt") == 0) {
		sys_print("CMD: halt\n\r");
		sys_halt();
	} else if (strcmp(cmd, "start") == 0) {
		sys_print("CMD: start\n\r");
	} else {
		sys_print(cmd);
		sys_print(" not found\n\r");
	}

	return;
}

void shell(void) {
	static char s[80];
	char c;
	int i;

	i = 0;
	s[0] = '\0';

	sys_print("c:> ");
	while (1) {
		if (keyboard_writepos > keyboard_readpos) {
			c = sys_getchar();
			if (c == '\n') {
				sys_print("\r");
				evaluate(s);
				s[0] = '\0';
				i = 0;
				sys_print("c:> ");
			} else {
				s[i++] = c;
				s[i] = '\0';
			}
		}
	};

	return;
}

/******************************************************************************/
