#include <string.h>

#ifndef SYS_H
#define SYS_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;

extern long system_seconds;
extern long system_microseconds;

typedef struct {
	unsigned hold_time, sleep_ticks, sleep_delay, timer_inc;
	uint32_t misc;
	uint32_t keys;
	uint64_t key_timers[8];
	uint16_t old_rows[20];
} sysctx_t;

typedef struct {
	uint8_t mem[256]; uint16_t pc, stack;
	uint8_t a, r[5], cf, tmr, tf, timer_en;
} cpu_state_t;

struct timeval {
    long tv_sec;
    long tv_usec;
};
int gettimeofday(struct timeval *tv) {
    tv->tv_sec = system_seconds;
    tv->tv_usec = system_microseconds;
    return 0;
}
static uint64_t get_time_usec() {
	struct timeval time;
	gettimeofday(&time);
	return time.tv_sec * 1000000L + time.tv_usec;
}

static void sys_init(sysctx_t *sys) {

	memset(sys, 0, sizeof(*sys));

	{
		uint64_t time = get_time_usec();
		int i;
		for (i = 0; i < 7; i++) sys->key_timers[i] = time;
	}
/*
	printf("\33[2J\33[?25l"); // clear screen, hide cursor
	{
		int y = 3;
		printf("\33[%uH/--------------------\\", y++);
		for (; y <= 3 + 20; y++)
			printf("\33[%uH|                    |", y);
		printf("\33[%uH\\--------------------/", y);
		printf("\33[H\n"); // refresh screen
	}
*/
}

static void sys_redraw(sysctx_t *sys, uint8_t *mem) {	//need to rewrite
	int i, j;

	for (i = 0; i < 20; i++) {
		int a, x;
		a = mem[217 + i * 2] << 4 | mem[216 + i * 2];
		if ((unsigned)(i - 4) < 8) {
			x = mem[196 + i * 2 - 8] & 3;
			x = (x << 1 | x >> 1) & 3;
		} else {
			const uint8_t tab[20 * 2] = {
				192,3, 192,0, 192,2, 192,1,
				0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
				212,2, 212,0, 212,1, 212,3,
				213,0, 213,1, 213,3, 213,2
			};
			int off = tab[i * 2], bits = tab[i * 2 + 1];
			x = mem[off + 2] >> bits & 1;
			x = x << 1 | (mem[off] >> bits & 1);
		}
		a = a << 2 | x;
		if (sys->old_rows[i] != a) {
			char buf[20], *d = buf;
			sys->old_rows[i] = a;
			for (j = 0; j < 10; j++, a <<= 1, d += 2) {
				if (a & 0x200) d[0] = '[', d[1] = ']';
				else d[0] = ' ', d[1] = ' ';
			}
//			printf("\33[%u;2H%.20s", i + 4, buf);
		}
	}
//	printf("\33[H\n"); // refresh screen
}

static int sys_events(sysctx_t *sys) {	//need to rewrite
	uint64_t time = get_time_usec();
	unsigned hold_time = sys->hold_time * 1000;
	int i;
	for (i = 0; i < 8; i++)
		if (time - sys->key_timers[i] > hold_time)
			sys->keys &= ~(1 << i);

#define SET_KEY(key) do { \
	sys->keys |= 1 << key; \
	sys->key_timers[key] = time; \
} while (0)

	for (;;) {
		int a, n, status = 0;
		char buf[8];
//		n = read(0, &buf, sizeof(buf));

//		if (n != sizeof(buf)) {
//			if (status == 1) // escape = exit
//				sys->keys |= 1 << 16;
//			break;
//		}
	}
#undef SET_KEY
	return sys->keys;
}

#endif
