#ifndef __HAL_OLED_H
#define __HAL_OLED_H

#ifdef __cplusplus
extern "C"
{
#endif

  
#include <ioCC2530.h>
#include <stdio.h>
#include <string.h>


#define  u8 unsigned char 
#define  u32 unsigned int 
#define OLED_CMD  0	//写命令
#define OLED_DATA 1	//写数据
#define OLED_MODE 0

//-----------------OLED端口定义----------------  	
#define OLED_SDIN  P2_0
#define OLED_SCL   P1_2

#define OLED_SCLK_Clr() OLED_SCL=0
#define OLED_SCLK_Set() OLED_SCL=1

#define OLED_SDIN_Clr() OLED_SDIN=0
#define OLED_SDIN_Set() OLED_SDIN=1

/*----------------------*/
#define HAL_OLED_LINE_1 0x00
#define HAL_OLED_LINE_2 0x01
#define HAL_OLED_LINE_3 0x02
#define HAL_OLED_LINE_4 0x03
#define HAL_OLED_LINE_5 0x04
#define HAL_OLED_LINE_6 0x05
#define HAL_OLED_LINE_7 0x06
#define HAL_OLED_XSTART 0x00
#define HAL_OLED_XMIDDLE 0x40 //只有X轴可以用此定义

//OLED模式设置
//0:4线串行模式
//1:并行8080模式

#define SIZE1 12
#define SIZE2 16
  
  
#define XLevelL		0x02
#define XLevelH		0x10
#define Max_Column	128
#define Max_Row		64
#define	Brightness	0xFF 
#define X_WIDTH 	128
#define Y_WIDTH 	64	    						  
				   


//OLED控制用函数
extern void OLED_WR_Byte(unsigned dat,unsigned cmd);  
extern void OLED_Display_On(void);
extern void OLED_Display_Off(void);	   							   		    
extern void OLED_Init(void);
extern void OLED_Clear(void);
extern void OLED_DrawPoint(u8 x,u8 y,u8 t);
extern void OLED_Fill(u8 x1,u8 y1,u8 x2,u8 y2,u8 dot);
extern void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 Char_Size);
extern void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size);
extern void Hal_Oled_WriteString(u8 x,u8 y, u8 *p,u8 Char_Size);	 
extern void OLED_Set_Pos(unsigned char x, unsigned char y);
extern void OLED_ShowCHinese(u8 x,u8 y,u8 no);
extern void OLED_DrawBMP(unsigned char x0, unsigned char y0,unsigned char x1, unsigned char y1,unsigned char BMP[]);
extern void fill_picture(unsigned char fill_Data);
extern void Picture(void);
extern void IIC_Start(void);
extern void IIC_Stop(void);
extern void Write_IIC_Command(unsigned char IIC_Command);
extern void Write_IIC_Data(unsigned char IIC_Data);
extern void Write_IIC_Byte(unsigned char IIC_Byte);
extern void IIC_Wait_Ack(void);
extern void OLED_On(void);
extern u32 oled_pow(u8 m,u8 n);

#ifdef __cplusplus
}
#endif

#endif  
	 



