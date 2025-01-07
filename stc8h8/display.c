/*****************************************************
*文件功能：实现LCD屏幕的刷新
*编写时间：2022.12.29
*文件作者：鹏老师
*******************************************************/
#include "STC8H.h"
#include <intrins.h>

unsigned char random_number = 0;
	
const unsigned char com_data[10] = {0xFE, 0xFD, 0xFB,0xF7,0xEF,0xDF,0xBF,0x7F,0xBF,0x7F};
static unsigned char com_num = 0;//记录当刷新到第几个COM
static unsigned char com_status = 0;//记录当前刷新的COM状态(高或者第)
long system_seconds = 0;    // 系统时间的秒部分
long system_microseconds = 0; // 系统时间的微秒部分

//显存数据
unsigned char Screen_Buff[10][4] = { 0 };

static void Timer_0_Init(void)		//2毫秒@30.000MHz
{
	AUXR |= 0x80;		//定时器时钟1T模式
	TMOD &= 0xF0;		//设置定时器模式
	TL0 = 0xA0;		//设置定时初始值
	TH0 = 0x15;		//设置定时初始值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	
	ET0 = 1;
	EA = 1;
}

void Timer0_Isr() interrupt 1
{
	random_number ++;//定时器产生伪随机数
	system_microseconds += 1;  // 1毫秒

  if (system_microseconds >= 1000) {
		system_microseconds -= 1000;
		system_seconds++;  // 如果微秒达到1000000，秒数增加1
	}
		
	if(com_status == 0) //选中的COM为低电平0V，未选中的COM为2.4V，Sige 高电平显示
	{ 
		P2M0 = 0xff; //P2 推挽
		P2M1 = 0;
		P2 = 0xff;
		
		P1M0 |= 0xC0; //P16 P17 推挽
		P1M1 &= 0x3F;
		P1 |= 0xC0;
		
		P7M1 = 0xff; //P7 开漏
		P7M0 = 0xff;
		P7 = 0xff;
		
		P4M1 |= 0xC0; //P4 开漏
		P4M0 |= 0xC0;
		P4 |= 0xC0;
		
		
		if(com_num < 8) //前8个Com，由P2和P7控制
		{
			P2 = com_data[com_num]; //将选中的COM设为低电平
			P7 = com_data[com_num];
		}
		else //后两个Com，由P1.6,P1.7以及P4.6，P4.7控制
		{
			P1 &= com_data[com_num];
			P4 &= com_data[com_num];
		}
		
		//需要显示的Sige为推挽输出态，并输出高电平，不需要显示的Sige为开漏态输出高电平（高阻）
		P6M0 = 0xFF;
		P6M1 = ~Screen_Buff[com_num][0];
		P6 = 0xFF;
		
		P5M0 |= 0x0F;
		P5M1 |= 0x0F;
		P5M1 &= ~(Screen_Buff[com_num][1]&0x0f);
		P5 |= 0x0F;
		
		P0M0 = 0xFF;
		P0M1 = ~Screen_Buff[com_num][2];
		P0 = 0xFF;
		
		P4M0 |= 0x3F;
		P4M1 |= 0x3F;
		P4M1 &= ~(Screen_Buff[com_num][3]&0x3F);
		P4 |= 0x3F;

		
		com_num++;
		if(com_num == 10)
		{
			com_num = 0;
			com_status = 1;
		}
	}
	else //选中的COM为高电平，未选中的COM为0.8V，Sige 低电平显示
	{
		P2M0 = 0xFF; //P2 开漏，输出高(高阻)
		P2M1 = 0xFF;
		P2 = 0xff;
		
		P1M0 |= 0xC0; //P16 P17 开漏，输出高(高阻)
		P1M1 |= 0xC0;
		P1 |= 0xC0;
		
		P7M0 = 0xff;  //P7 开漏，输出高(高阻)
		P7M1 =0xFF;
		P7 = 0xFF;
		
		P4M0 |= 0xC0; //P46 P47 开漏，输出高(高阻)
		P4M1 |= 0xC0;
		P4 |= 0xC0;
		
		if(com_num < 8) //前8个Com，由P2和P7控制
		{
			P7M1 = com_data[com_num];//对应COM设为推挽输出模式，输出高电平
		}
		else //后两个Com，由P1.6,P1.7以及P4.6，P4.7控制
		{
			P4M1 &= com_data[com_num];
		}
		
		//需要显示的Sige为低电平，不需要显示的Sige为高阻态
		P6M0 = 0xFF;
		P6M1 = 0xFF;
		P6 = ~Screen_Buff[com_num][0];
		
		P5M0 |= 0x0F;
		P5M1 |= 0x0F;
		P5|= 0x0F;
		P5 &= ~(Screen_Buff[com_num][1]&0x0f);
		
		P0M0 = 0xFF;
		P0M1 = 0xFF;
		P0 = ~Screen_Buff[com_num][2];
		
		P4M0 |= 0x3F;
		P4M1 |= 0x3F;
		P4 |= 0x3F;
		P4 &= ~(Screen_Buff[com_num][3]&0x3F);

		com_num++;
		if(com_num == 10) 
		{
			com_num = 0;
			com_status = 0;
		}
	}
}

void Display_Init()
{
	Timer_0_Init();		
}