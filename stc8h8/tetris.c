/*****************************************************
*文件功能：俄罗斯方块游戏
*编写时间：2022.12.30
*文件作者：鹏老师
*******************************************************/
#include "STC8H.h"
#include <intrins.h>
#include "display.h"
#include "voice.h"

void put_char(unsigned char c);
void delay_ms(unsigned int ms);
//int型数据位 16 bit

//方块下落的时间间隔
unsigned int Tetris_Time = 0;

//7种方块元素及其变换数量
code unsigned int boxs[7][5] = 
{
	{0x0E40,0x4C40,0x4E00,0x4640},
	{0x4C80,0xC600,0x4C80,0xC600},
	{0x8C40,0x6C00,0x8C40,0x6C00},
	{0xE800,0xC440,0x2E00,0x4460},
	{0x8E00,0xC880,0xE200,0x2260},
	{0x0F00,0x4444,0x0F00,0x4444},
	{0xCC00,0xCC00,0xCC00,0xCC00}
};

code unsigned long full_data[20] = 
{
		0x00000001,0x00000002,0x00000004,0x00000008,0x00000010,0x00000020,0x00000040,0x00000080,
		0x00000100,0x00000200,0x00000400,0x00000800,0x00001000,0x00002000,0x00004000,0x00008000,
		0x00010000,0x00020000,0x00040000,0x00080000
};

// 左右键声音
code unsigned char KeyRL_Voice[] = { 0x30, 0x60, 0x00};

// 变换键声音
code unsigned char KeyG_Voice[] = { 0x60, 0x30, 0x00};

unsigned char full_line[4] = {0}, full_line_count = 0;//用来记录那些行已满

//当前正在显示的方块的内容
unsigned int box = 0;

//当前正在显示的方块的索引，形变的索引(对应boxs变量)
char box_index = 0, shape_index=1;

//当前显示的方块在屏幕上的坐标
char x = 4,y=0;

//for循环使用的一些变量
unsigned char i,j;

//可移动的方块在屏幕上的缓存
xdata unsigned long Tetris_Move_Buff[10] = { 0 };

//已落下的方块在屏幕上的缓存
xdata unsigned long Tetris_Static_Buff[10] = { 0 };

//合并后的屏幕缓存数据
xdata unsigned long Tetris_Mix_Buff[10] = { 0 };

void Tetris_Init()
{
	box_index = random_number%7;
	shape_index=random_number%4;
}

//检查方块的移动或变换是否符合要求
unsigned char Tetris_Check(char x,char y, unsigned int box)
{
	//下边缘检测
	if(y == 18) return 0;
	if(y == 17)	if(box & 0x8888) return 0;
	
	//左边缘检测
	if(x == -2) return 0;
	if(x == -1) if(box & 0xF000)return 0;
	
	//右边缘检测
	if(x == 10)return 0;
	if(x == 9)if(box & 0x0F00)return 0;
	if(x == 8)if(box & 0x00F0)return 0;
	if(x == 7)if(box & 0x000F)return 0;
	
	//与已落下方块碰撞检测
	if(x>=0 && x < 10)    {Tetris_Move_Buff[x]= box>>12;      				Tetris_Move_Buff[x]<<=y;}
	if(x+1>=0 && x+1 < 10){Tetris_Move_Buff[x+1]= (box&0x0F00) >> 8;  Tetris_Move_Buff[x+1]<<=y;}
	if(x+2>=0 && x+2 < 10){Tetris_Move_Buff[x+2]= ((box&0xF0)>>4);  	Tetris_Move_Buff[x+2]<<=y;}
	if(x+3>=0 && x+3 < 10){Tetris_Move_Buff[x+3]= (box&0x0F);         Tetris_Move_Buff[x+3]<<=y;}
	
	y = x + 4; 
	if(x < 0) x = 0;
	if(y > 10) y = 10;
	
	for(;x < y; x++)
	{
		if(Tetris_Static_Buff[x] & Tetris_Move_Buff[x]) return 0;
	}
	
	return 1;
}

