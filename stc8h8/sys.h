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

extern int mute = 1;

extern int rotate = 1;

extern int pause = 1;
int muteflags = 0;

int firstscore = 0;
int secondscore = 0;
int thirdscore = 0;
int fourthscore = 0;

int level = 0;
int speed = 0;
// 左右键声音
code unsigned char KeyRL_Voice[] = { 0x30, 0x60, 0x00};

// 变换键声音
code unsigned char KeyG_Voice[] = { 0x60, 0x30, 0x00};

//5 234 321 135 432 2345 311
code unsigned char Start_Voice[] = { 0x73, 0x41, 0x51, 0x63, 0x51, 0x41,0x33,0x31,0x51,0x73,0x61,0x51,0x43};

typedef struct {
	unsigned hold_time, sleep_ticks, sleep_delay, timer_inc;
	uint32_t misc;
	uint32_t keys;
	uint32_t old_speed;
	uint32_t old_next;
	uint32_t old_level;
	uint32_t old_score;
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

void Delay10us()		//@30.000MHz,8.6uS
;
void put_char(unsigned char c) //baud 115200
;

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


void score_draw(int i,int j){
	Screen_Buff[i][3] |= (1 << j);
}

void score_undraw(int i,int j){
	Screen_Buff[i][3] &= ~(1 << j);
}


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
					if(i<8){
					// 如果当前位是 1，存储 bit = 1
						Screen_Buff[j][0] |= (1 << i);
					}else if(i<12){
						Screen_Buff[j][1] |= (1 << (i-8));
					}else{
						Screen_Buff[j][2] |= (1 << (i-12));
					}
				} else {
					if(i<8){
					// 如果当前位是 0，存储 bit = 0
						Screen_Buff[j][0] &= ~(1 << i);
					}else if(i<12){
						Screen_Buff[j][1] &= ~(1 << (i-8));
					}else{
						Screen_Buff[j][2] &= ~(1 << (i-12));
					}
				}
			}
		}
	}
	// update next
	{
		int a = mem[184] | mem[186] << 4 | mem[188] << 8 | mem[190] << 12;
		int diff = a ^ sys->old_next;
		if (diff) {
			sys->old_next = a;
			//printf("0x%x\n",a);
			score_undraw(8,0);score_undraw(8,1);score_undraw(8,2);score_undraw(8,3);
			score_undraw(9,0);score_undraw(9,1);score_undraw(9,2);score_undraw(9,3);
			if(a == 0xf0 || a == 0x1111){
				score_draw(8,0);score_draw(8,1);score_draw(8,2);score_draw(8,3);
			}else
			if(a == 0x2640 || a == 0x5600){
				score_draw(8,1);score_draw(9,1);score_draw(9,2);score_draw(9,3);
			}else
			if(a == 0x4620 || a == 0x6500){
				score_draw(8,2);score_draw(9,2);score_draw(9,1);score_draw(9,0);
			}else
			if(a == 0x550){
				score_draw(9,1);score_draw(8,1);score_draw(8,2);score_draw(9,2);
			}else
			if(a == 0x1700 || a == 0x7200 || a == 0x6440 || a == 0x2260){
				score_draw(8,2);score_draw(8,1);score_draw(9,2);score_draw(9,3);
			}else
			if(a == 0x7100 || a == 0x2700 || a == 0x4460 || a == 0x6220){
				score_draw(8,2);score_draw(8,1);score_draw(9,1);score_draw(9,0);
			}else
			if(a == 0x7400 || a == 0x2620 || a == 0x4640 || a == 0x4700){
				score_draw(8,1);score_draw(9,1);score_draw(9,2);score_draw(9,0);
			}
		}
	}
	{
		int a;
		static const uint16_t digit1[] = {
			0x8c8c, 0x0880, 0x84c8, 0x88c8, 0x08c4,
			0x884c, 0x8c4c, 0x0888, 0x8ccc, 0x88cc };
		// update speed
		a = mem[196] | mem[198] << 4 | mem[200] << 8 | mem[202] << 12;
		a &= 0x8ccc;
		if (a != sys->old_speed) {
			sys->old_speed = a;
			for (j = 0; j < 10; j++) if (a == digit1[j]) break;
			speed = j < 10 ? j : j % 10;
		}
		// update level
		a = mem[204] | mem[206] << 4 | mem[208] << 8 | mem[210] << 12;
		a &= 0x8ccc;
		
		
		if (a != sys->old_level) {
			sys->old_level = a;
			for (j = 0; j < 10; j++) if (a == digit1[j]) break;
			level = j < 10 ? j : j % 10;
		}
	}
	// update score
	{
		/*
		int buf[4]; uint32_t a;
		static const uint8_t digit4[] = {
			0xe7, 0xa0, 0xcb, 0xe9, 0xac, 0x6d, 0x6f, 0xe0, 0xef, 0xed };
		a  = (mem[179] | mem[199] << 4) << 24;
		a |= (mem[185] | mem[201] << 4) << 16;
		a |= (mem[189] | mem[187] << 4) << 8;
		a |=  mem[191] | mem[203] << 4;
		put_char(a);
		a &= 0xefefefef;
		if (a != sys->old_score) {
			sys->old_score = a;
			for (i = 0; i < 4; i++, a >>= 8) {
				int x = a & 0xff;
				for (j = 0; j < 10; j++) if (x == digit4[j]) break;
				buf[i] = j < 10 ? j : j % 10;
			}
		}
		*/
		fourthscore = mem[0x4F];
		thirdscore = mem[0x4E];
		secondscore = mem[0x4D];
		firstscore = mem[0x4C];
	}
	Screen_Buff[1][3] |= (1 << 4);
	Screen_Buff[6][3] |= (1 << 3);
	Screen_Buff[4][3] |= (1 << 3);
	Screen_Buff[0][3] |= (1 << 5);
	//Screen_Buff[1][3] |= (1 << 2);
	//put_char(mem[177]>>2 & 1);

	
	if(speed==1){
								score_undraw(8,5);
		score_undraw(7,5);			score_draw(6,5);
								score_undraw(5,5);
		score_undraw(4,5);			score_draw(3,5);
								score_undraw(2,5);
		
	}else 
	if(speed==2){
								score_draw(8,5);
		score_undraw(7,5);			score_draw(6,5);
								score_draw(5,5);
		score_draw(4,5);			score_undraw(3,5);
								score_draw(2,5);
	}else
	if(speed==3){
								score_draw(8,5);
		score_undraw(7,5);			score_draw(6,5);
								score_draw(5,5);
		score_undraw(4,5);			score_draw(3,5);
								score_draw(2,5);
	}else
	if(speed==4){
								score_undraw(8,5);
		score_draw(7,5);			score_draw(6,5);
								score_draw(5,5);
		score_undraw(4,5);			score_draw(3,5);
								score_undraw(2,5);
	}else
	if(speed==5){
								score_draw(8,5);
		score_draw(7,5);			score_undraw(6,5);
								score_draw(5,5);
		score_undraw(4,5);			score_draw(3,5);
								score_draw(2,5);
	}else
	if(speed==6){
								score_draw(8,5);
		score_draw(7,5);			score_undraw(6,5);
								score_draw(5,5);
		score_draw(4,5);			score_draw(3,5);
								score_draw(2,5);
	}else
	if(speed==7){
								score_draw(8,5);
		score_undraw(7,5);			score_draw(6,5);
								score_undraw(5,5);
		score_undraw(4,5);			score_draw(3,5);
								score_undraw(2,5);	
	}else
	if(speed==8){
								score_draw(8,5);
		score_draw(7,5);			score_draw(6,5);
								score_draw(5,5);
		score_draw(4,5);			score_draw(3,5);
								score_draw(2,5);
	}else
	if(speed==9){
								score_draw(8,5);
		score_draw(7,5);			score_draw(6,5);
								score_draw(5,5);
		score_undraw(4,5);			score_draw(3,5);
								score_draw(2,5);
	}else
	if(speed==0){
								score_draw(8,5);
		score_draw(7,5);			score_draw(6,5);
								score_undraw(5,5);
		score_draw(4,5);			score_draw(3,5);
								score_draw(2,5);
	}
	
	if(level==1){
								score_undraw(8,4);
		score_undraw(7,4);			score_draw(6,4);
								score_undraw(5,4);
		score_undraw(4,4);			score_draw(3,4);
								score_undraw(2,4);
		
	}else 
	if(level==2){
								score_draw(8,4);
		score_undraw(7,4);			score_draw(6,4);
								score_draw(5,4);
		score_draw(4,4);			score_undraw(3,4);
								score_draw(2,4);
	}else
	if(level==3){
								score_draw(8,4);
		score_undraw(7,4);			score_draw(6,4);
								score_draw(5,4);
		score_undraw(4,4);			score_draw(3,4);
								score_draw(2,4);
	}else
	if(level==4){
								score_undraw(8,4);
		score_draw(7,4);			score_draw(6,4);
								score_draw(5,4);
		score_undraw(4,4);			score_draw(3,4);
								score_undraw(2,4);
	}else
	if(level==5){
								score_draw(8,4);
		score_draw(7,4);			score_undraw(6,4);
								score_draw(5,4);
		score_undraw(4,4);			score_draw(3,4);
								score_draw(2,4);
	}else
	if(level==6){
								score_draw(8,4);
		score_draw(7,4);			score_undraw(6,4);
								score_draw(5,4);
		score_draw(4,4);			score_draw(3,4);
								score_draw(2,4);
	}else
	if(level==7){
								score_draw(8,4);
		score_undraw(7,4);			score_draw(6,4);
								score_undraw(5,4);
		score_undraw(4,4);			score_draw(3,4);
								score_undraw(2,4);	
	}else
	if(level==8){
								score_draw(8,4);
		score_draw(7,4);			score_draw(6,4);
								score_draw(5,4);
		score_draw(4,4);			score_draw(3,4);
								score_draw(2,4);
	}else
	if(level==9){
								score_draw(8,4);
		score_draw(7,4);			score_draw(6,4);
								score_draw(5,4);
		score_undraw(4,4);			score_draw(3,4);
								score_draw(2,4);
	}else
	if(level==0){
								score_draw(8,4);
		score_draw(7,4);			score_draw(6,4);
								score_undraw(5,4);
		score_draw(4,4);			score_draw(3,4);
								score_draw(2,4);
	}
	
	
	
	
	if(firstscore==1){
		/*
		Screen_Buff[0][3] |= (1 << 0);
		Screen_Buff[0][3] |= (1 << 2);
		
		Screen_Buff[1][3] &= ~(1 << 0);
		Screen_Buff[0][3] &= ~(1 << 1);
		Screen_Buff[1][3] &= ~(1 << 3);
		Screen_Buff[1][3] &= ~(1 << 1);
		Screen_Buff[1][3] &= ~(1 << 2);
		*/
		score_draw(0,0);
		score_draw(0,2);
		score_undraw(1,0);
		score_undraw(0,1);
		score_undraw(1,3);
		score_undraw(1,1);
		score_undraw(1,2);
		
	}else 
	if(firstscore==2){
	/*
		Screen_Buff[0][3] |= (1 << 0);
		Screen_Buff[1][3] |= (1 << 0);
		
		Screen_Buff[0][3] |= (1 << 1);
		Screen_Buff[1][3] |= (1 << 2);
		Screen_Buff[1][3] |= (1 << 3);
		
		Screen_Buff[1][3] &= ~(1 << 1);
		Screen_Buff[0][3] &= ~(1 << 2);
	*/
		
		score_draw(0,0);
		score_draw(1,0);
		score_draw(0,1);
		score_draw(1,2);
		score_draw(1,3);
		score_undraw(1,1);
		score_undraw(0,2);
	}else
	if(firstscore==3){
								score_draw(1,0);
		score_undraw(1,1);			score_draw(0,0);
								score_draw(0,1);
		score_undraw(1,2);			score_draw(0,2);
								score_draw(1,3);
	}else
	if(firstscore==4){
								score_undraw(1,0);
		score_draw(1,1);			score_draw(0,0);
								score_draw(0,1);
		score_undraw(1,2);			score_draw(0,2);
								score_undraw(1,3);
	}else
	if(firstscore==5){
								score_draw(1,0);
		score_draw(1,1);			score_undraw(0,0);
								score_draw(0,1);
		score_undraw(1,2);			score_draw(0,2);
								score_draw(1,3);
	}else
	if(firstscore==6){
								score_draw(1,0);
		score_draw(1,1);			score_undraw(0,0);
								score_draw(0,1);
		score_draw(1,2);			score_draw(0,2);
								score_draw(1,3);
	}else
	if(firstscore==7){
								score_draw(1,0);
		score_undraw(1,1);			score_draw(0,0);
								score_undraw(0,1);
		score_undraw(1,2);			score_draw(0,2);
								score_undraw(1,3);	
	}else
	if(firstscore==8){
								score_draw(1,0);
		score_draw(1,1);			score_draw(0,0);
								score_draw(0,1);
		score_draw(1,2);			score_draw(0,2);
								score_draw(1,3);	
	}else
	if(firstscore==9){
								score_draw(1,0);
		score_draw(1,1);			score_draw(0,0);
								score_draw(0,1);
		score_undraw(1,2);			score_draw(0,2);
								score_draw(1,3);
	}else
	{
								score_draw(1,0);
		score_draw(1,1);			score_draw(0,0);
								score_undraw(0,1);
		score_draw(1,2);			score_draw(0,2);
								score_draw(1,3);
	}
	
	if(secondscore==1){
								score_undraw(3,0);
		score_undraw(3,1);			score_draw(2,0);
								score_undraw(2,1);
		score_undraw(3,2);			score_draw(2,2);
								score_undraw(3,3);
		
	}else 
	if(secondscore==2){
								score_draw(3,0);
		score_undraw(3,1);			score_draw(2,0);
								score_draw(2,1);
		score_draw(3,2);			score_undraw(2,2);
								score_draw(3,3);
	}else
	if(secondscore==3){
								score_draw(3,0);
		score_undraw(3,1);			score_draw(2,0);
								score_draw(2,1);
		score_undraw(3,2);			score_draw(2,2);
								score_draw(3,3);
	}else
	if(secondscore==4){
								score_undraw(3,0);
		score_draw(3,1);			score_draw(2,0);
								score_draw(2,1);
		score_undraw(3,2);			score_draw(2,2);
								score_undraw(3,3);
	}else
	if(secondscore==5){
								score_draw(3,0);
		score_draw(3,1);			score_undraw(2,0);
								score_draw(2,1);
		score_undraw(3,2);			score_draw(2,2);
								score_draw(3,3);
	}else
	if(secondscore==6){
								score_draw(3,0);
		score_draw(3,1);			score_undraw(2,0);
								score_draw(2,1);
		score_draw(3,2);			score_draw(2,2);
								score_draw(3,3);
	}else
	if(secondscore==7){
								score_draw(3,0);
		score_undraw(3,1);			score_draw(2,0);
								score_undraw(2,1);
		score_undraw(3,2);			score_draw(2,2);
								score_undraw(3,3);	
	}else
	if(secondscore==8){
								score_draw(3,0);
		score_draw(3,1);			score_draw(2,0);
								score_draw(2,1);
		score_draw(3,2);			score_draw(2,2);
								score_draw(3,3);	
	}else
	if(secondscore==9){
								score_draw(3,0);
		score_draw(3,1);			score_draw(2,0);
								score_draw(2,1);
		score_undraw(3,2);			score_draw(2,2);
								score_draw(3,3);
	}else
	{
								score_draw(3,0);
		score_draw(3,1);			score_draw(2,0);
								score_undraw(2,1);
		score_draw(3,2);			score_draw(2,2);
								score_draw(3,3);
	}
	
	
	if(thirdscore==1){
								score_undraw(5,0);
		score_undraw(5,1);			score_draw(4,0);
								score_undraw(4,1);
		score_undraw(5,2);			score_draw(4,2);
								score_undraw(5,3);
		
	}else 
	if(thirdscore==2){
								score_draw(5,0);
		score_undraw(5,1);			score_draw(4,0);
								score_draw(4,1);
		score_draw(5,2);			score_undraw(4,2);
								score_draw(5,3);
	}else
	if(thirdscore==3){
								score_draw(5,0);
		score_undraw(5,1);			score_draw(4,0);
								score_draw(4,1);
		score_undraw(5,2);			score_draw(4,2);
								score_draw(5,3);
	}else
	if(thirdscore==4){
								score_undraw(5,0);
		score_draw(5,1);			score_draw(4,0);
								score_draw(4,1);
		score_undraw(5,2);			score_draw(4,2);
								score_undraw(5,3);
	}else
	if(thirdscore==5){
								score_draw(5,0);
		score_draw(5,1);			score_undraw(4,0);
								score_draw(4,1);
		score_undraw(5,2);			score_draw(4,2);
								score_draw(5,3);
	}else
	if(thirdscore==6){
								score_draw(5,0);
		score_draw(5,1);			score_undraw(4,0);
								score_draw(4,1);
		score_draw(5,2);			score_draw(4,2);
								score_draw(5,3);
	}else
	if(thirdscore==7){
								score_draw(5,0);
		score_undraw(5,1);			score_draw(4,0);
								score_undraw(4,1);
		score_undraw(5,2);			score_draw(4,2);
								score_undraw(5,3);	
	}else
	if(thirdscore==8){
								score_draw(5,0);
		score_draw(5,1);			score_draw(4,0);
								score_draw(4,1);
		score_draw(5,2);			score_draw(4,2);
								score_draw(5,3);	
	}else
	if(thirdscore==9){
								score_draw(5,0);
		score_draw(5,1);			score_draw(4,0);
								score_draw(4,1);
		score_undraw(5,2);			score_draw(4,2);
								score_draw(5,3);
	}else
	{
								score_draw(5,0);
		score_draw(5,1);			score_draw(4,0);
								score_undraw(4,1);
		score_draw(5,2);			score_draw(4,2);
								score_draw(5,3);
	}

	
	if(fourthscore==1){
								score_undraw(7,0);
		score_undraw(7,1);			score_draw(6,0);
								score_undraw(6,1);
		score_undraw(7,2);			score_draw(6,2);
								score_undraw(7,3);
		
	}else 
	if(fourthscore==2){
								score_draw(7,0);
		score_undraw(7,1);			score_draw(6,0);
								score_draw(6,1);
		score_draw(7,2);			score_undraw(6,2);
								score_draw(7,3);
	}else
	if(fourthscore==3){
								score_draw(7,0);
		score_undraw(7,1);			score_draw(6,0);
								score_draw(6,1);
		score_undraw(7,2);			score_draw(6,2);
								score_draw(7,3);
	}else
	if(fourthscore==4){
								score_undraw(7,0);
		score_draw(7,1);			score_draw(6,0);
								score_draw(6,1);
		score_undraw(7,2);			score_draw(6,2);
								score_undraw(7,3);
	}else
	if(fourthscore==5){
								score_draw(7,0);
		score_draw(7,1);			score_undraw(6,0);
								score_draw(6,1);
		score_undraw(7,2);			score_draw(6,2);
								score_draw(7,3);
	}else
	if(fourthscore==6){
								score_draw(7,0);
		score_draw(7,1);			score_undraw(6,0);
								score_draw(6,1);
		score_draw(7,2);			score_draw(6,2);
								score_draw(7,3);
	}else
	if(fourthscore==7){
								score_draw(7,0);
		score_undraw(7,1);			score_draw(6,0);
								score_undraw(6,1);
		score_undraw(7,2);			score_draw(6,2);
								score_undraw(7,3);	
	}else
	if(fourthscore==8){
								score_draw(7,0);
		score_draw(7,1);			score_draw(6,0);
								score_draw(6,1);
		score_draw(7,2);			score_draw(6,2);
								score_draw(7,3);	
	}else
	if(fourthscore==9){
								score_draw(7,0);
		score_draw(7,1);			score_draw(6,0);
								score_draw(6,1);
		score_undraw(7,2);			score_draw(6,2);
								score_draw(7,3);
	}else
	{
								score_draw(7,0);
		score_draw(7,1);			score_draw(6,0);
								score_undraw(6,1);
		score_draw(7,2);			score_draw(6,2);
								score_draw(7,3);
	}
	if(mem[183]==0x0A){
		Screen_Buff[1][3] |= (1 << 5);
	}else{
		Screen_Buff[1][3] &= ~(1 << 5);
	}
	if(mute){
		Screen_Buff[2][3] |= (1 << 3);
	}else
		Screen_Buff[2][3] &= ~(1 << 3);
	
	if(rotate){
		Screen_Buff[0][3] |= (1 << 4);
		Screen_Buff[0][3] &= ~(1 << 3);
	}else{
		Screen_Buff[0][3] |= (1 << 3);
		Screen_Buff[0][3] &= ~(1 << 4);
	}
