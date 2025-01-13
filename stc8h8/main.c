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

void Voice_Init(void);


const unsigned char com_data[10] = {0xFE, 0xFD, 0xFB,0xF7,0xEF,0xDF,0xBF,0x7F,0xBF,0x7F};
static unsigned char com_num = 0;//记录当刷新到第几个COM
static unsigned char com_status = 0;//记录当前刷新的COM状态(高或者第)
long system_seconds = 0;    // 系统时间的秒部分
long system_microseconds = 0; // 系统时间的微秒部分

sysctx_t ctx;
cpu_state_t cpu;


void Timer0_Isr() interrupt 1
{
	random_number ++;//定时器产生伪随机数
	//system_microseconds += 1;  // 1毫秒

  //if (system_microseconds >= 1000) {
	//	system_microseconds -= 1000;
	//	system_seconds++;  // 如果微秒达到1000000，秒数增加1
	//}
	
	sys_redraw(&ctx,(&cpu)->mem);
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


void main()
{
	unsigned char n = 0 , i = 0 ,d = 0 ;
	unsigned int hold_time = 50;
	unsigned int sleep_ticks = 1000, sleep_delay = 1000;
	unsigned int timer_inc = 16;

	
	memset(&cpu, 0, sizeof(cpu));

	memset(&ctx, 0, sizeof(ctx));
	
	P_SW2 |= 0x80;
	
	EA = 0;
	
	IP = 0x00;   // 先清零
	PT1 = 1;	//定时器1优先级(1)
	
	Pin_Init();
	Display_Init();
	Voice_Init();

	EA = 1;
	while(P15 == 1) //等待长按开机键松开
	{
		delay_ms(2);
	}
	//Tetris_Init(); //随机生成第一个方块
	
	timer_inc = timer_inc ? 16 / timer_inc : 16;
	if (timer_inc > 16) timer_inc = 16;
	
	
	//sys_init(&ctx);

	//ctx.hold_time = hold_time;
	//ctx.sleep_ticks = sleep_ticks;
	//ctx.sleep_delay = sleep_delay;
	ctx.timer_inc = timer_inc;
	//Screen_Buff[1][0] = 0xf;
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