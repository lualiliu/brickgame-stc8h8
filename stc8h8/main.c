#include "STC8H.h"
#include <intrins.h>
#include "display.h"
#include "voice.h"

#include "sys.h"
#include "core.h"
#include "rom.h"

#define KEY_U   0x80 //上
#define KEY_L   0x40 //左
#define KEY_D   0x20 //下
#define KEY_R   0x10 //右
#define KEY_M   0x08 //音乐
#define KEY_S   0x04 //开始/停止
#define KEY_RST 0x02 //重置
#define KEY_G   0x01 //游戏选项

/**
 * @description: 51延时函数 @11.0592MHz
 * @param {uint ms 毫秒} 
 * @return: void
 */
void delay_ms(unsigned int ms)
{
   	unsigned char i, j;
		while(ms--)
		{
			_nop_();
			_nop_();
			_nop_();
			i = 30;
			j = 190;
			do
			{
				while (--j);
			} while (--i);
		}
}

void Pin_Init()
{
	P1M0 = 0x08;  //开关机及指示灯
	P1M1 = 0x20;
	KEY_POWER = 1; 
	POWER_EN = 1;
	
	P3M0 = 0x00; // 按键
	P3M1 = 0x00;
	P3 = 0xFF;
//	
	P5M0 = 0x10;
	P5M1 = 0;
}

void Delay10us()		//@30.000MHz，8.6uS
{
	unsigned char i;
	i = 85;
	while (--i);
}

void put_char(unsigned char c) //baud 115200
{
	P31 = 0;Delay10us();
	
	if(c & 0x01)P31 = 1; else P31 = 0;Delay10us();
	if(c & 0x02)P31 = 1; else P31 = 0;Delay10us();
	if(c & 0x04)P31 = 1; else P31 = 0;Delay10us();
	if(c & 0x08)P31 = 1; else P31 = 0;Delay10us();
	if(c & 0x10)P31 = 1; else P31 = 0;Delay10us();
	if(c & 0x20)P31 = 1; else P31 = 0;Delay10us();
	if(c & 0x40)P31 = 1; else P31 = 0;Delay10us();
	if(c & 0x80)P31 = 1; else P31 = 0;Delay10us();
	
	P31 = 1;Delay10us();
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
}

void Tetris_Loop(unsigned char a);
void Tetris_Init();
void Voice_Init(void);

void main()
{
	unsigned char n = 0 , i = 0 ,d = 0 ;
	unsigned int hold_time = 50;
	unsigned int sleep_ticks = 1000, sleep_delay = 1000;
	unsigned int timer_inc = 32;

	sysctx_t ctx;
	cpu_state_t cpu;
	
	P_SW2 |= 0x80;
	Pin_Init();
	Display_Init();
	Voice_Init();

	while(P15 == 1) //等待长按开机键松开
	{
		delay_ms(2);
	}
	//Tetris_Init(); //随机生成第一个方块
	
	timer_inc = timer_inc ? 0x10000 / timer_inc : 0x10000;
	if (timer_inc > 0x10000) timer_inc = 0x10000;
	
	memset(&cpu, 0, sizeof(cpu));

	memset(&ctx, 0, sizeof(ctx));
	
	sys_init(&ctx);

	ctx.hold_time = hold_time;
	ctx.sleep_ticks = sleep_ticks;
	ctx.sleep_delay = sleep_delay;
	ctx.timer_inc = timer_inc;
	
	//test_keys();

	run_game(E23PlusMarkII96in1_bin, &ctx, &cpu);

	//sys_close(&ctx);

	//Screen_Buff[5][3] = 0x8;
	
		//key_scan();
		//Tetris_Loop(key_down_bit);

//		
//		Screen_Buff[0][0] = 1 << n;
//		Screen_Buff[1][0] = 1 << i;
//		
//		if(key_down_bit & 0x40) //左移
//		{
//			n ++;
//		}
//		if(key_down_bit & 0x10)//右移
//		{
//			n --;
//		}
//		
//		if(key_down_bit & 0x20) //左移
//		{
//			i ++;
//		}
//		if(key_down_bit & 0x80)//右移
//		{
//			i --;
//		}
//		
//		Screen_Buff[n][3] = 1 << i;
	
}