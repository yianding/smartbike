#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Serial.h"
#include "Serial2.h"
#include "LED.h"
#include "string.h"
#include "AD.h"

uint16_t ADValue;
float Voltage;
extern volatile uint8_t timer_flag;
void unLockBike(){
			//AA 55 10 11 00 21
		//AA 55 10 17 01 A5 CD
		//AA 55 10 19 01 00 2A
		uint8_t d[10];
		d[0]=0xAA;
		d[1]=0x55;
		d[2]=0x10;
		d[3]=0x11;
		d[4]=0x00;
		d[5]=0x21;
	  Serial_SendArray(d,6);
		Delay_ms(100);
	
		d[0]=0xAA;
		d[1]=0x55;
		d[2]=0x10;
		d[3]=0x17;
		d[4]=0x01;
		d[5]=0xA5;
		d[6]=0xCD;
	  Serial_SendArray(d,7);
		Delay_ms(100);

		d[0]=0xAA;
		d[1]=0x55;
		d[2]=0x10;
		d[3]=0x19;
		d[4]=0x01;
		d[5]=0x00;
		d[6]=0x2A;
	  Serial_SendArray(d,7);
		Delay_ms(100);

}
int main(void)
{
	OLED_Init();
	LED_Init();
	Serial_Init();
	
	//OLED_ShowString(1, 1, "TT");
	//OLED_ShowString(3, 1, "RRddt");
	//OLED_ShowString(2, 1, "123456789ao");
	//OLED_ShowString(4, 1, "123456");
	
	
	AD_Init();
	
	OLED_ShowString(1, 1, "ADValue:");
	OLED_ShowString(2, 1, "Volatge:0.00V");
	
	Serial_Init2();
	AT_Init();
	AT_Out();
	GPS_open();                                //delay
	TIM2_Init();
	while (1)
	{
		ADValue = AD_GetValue();
		Voltage = (float)ADValue / 4095 * 3.3;
		
		OLED_ShowNum(1, 9, ADValue, 4);
		OLED_ShowNum(2, 9, Voltage, 1);
		OLED_ShowNum(2, 11, (uint16_t)(Voltage * 100) % 100, 2);
		unLockBike();
		Delay_ms(100);
	    	if(timer_flag==1)
		     {
		      	timer_flag = 0;		
			    GPS_ReceiveDataAndDo();            //读取的可能是同步的，也可能是上一秒存下来的，但是问题不大
		//	    OLED_ShowString(4, 1, "GPS");
		     }
		
	}
/*

	Delay_ms(1000);
	Serial_SendString("AT+QICSGP=1,1,\"\",\"\",\"\"\r\n");
Delay_ms(1000);
  Serial_SendString("AT+NETOPEN\r\n");
	Delay_ms(1000);
  Serial_SendString("AT+MCONFIG=\"4G_TEST\"\r\n");
		Delay_ms(1000);
	Serial_SendString("AT+MIPSTART=\"broker.emqx.io\",1883\r\n");
	Delay_ms(1000);
	
	Serial_SendString("AT+MCONNECT=1,60\r\n");
	Delay_ms(1000);
	Serial_SendString("AT+MSUB=\"phone\",0\r\n");
	
	
	
	
	while(1){
	Delay_ms(3000);
	Serial_SendString("AT+MPUB=\"4G\",0,0,\"This message send from my stm32\"\r\n");
	}
	*/
		while (1)
	{
		if (Serial_RxFlag == 1)
		{
			OLED_ShowString(4, 1, "     dd         ");
			OLED_ShowString(4, 1, Serial_RxPacket);
			//Serial_SendString("AT+MPUB=\"4G\",0,0,"+Serial_RxPacket);
		
			if (strcmp(Serial_RxPacket, "LED_ON") == 0)
			{
				LED1_ON();
				Serial_SendString("LED_ON_OK\r\n");
				OLED_ShowString(2, 1, "                ");
				OLED_ShowString(2, 1, "LED_ON_OK");
			}
			else if (strcmp(Serial_RxPacket, "LED_OFF") == 0)
			{
				LED1_OFF();
				Serial_SendString("LED_OFF_OK\r\n");
				OLED_ShowString(2, 1, "                ");
				OLED_ShowString(2, 1, "LED_OFF_OK");
			}
			else
			{
				Serial_SendString("ERROR_COMMAND\r\n");
				OLED_ShowString(2, 1, "                ");
				OLED_ShowString(2, 1, "ERROR_COMMAND");
			}
			
			Serial_RxFlag = 0;
		}
	}
}


