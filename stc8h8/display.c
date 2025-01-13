#include "STC8H.h"
#include <intrins.h>

unsigned char random_number = 0;
	

//显存数据
unsigned char Screen_Buff[10][4] = { 0 };

static void Timer_0_Init(void)		//2毫秒@30.000MHz
{
	AUXR |= 0x80;		//定时器时钟1T模式
	TMOD &= 0xF0;		//设置定时器模式
	TL0 = 0xA0;		//设置定时初始值
	TH0 = 0x15;		//设置定时初始值
	//TL0 = 0xA0;		//设置定时初始值
	//TH0 = 0x15;		//设置定时初始值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	
	ET0 = 1;
	EA = 1;
}

void Display_Init()
{
	Timer_0_Init();		
}