//	printf("\33[H\n"); // refresh screen
}


unsigned char key_down_bit;//记录当前扫描到的按下动作的按键
unsigned char key_down_t[8];//记录每一个按键当前已按下的时长
void key_scan()
{
	static unsigned char key_last_status = 0xFF;
	static unsigned char key_now_status;
	
	key_now_status = P1&0x03;
	key_now_status |= P3&0xFC;

	key_down_bit = (key_now_status ^ key_last_status)&(~key_now_status);
	
	if(key_now_status & 0x01) key_down_t[0] = 0;
	else key_down_t[0]++;
	
	if(key_now_status & 0x02) key_down_t[1] = 0;
	else key_down_t[1]++;
	
	if(key_now_status & 0x04) key_down_t[2] = 0;
	else key_down_t[2]++;
	
	if(key_now_status & 0x08) key_down_t[3] = 0;
	else key_down_t[3]++;
	
	if(key_now_status & 0x10) key_down_t[4] = 0;
	else key_down_t[4]++;
	
	if(key_now_status & 0x20) key_down_t[5] = 0;
	else key_down_t[5]++;
	
	if(key_now_status & 0x40) key_down_t[6] = 0;
	else key_down_t[6]++;
	
	if(key_now_status & 0x80) key_down_t[7] = 0;
	else key_down_t[7]++;
	
	key_last_status = key_now_status;
	
	//put_char(key_now_status);
}

