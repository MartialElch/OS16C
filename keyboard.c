/******************************************************************************/
__asm__ (
	".code16gcc"
);

/******************************************************************************/
#include "include/types.h"
#include "include/keyboard.h"
#include "include/keymap.h"

void sys_puthex(uint8_t i);
void sys_putchar(char c);

/******************************************************************************/
int keyboard_readpos = 0;
int keyboard_writepos = 0;

static char keyboard_buffer[256];

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
	if (keyboard_writepos > keyboard_readpos) {
		return keyboard_buffer[keyboard_readpos++];
	} else {
		return 0;
	}
}

/******************************************************************************/
/* keyboard */
void keyboard_addbuffer(uint8_t c) {
	if (!(c&0x80)) {			/* key press */
		if (keymap[c] != '\0') {
			keyboard_buffer[keyboard_writepos++] = keymap[c];
			sys_putchar(keymap[c]);
		} else {
			sys_puthex(c);
		}
	}

	return;
}

void keyboard_handler(void) {
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

	keyboard_addbuffer(c);

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
