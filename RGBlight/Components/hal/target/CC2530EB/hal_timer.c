/**************************************************************************************************
  Filename:       hal_timer.c
  Revised:        $Date: 2010-05-28 15:26:34 -0700 (Fri, 28 May 2010) $
  Revision:       $Revision: 22676 $

  Description:   This file contains the interface to the Timer Service.


  Copyright 2006-2010 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED 揂S IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/*********************************************************************
 NOTE: Z-Stack and TIMAC no longer use CC2530 Timer 1, Timer 3, and 
       Timer 4. The supporting timer driver module is removed and left 
       for the users to implement their own application timer 
       functions.
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include  "hal_mcu.h"
#include  "hal_defs.h"
#include  "hal_types.h"
#include  "hal_timer.h"

void HalTimerInit(void)
{
  /*备用引脚配置*/
  
  PERCFG &= ~0x40;//定时器备用位置1 
 // P2DIR |= 0xC0;  //定时器通道2-3具有第一优先级
  P0SEL |= 0x7C;  // Set P0_2-P0_5 to peripheral
  P0DIR |= 0x7C;
  //P1SEL |= 0x07;
  //P1DIR |= 0x07;
  P2DIR |= 0xC0; 
  //P2SEL &= ~0x10; //定时器1优先于定时器4
  
    
  T1CCTL0 |=  0x1C;//通道初始化
  T1CCTL1 |=  0x1C;
  T1CCTL2 |=  0x1C;
  T1CCTL3 |=  0x1C;
  T1CCTL4 |=  0x1C;
  
  T1CC0H = 0x00;//PWM周期和占空比
  T1CC0L = 0xFF;//255方便和RGBA对应
  T1CC1H = 0x00;
  T1CC1L = 0x00;
  T1CC2H = 0x00;
  T1CC2L = 0x00;
  T1CC3H = 0x00;
  T1CC3L = 0x00;
  T1CC4H = 0x00;
  T1CC4L = 0x00;
  
  T1CTL &= ~(0x03);//暂停运行定时器1
  T1CTL &= ~(0x0c);//恢复不分频状态
  T1CTL |= 0x0c;//设置分频0c
  T1CTL |= 0x02;//设置模模式02


}

