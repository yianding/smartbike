#ifndef __Serial2_H
#define __Serial2_H

#include <stdio.h>


void Serial_Init2(void);
void AT_Init(void);
void GPS_open(void);
extern void USART2_IRQHandler(void);
void Lock_Do(void);
void AT_Out(void);
void GPS_Send(void);

#endif
