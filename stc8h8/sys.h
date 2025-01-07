#include "STC8H.h"
#include <intrins.h>
#include <string.h>
#include "display.h"
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
	//uint64_t key_timers[8];
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
	return time.tv_sec * 1000 + time.tv_usec;
}

sbit KEY_POWER   =  P1^5; //开关机键
sbit POWER_EN    =  P1^3; //开机使能引脚

#define KEY_U   0x80 //上
#define KEY_L   0x40 //左S
#define KEY_D   0x20 //下
#define KEY_R   0x10 //右
#define KEY_M   0x08 //音乐
#define KEY_S   0x04 //开始/停止
#define KEY_RST 0x02 //重置
#define KEY_G   0x01 //游戏选项

unsigned char key_down_bit;//记录当前扫描到的按下动作的按键
unsigned char key_down_t[8];//记录每一个按键当前已按下的时长


static void sys_init(sysctx_t *sys) {

	memset(sys, 0, sizeof(*sys));

	{
		uint64_t time = get_time_usec();
		//int i;
		//for (i = 0; i < 7; i++) sys->key_timers[i] = time;
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
		/*
		if (sys->old_rows[i] != a) {
			char buf[20], *d = buf;
			sys->old_rows[i] = a;
			for (j = 0; j < 10; j++, a <<= 1, d += 2) {
				if (a & 0x200) d[0] = '[', d[1] = ']';
				else d[0] = ' ', d[1] = ' ';
			}
//			printf("\33[%u;2H%.20s", i + 4, buf);
		}
		*/
		if (sys->old_rows[i] != a) { 
			// 更新当前行的状态
			sys->old_rows[i] = a;
			
			// 遍历当前列的每个位置（10个位置）
			for (j = 0; j < 10; j++, a <<= 1) {
				if (a & 0x200) {
					if(i<8 && i>0){
					// 如果当前位是 1，存储 bit = 1
						Screen_Buff[j][0] |= (1 << i);
					}else if(i<12 && i>8){
						Screen_Buff[j][1] |= (1 << (i-8));
					}else if(i<20 && i>12){
						Screen_Buff[j][2] |= (1 << (i-12));
					}
				} else {
					if(i<8 && i>0){
					// 如果当前位是 0，存储 bit = 0
						Screen_Buff[j][0] &= ~(1 << i);
					}else if(i<12 && i>8){
						Screen_Buff[j][1] &= ~(1 << (i-8));
					}else if(i<20 && i>12){
						Screen_Buff[j][2] &= ~(1 << (i-12));
					}
				}
			}
		}
	}
//	printf("\33[H\n"); // refresh screen
}

static int sys_events(sysctx_t *sys) {
/*
	uint64_t time = get_time_usec();
	unsigned hold_time = sys->hold_time;
	int i;
	for (i = 0; i < 8; i++)
		if (time - sys->key_timers[i] > hold_time)
			sys->keys &= ~(1 << i);

#define SET_KEY(key) do { \
	sys->keys |= 1 << key; \
	sys->key_timers[key] = time; \
} while (0)
*/
#define SET_KEY(key) do { \
	sys->keys |= 1 << key; \
} while (0)
	static unsigned char key_last_status = 0xFF;
	static unsigned char key_now_status;
	
	key_now_status = P1&0x03;
	key_now_status |= P3&0xFC;

	key_down_bit = (key_now_status ^ key_last_status)&(~key_now_status);
	
	if(key_now_status & KEY_G) key_down_t[0] = 0;
	else key_down_t[0]++;
	
	if(key_now_status & KEY_RST) key_down_t[1] = 0;
	else key_down_t[1]++;
	
	if(key_now_status & KEY_S) key_down_t[2] = 0;
	else key_down_t[2]++;
	
	if(key_now_status & KEY_M) key_down_t[3] = 0;
	else key_down_t[3]++;
	
	if(key_now_status & KEY_R) key_down_t[4] = 0;
	else key_down_t[4]++;
	
	if(key_now_status & KEY_D) key_down_t[5] = 0;
	else key_down_t[5]++;
	
	if(key_now_status & KEY_L) key_down_t[6] = 0;
	else key_down_t[6]++;
	
	if(key_now_status & KEY_U) key_down_t[7] = 0;
	else key_down_t[7]++;
	
	/*
		case 'w': key = 0; break; // w = up
		case 'a': key = 3; break; // a = left
		case 's': key = 1; break; // s = down
		case 'd': key = 2; break; // d = right
		case 'p': key = 4; break; // p = start/pause
		case 'm': key = 5; break; // m = mute
		case 'r': key = 6; break; // r = on/off
	*/
	if(key_down_bit & KEY_G){
		SET_KEY(4);
	}
	
	if(key_down_bit & KEY_RST){
		SET_KEY(6);
	}
	
	if(key_down_bit & KEY_S){
		SET_KEY(4);
	}
	
	if(key_down_bit & KEY_M){
		SET_KEY(5);
	}
	
	if(key_down_bit & KEY_R){
		SET_KEY(2);
	}
	
	if(key_down_bit & KEY_D){
		SET_KEY(1);
	}
	
	if(key_down_bit & KEY_L){
		SET_KEY(3);
	}
	
	if(key_down_bit & KEY_U){
		SET_KEY(0);
	}

	key_last_status = key_now_status;
	
	if(KEY_POWER == 1){while(KEY_POWER == 1);POWER_EN = 0; }; //关机

#undef SET_KEY
	return sys->keys;
}

#endif
