#include <REG51.H>  
#include <intrins.h>
#include <absacc.h>
#include <stdio.h> 
#include <math.h> 
#include <string.h> 
#include "uart.h"
sfr P3M0 =0xb1;
sfr P3M1 =0xb2;
//#include "ds1302.c"
#define uchar unsigned char
#define uint  unsigned int
sbit swich=P1^3;

uchar uart_dat[32]=0;
idata long  count_num=0;
uchar uart_num=0;
uchar endflag = 0;
uchar comm_dat[32];
uchar comm_len=0;
uchar k=0;
signed char PWM=0;
bit   new_en=0;
uchar ButtonFlag = 0;


uchar xdata gettxtval[64];
unsigned long getnumval=0; 
uchar xdata Tx_comm[64];
/*******************测试单片机STC12LE5A60S2******************************/
/*******************晶振 33MHz*******************************************/


void MEMCOPY(uchar *dest,uchar *src,uint len)
{
	while(len--)
	{
		*dest++ = *src++;
	}
}

/**************************************************************************
 - 功能描述：51单片机的串口初始化
 - 隶属模块：STC51串口操作
 - 函数属性：外部，使用户使用
 - 参数说明：无
 - 返回说明：无
 **************************************************************************/
void UART_Init()     
{
 PCON|=0x00; //PCON的最高位SMOD=1时波特率加倍 
 TMOD=0x20;  //时器1为方式2 初值自动装入 产生波特率
 TH1=0xf7;   //定时器初始为0XF7，晶振为33MHz，本函数实际产生的波特率为9600bps
 TL1=0xf7;	 //同上  f7
 
 TH0=(65536-50000)/256;
 TL0=(65536-50000)%256;

 SCON=0x50;	 //串口设置为方式1,REN=1,允许接收
 TR1=1;      //启动定时器1
 TR0=1;      //启动定时器0
 ET0=1;		 //使能定时器T0中断
 ES=1;       //使能串口接收中断，
 EA=1;       //打开所有中断
}


void timer0()interrupt  1//定时器中断函数
{
	TH0=0xFF;//(65536-50000)/256;
 	TL0=0x00;//(65536-50000)%256;
	//swich=~swich; 
	if(k<PWM)
	swich = 0;
	if((k<=100)&&(k>PWM))
	swich = 1;
	if(k>=100)
	k=0;
	k++;


}

void uart_get_int(void) interrupt  4//串口中断接收函数
{
    uchar a=0;
	
	if(RI==1)
	{	   
		RI = 0; 
		a = SBUF;
		uart_dat[uart_num] = a;
		uart_num++;
		if(a==0xff)
		{
			endflag++;
			if(endflag == 3)
			{
				comm_len = uart_num;
				MEMCOPY(comm_dat,uart_dat,comm_len);
				uart_num = 0;
				endflag = 0;
			}
		}  
		else
		{
			endflag = 0;	
		}
	}
	UART_Init();
}

/**************************************************************************
 - 功能描述：51单片机的串口发送字节的函数
 - 隶属模块：STC51串口操作
 - 函数属性：外部，使用户使用
 - 参数说明：mydata:要发送的一个字节
 - 返回说明：无
 - 注：发送一个字节，是串口发送的基础操作
 **************************************************************************/
void UART_Send_Byte(unsigned char mydata)	
{
 ES=0;
 TI=0;
 SBUF=mydata;
 while(!TI);
 TI=0;
 ES=1;
}

/**************************************************************************
 - 功能描述：51单片机的串口发送字符串
 - 隶属模块：STC51串口操作
 - 函数属性：外部，使用户使用
 - 参数说明：s:指向字符串的指针
 - 返回说明：无
 **************************************************************************/
void UART_Send_Str(char *s)
{
 int i=0;
 while(s[i]!=0)
 {
 	UART_Send_Byte(s[i]);
 	i++;
 }
 
}
void UART_Send_END(void)
{
	 UART_Send_Byte(0xFF);
	 UART_Send_Byte(0xFF);
	 UART_Send_Byte(0xFF);
}