//检查是否有完整的行，并执行相应的消除动作
void Full_Line_Check(void)
{
	for(i=0;i < 10; i++)
	{
		Tetris_Static_Buff[i] |= Tetris_Move_Buff[i];
		Tetris_Move_Buff[i] = Tetris_Static_Buff[i];//用于后面计算满行
	}

	//从当前方块下落后的左边开始检查
	i=y;		y+=4;		if(y>20)y=20;
	for(;i < y; i++) //检测哪些行数据已满
	{
		for(j=0;j<10;j++)
		{
			if((Tetris_Static_Buff[j] & full_data[i]) == 0)break;
		}
		
		if(j == 10) full_line[full_line_count++] = i; //记录数据已满的行号
	}
	
	for(i=0; i < full_line_count;i++) //把满行的数据清空(用于动画闪烁)
	{
		for(j=0;j<10;j++)
		{
			Tetris_Static_Buff[j] &= ~full_data[full_line[i]];
		}
	}
	for(i=0;i<10;i++) //更新到显存
	{
		Screen_Buff[i][0] = Tetris_Static_Buff[i];
		Screen_Buff[i][1] = Tetris_Static_Buff[i]>>8;
		Screen_Buff[i][2] = Tetris_Static_Buff[i]>>12;
	}
	delay_ms(200);
	
	for(i=0; i < full_line_count;i++) //清空后补齐(用于动画闪烁)
	{
		for(j=0;j<10;j++)
		{
			Tetris_Static_Buff[j] |= full_data[full_line[i]];
		}
	}
	for(i=0;i<10;i++) //更新到显存
	{
		Screen_Buff[i][0] = Tetris_Static_Buff[i];
		Screen_Buff[i][1] = Tetris_Static_Buff[i]>>8;
		Screen_Buff[i][2] = Tetris_Static_Buff[i]>>12;
	}
	delay_ms(200);
	
	for(i=0; i < full_line_count;i++) //把满行的数据清空(用于动画闪烁)
	{
		for(j=0;j<10;j++)
		{
			Tetris_Static_Buff[j] &= ~full_data[full_line[i]];
		}
	}
	for(i=0;i<10;i++) //更新到显存
	{
		Screen_Buff[i][0] = Tetris_Static_Buff[i];
		Screen_Buff[i][1] = Tetris_Static_Buff[i]>>8;
		Screen_Buff[i][2] = Tetris_Static_Buff[i]>>12;
	}
	delay_ms(200);
	
	for(i=0; i < full_line_count;i++) //清空后补齐(用于动画闪烁)
	{
		for(j=0;j<10;j++)
		{
			Tetris_Static_Buff[j] |= full_data[full_line[i]];
		}
	}
	for(i=0;i<10;i++) //更新到显存
	{
		Screen_Buff[i][0] = Tetris_Static_Buff[i];
		Screen_Buff[i][1] = Tetris_Static_Buff[i]>>8;
		Screen_Buff[i][2] = Tetris_Static_Buff[i]>>12;
	}
	
	for(i=0; i < full_line_count;i++) 
	{
			for(j=0;j<10;j++)
			{
				Tetris_Static_Buff[j] &= ((unsigned long)(0xfffff) << (full_line[i]+1));
				Tetris_Move_Buff[j] &=   ((unsigned long)(0xfffff) >> (20-full_line[i]));

				Tetris_Move_Buff[j] <<= 1;
				
				Tetris_Static_Buff[j] |= Tetris_Move_Buff[j];
				Tetris_Move_Buff[j] = Tetris_Static_Buff[j];//用于后面计算满行
			}
	}
	
	full_line_count = 0;
	
	box_index = random_number%7;
	shape_index=random_number%4;
	
	//生成新的方格
	x = 4;
	y = 0;
}

void Tetris_Loop(unsigned char key_down_bit)
{
	for( i =0; i <10;i++)
	{
			Tetris_Move_Buff[i] = 0;
	}

	if(key_down_bit & 0x80)//上，暂停
	{
		delay_ms(1000);
		while(1)
		{
			if(P37 == 0) break;
		};
	}
	
	if(key_down_bit & 0x01)//方块旋转
	{
		Voice_Play(KeyG_Voice);
		
		i = shape_index +1;
		if(i == 4) i = 0;
		
		box = boxs[box_index][i];
		if(Tetris_Check(x, y, box)) shape_index++;
		
		if(shape_index == 4)
			shape_index = 0;
	}
	
	box = boxs[box_index][shape_index];
	
	if(key_down_bit & 0x40) //左移
	{
		if(Tetris_Check(x-1, y, box)) x --;
		Voice_Play(KeyRL_Voice);
	}
	
	if(key_down_bit & 0x10)//右移
	{
		if(Tetris_Check(x+1, y, box)) x ++;
		Voice_Play(KeyRL_Voice);
	}
	
	Tetris_Time ++;
	if(key_down_bit & 0x20) //下,加速到底
	{
		Voice_Play(KeyRL_Voice);
		while(Tetris_Check(x, y+1, box)) y ++;
	}
	else if(Tetris_Time == 2000)
	{
		Tetris_Time = 0;
		if(Tetris_Check(x, y+1, box)) y ++;
		else //到底部了
		{
			key_down_bit |= 0x20;//和按下下键效果是一样的
		}
	}
	
	if(x>=0 && x < 10)    {Tetris_Move_Buff[x]= box>>12;      				Tetris_Move_Buff[x]<<=y;}
	if(x+1>=0 && x+1 < 10){Tetris_Move_Buff[x+1]= (box&0x0F00) >> 8;  Tetris_Move_Buff[x+1]<<=y;}
	if(x+2>=0 && x+2 < 10){Tetris_Move_Buff[x+2]= ((box&0xF0)>>4);  	Tetris_Move_Buff[x+2]<<=y;}
	if(x+3>=0 && x+3 < 10){Tetris_Move_Buff[x+3]= (box&0x0F);         Tetris_Move_Buff[x+3]<<=y;}
	
	if(key_down_bit & 0x20) //下键，或者下落到底后 移动的方块编程变成静态的
	{
		Full_Line_Check();
	}
	

	for(i=0;i < 10; i++)
	{
		Tetris_Mix_Buff[i] = Tetris_Static_Buff[i] | Tetris_Move_Buff[i];

		Screen_Buff[i][0] = Tetris_Mix_Buff[i];
		Screen_Buff[i][1] = Tetris_Mix_Buff[i]>>8;
		Screen_Buff[i][2] = Tetris_Mix_Buff[i]>>12;
	}
}