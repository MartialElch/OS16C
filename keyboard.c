/******************************************************************************/
__asm__ (
	".code16gcc"
);

/******************************************************************************/
#include "include/types.h"
#include "include/keymap.h"

void sys_puthex(uint8_t i);
void sys_putchar(char c);

/******************************************************************************/
int read_pos = 0;
int write_pos = 0;

static char key_buffer[256];

/******************************************************************************/
/* system */
static void outb(uint8_t v, uint16_t port) {
	__asm__ volatile (
		"outb %0, %1"
		: /* no output */
		: "a" (v), "dN" (port)
		: /* no globber */);
}

static uint8_t inb(uint16_t port) {
	uint8_t v;
	__asm__ volatile (
		"inb %1, %0"
		: "=a" (v)
		: "dN" (port)
		: /* no globber */);
	return v;
}

char sys_getchar(void) {
	if (write_pos > read_pos) {
		return key_buffer[read_pos++];
	} else {
		return 0;
	}
}

/******************************************************************************/
/* keyboard */
void bufferhandler(uint8_t c) {
	if (!(c&0x80)) {			/* key press */
		if (keymap[c] != '\0') {
			key_buffer[write_pos++] = keymap[c];
			sys_putchar(keymap[c]);
		} else {
			sys_puthex(c);
		}
	}

	return;
}

void keyboardhandler(void) {
	char a, c;

	__asm__ volatile (
		"    pusha"
		: /* no output */
		: /* no input */
		: /* no globber */);

	c = inb(0x60);
	a = inb(0x61);
	outb(a & 0x80, 0x61);
	outb(a, 0x61);
	outb(0x20, 0x20);

	bufferhandler(c);

	__asm__ volatile (
		"    popa\n"
		"    leave\n"
		"    iretw"
		: /* no output */
		: /* no input */
		: /* no globber */);

	return;
}

/******************************************************************************/