static int sys_events(sysctx_t *sys) {
	key_scan();
	sys->keys = 0;
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
#define SET_KEY(key) sys->keys |= 1 << key;
	
	/*
		case 'w': key = 0; break; // w = up
		case 'a': key = 3; break; // a = left
		case 's': key = 1; break; // s = down
		case 'd': key = 2; break; // d = right
		case 'p': key = 4; break; // p = start/pause
		case 'm': key = 5; break; // m = mute
		case 'r': key = 6; break; // r = on/off
	*/
	
	if(KEY_POWER == 1){while(KEY_POWER == 1);POWER_EN = 0; }; //关机

	if(key_down_t[0]){	//rotate
		//SET_KEY(4);
		if(mute) Voice_Play(KeyG_Voice);
		if(rotate) rotate=0;else rotate=1;
		return 0xfe;
	}else
	
	/*
	if(key_down_t[1] & KEY_RST){
		SET_KEY(6);
		return 0x0f;
	}else
	*/
	if(key_down_t[2]){	//start
		//SET_KEY(4);
		if(mute) Voice_Play(Start_Voice);
		if(pause==2) pause=0;else pause++;
		return 0xef;
	}else
	
	if(key_down_t[3]){	//mute
		//SET_KEY(5);
		//return 0xdf;
		if(mute) mute=0;else mute=1;
		return 0xdf;
	}else
	
	if(key_down_t[4]){
		//SET_KEY(2);
		if(mute) Voice_Play(KeyRL_Voice);
		return 0xfb;
	}else
	
	if(key_down_t[5]){
		//SET_KEY(1);
		//Voice_Init();
		if(mute) Voice_Play(KeyRL_Voice);
		return 0xfd;
	}else
	
	if(key_down_t[6]){
		//SET_KEY(3);
		if(mute) Voice_Play(KeyRL_Voice);
		return 0xf7;
	}else
	
	if(key_down_t[7]){
		//SET_KEY(0);
		if(mute) Voice_Play(KeyRL_Voice);
		return 0xfe;
	}

#undef SET_KEY
	return 0xff;
}

#endif
