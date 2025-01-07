/*****************************************************
*文件功能：实现LCD屏幕的刷新
*编写时间：2022.12.29
*文件作者：鹏老师
*******************************************************/
#include "STC8H.h"
#include <intrins.h>
#include "voice.h"

char * curr_play = 0; //当前播放的乐曲段
unsigned char buff_index = 0; //当前播放的乐曲段中的音符索引
unsigned char curr_note = 0;  //当前正在播放的音符的音高
unsigned int  curr_note_time = 0; //当前正在播放的音符的时值
unsigned int  note_time  = 0; //当前音符已经播放的时长(单位为音符的半周期)

//每个音符对应的定时器初值, 音符分别是 5,6,7,1,2,3,4,5,6,7,1,2,3,4,5,6 共16个音符
code unsigned char TL1_Table[16] = { 0x96, 0xE0, 0x68, 0xF8, 0x28, 0x0E, 0x18, 0x3C, 0x70, 0xB4, 0xFC, 0x14, 0x96, 0xD0, 0x9E, 0xB8 };
code unsigned char TH1_Table[16] = { 0x6A, 0x7A, 0x89, 0x8F, 0x9C, 0xA7, 0xAC, 0xB5, 0xBD, 0xC4, 0xC7, 0xCE, 0xD3, 0xD5, 0xDA, 0xDE };

//每个音符在16分音符(0.12S)的半周期数量
//const unsigned int  TT1_Table[16] = { 196, 220, 247, 261,	293, 329, 349, 392, 440, 494, 523, 587, 659, 698, 784,  880};
//const unsigned int  TT1_Table[16] = { 118,	132,	148,	157,	176,	198,	209,	235,	264,	296,	314,	352,	395,	419,	470,	528};

code unsigned int  TT1_Table[16] = { 94, 105, 118, 126, 141,	158, 167, 188,	211,	237,	251,	282,	316,	335,	376,	422};

//音乐数据中每个字节表示一个音，前4位表示音高(顺序如上表)，后四位表示时值(0表示16分音符，1表示8分音符，3表示4分音符,7表示2分音符，15表示全音符)
//例如 0x00表示 16分音符低音5,0x43表示4分音符中音2

//测试声音(以16音符逐个播放每一个音)
code unsigned char Test_Note[] = { 0x00, 0x10, 0x20, 0x30, 0x40 ,0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0, 0x00};

//开机声音
code unsigned char Turn_On_Voice[] = { 0xB1, 0xA1, 0x53, 0x73, 0xA7, 0x00};

//按键声音

////按键声音
//const unsigned char Turn_On[] = { 0x30, 0x10, 0x00};

////按键声音
//const unsigned char Turn_On[] = { 0x30, 0x10, 0x00};

void Timer1Init(void)		//100微秒@30.000MHz
{
	AUXR |= 0x40;		//定时器时钟1T模式
	TMOD &= 0x0F;		//设置定时器模式
	TF1 = 0;		//清除TF1标志
}

void Timer1_Isr() interrupt 3
{
	if( P54 == 1) P54 = 0;
	else P54 = 1;
	
	note_time ++;
	
	if(note_time == curr_note_time) // 该音符播放完成
	{
		TR1 = 0; //定时器1停止计时
		ET1 = 0; //关闭定时器1中断
		P54 = 0;
		
		
		note_time =  0;
		buff_index ++ ; // 下一个音符
		
		if ( curr_play[buff_index] != 0) // 播放未结束
		{
			curr_note = (curr_play[buff_index] >> 4) & 0x0F; 
			
			curr_note_time = TT1_Table[curr_note] *  ((curr_play[buff_index] & 0x0F) + 1);
			TL1 = TL1_Table[curr_note];		//设置定时初始值
			TH1 = TH1_Table[curr_note];		//设置定时初始值
			
			TR1 = 1;		//定时器1开始计时
			ET1 = 1; //开启定时器1中断
		}
	}
}

void Voice_Stop()
{
	TR1 = 0; //定时器1开始计时
	ET1 = 0; //开启定时器1中断
	P54 = 0;
}

void Voice_Play(char * p)
{
	TR1 = 0; //定时器1开始计时
	ET1 = 0; //开启定时器1中断
	note_time  = 0;
	buff_index = 0;
	
	curr_play = p;
	curr_note = (curr_play[buff_index] >> 4) & 0x0F;
	curr_note_time = TT1_Table[curr_note] *  ((curr_play[buff_index] & 0x0F) + 1);
	TL1 = TL1_Table[curr_note];		//设置定时初始值
	TH1 = TH1_Table[curr_note];		//设置定时初始值
	
	TR1 = 1;		//定时器1开始计时
	ET1 = 1; //开启定时器1中断
}

void Voice_Play_OK()
{
		TR1 = 1;		//定时器1开始计时
		ET1 = 1; //开启定时器1中断
		EA = 1; //开启定时器1中断
}

void Voice_Init()
{
	Timer1Init();		
	Voice_Play(Turn_On_Voice);
}