void delayms(int count)  // /* X1ms */
{
        int i,j;
        for(i=0;i<count;i++)
                for(j=0;j<1000;j++);
}

void TouchEventReturn(void)
{
	switch(comm_dat[1])
	{
	case 0:
		//页面0数据处理
		if(comm_dat[2]==1)
		{
			//确定当前按下控件ID为1
			if(comm_dat[3]==0x01)
			{
				//确定事件为按下事件
				ButtonFlag	= ~ButtonFlag;
				if(ButtonFlag)
				{
			  UART_Send_Str("q0.picc=1");
					  UART_Send_END();
					  swich = 0;
				}
				else
				{
					UART_Send_Str("q0.picc=0");
					UART_Send_END();
					swich = 1;
				}
			}
		}
		break;
	case 1:
		//页面1数据处理
		break;
	case 2:
		//页面2数据处理
		break;
	default:
		break;	
	}
}

void StringValueReturn(void)
{
	if(comm_len > 60)
		return;
	MEMCOPY(&gettxtval[0],&comm_dat[1],comm_len-1);	//拷贝字符串内同
	gettxtval[comm_len-1]=0;						//增加字符串结束标志

}

void NumberValueReturn(void)
{
	getnumval  = ((unsigned long)comm_dat[4]) << 24;
	getnumval += ((unsigned long)comm_dat[3]) << 16;
	getnumval += ((unsigned long)comm_dat[2]) << 8;
	getnumval += (unsigned long)comm_dat[1];
}

void main(void)
{
 uint s=0;
 uchar flag = 1;
 P3M0&= ~0X02;
 P3M1|=0X02;   //TX强上拉
 
 UART_Init();  //初始化串口，板上的晶振为33MHz，实际的波特率为9600bps 
 delayms(80);  //等待屏幕初始化,建议50ms以上
 UART_Send_END();//发送一次结束符，清除上电产生的串口杂波数据  
 UART_Send_Str("page 0");
 UART_Send_END(); 
 delayms(2000);
// UART_Send_Str("t0.txt=\"40\"");
UART_Send_END();
  PWM=0;
  while(1)
  {
	if(comm_len)
	{
		switch(comm_dat[0])
		{
		case 0x65:		
			//触摸热区事件返回
			TouchEventReturn();
			break;
		case 0x70:
			//字符串变量数据返回
			StringValueReturn();
			break;
		case 0x71:
			//16进制类型变量数据返回
			NumberValueReturn();
			PWM = (signed char)getnumval;
			if(PWM < 0)
			{
				PWM = 0;
			}
			else if(PWM > 100)
			{
				PWM = 100;
			}
			sprintf(Tx_comm,"t0.txt=\"%d\"",(unsigned int)PWM);
			UART_Send_Str(Tx_comm);
			UART_Send_END();
			break;
			//请再次处理其他事件返回
		default:
		break;
		}
		comm_len = 0;
	}
   }
 while(1)
 {	
 	if(flag)
	{
 		
		PWM+=5;
		if(PWM >= 100)
		{
			flag = 0;
			PWM = 99;
		}
		delayms(500);
	}
	else
	{
		 PWM -= 5;
		 if(PWM <= 0)
		 {
		 	PWM = 1;
			flag = 1;
		 }
		 delayms(500);
	}
 }
/*   while(1)
  {
	if(comm_len)
	{
		switch(comm_dat[0])
		{
		case 0x65:		//触摸热区事件返回
			if(comm_dat[1]==0)
			{
				//确定页面ID为0
				if(comm_dat[2]==1)
				{
					//确定当前按下控件ID为1
					if(comm_dat[3]==0x01)
					{
						//确定事件为按下事件
						ButtonFlag	= ~ButtonFlag;
						if(ButtonFlag)
						{
							  UART_Send_Str("s0.picc=1");
							  UART_Send_END();
							  swich = 0;
						}
						else
						{
							UART_Send_Str("s0.picc=0");
							UART_Send_END();
							swich = 1;
						}
					}
				}
			}
		break;
		//请再次处理其他事件返回
		default:
		break;
		}
		comm_len = 0;
	}
   }

 
*/
}


