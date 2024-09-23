#ifndef __SERIAL2_H
#define __SERIAL2_H

#include <stdio.h>


void Serial_Init2(void);
void AT_Init(void);
void GPS_open(void);
void TIM2_Init(void);
extern void USART2_IRQHandler(void);
void AT_Out(void);
void GPS_ReceiveDataAndDo(void);



#endif