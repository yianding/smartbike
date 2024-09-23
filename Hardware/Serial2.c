#include "stm32f10x.h"                  // Device header
#include <string.h>
#include "DELAY.h"
#include "OLED.h"
#include <stdlib.h> 
#include <stdio.h>

#define BUFFER_SIZE 128         //4G���仺��
#define BUFFER_SIZE2 128        //GPS���仺��

volatile uint32_t timer_counter = 0;
volatile uint8_t timer_flag = 0;


void Serial_Init2(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;   //�����������
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;         //TX
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;    //��������ģʽ
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;         //RX
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART2, &USART_InitStructure);
	
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	NVIC_InitTypeDef NVIC_InitStructure;
        NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure); 
	
	USART_Cmd(USART2, ENABLE);
	
}

void Send_Command(const char* command)   //���ڴ�������
{
	 while (*command)
	 {
		 USART_SendData(USART2,*command);
		 while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
		 command ++;	 
	 }
	 
	 USART_SendData(USART2, '\r');
     while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
    
     USART_SendData(USART2, '\n');
     while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
	 	 
}

//-------------------------------------���ڷ�������ʼ���������ü��------------------------------------

void AT_Distrubute(void)     //����mqttAPN���������ӷ������Ŀ�ʼ
{
	const char* at_Distrubute = "AT+QICSGP=1,1,\"\",\"\",\"\""; 
	Send_Command(at_Distrubute);
	
}

void AT_NETOPEN(void)       //��ģ���������
{
	const char* at_OpenNet = "AT+NETOPEN";
	Send_Command(at_OpenNet);
	
}

void AT_ClienID(void)       //���ÿͻ���ID
{
	const char* at_ClientID = "AT+MCONFIG=\"STM32_TEST\"";
	Send_Command(at_ClientID);
	
}

void AT_MQTTMesseage(void)       //����MQTT��������Ϣ
{
	const char* at_MQTTS = "AT+MIPSTART=\"broker.emqx.io\",1883";
	Send_Command(at_MQTTS);
	
}

void AT_MQTTConnect(void)       //����MQTT������
{
	const char* at_MQTTC = "AT+MCONNECT=1,60";
	Send_Command(at_MQTTC);

}

void AT_TopicOut(void)       //����MQTT������
{
	const char* at_TopicOutC = "AT+MSUB=\"phone\",0";
	Send_Command(at_TopicOutC);
	
}

void AT_Init(void)           //��װģ�����ӵ���������һ������
{
	AT_Distrubute();
	Delay_ms(200);   
	AT_NETOPEN();
	Delay_ms(200);
	AT_ClienID();
	Delay_ms(200);
	AT_MQTTMesseage();
	Delay_ms(200);
	AT_MQTTConnect();
	Delay_ms(200);
	AT_TopicOut();
	Delay_ms(200);
}

void CreateAndSendCommand(char *input)           //���ڷ������ݵ�HH
{
	char command[100];
	strcpy(command, "AT+MPUB=\"HH\",0,0,\"");    //"AT+MPUB=\"HH\",0,0,\"Connection successful\"";
	strcat(command, input);
	strcat(command, "\"");
	
	Send_Command(command);
}

void AT_Out(void)                                //�����ã���ʱ��������
{
	const char* AT_Out = "AT+MPUB=\"HH\",0,0,\"Connecting\"";
	Send_Command(AT_Out);
	
}

//------------------------------------------���ղ���---------------------------------

char rxBuffer[BUFFER_SIZE];
char Lock_data[BUFFER_SIZE];
volatile uint16_t bufferIndex = 0;
char gps_data[BUFFER_SIZE2];

void Lock_Do(void)
{
	char Lock1[10];
	char Lock2[10];
	strcpy(Lock1,"Lock Open");
	strcpy(Lock2,"Lock Off");
                                                 //0x20000018 rxBuffer[] "A+MSUB: "phone",2 bytes,"on""	
	if (strstr(Lock_data, "\"on\"") != NULL)     //input == on ,LED7 off,LED6 on
	{
		CreateAndSendCommand(Lock1);
		memset(Lock_data, 0, BUFFER_SIZE);
	}
	else if (strstr(Lock_data, "\"off\"") != NULL)    //input == off ,LED6 off,LED7 on
	{
		CreateAndSendCommand(Lock2);
		memset(Lock_data, 0, BUFFER_SIZE);
	}
}

void USART2_IRQHandler(void)
{		
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) 
    {
        char receivedData = USART_ReceiveData(USART2);
      
        if (receivedData == '\n' || receivedData == '\r')
        {
            if (bufferIndex > 0)       // ȷ��������������Ч����
            {
                rxBuffer[bufferIndex] = '\0';                      //�������GPS�źţ�����BUFFER_SIZE����
				
				if(strstr(rxBuffer, "MSUB") != NULL)               //����ڲ������룬lock_dataҲ���ϸ���
                {
					memset(Lock_data, 0, BUFFER_SIZE);
					strncpy(Lock_data, rxBuffer, BUFFER_SIZE - 1);
					Lock_data[bufferIndex] = '\0';
					memset(rxBuffer, 0, BUFFER_SIZE);
				}
				if (strstr(rxBuffer,"$GNRMC") != NULL)
				{
					memset(gps_data, 0, BUFFER_SIZE2);             //�������
					strncpy(gps_data, rxBuffer, BUFFER_SIZE2 - 1);
					gps_data[bufferIndex] = '\n';
					gps_data[bufferIndex+1] = '\0';                //����BUFFER_SIZE2���ˡ�ÿ����£�������ȡ����ȴ�
					memset(rxBuffer, 0, BUFFER_SIZE);
				}
            }
            bufferIndex = 0;           //��Ϊ�򵥿���ֱ����
        }
        else
        {
            if (bufferIndex < BUFFER_SIZE - 1)
            {
                rxBuffer[bufferIndex++] = receivedData;
            }
            else
            {
                bufferIndex = 0; 
            }
        }
    }
}


//----------------------------------------���վ�γ�Ȳ�������ֻ�---------------------
void GPS_open(void)
{
	const char* GPS_OpenCommand = "AT+MGPSC=1";
	Send_Command(GPS_OpenCommand);
//	Delay_s(30);                                 //30+60=1.5 min
}


void GPS_min(char A[2][14],char Num[2][20])  //AӦ����δת���ľ�γ��
{
	double n1 = atof(A[0]) / 60.0;     //39.011160
	double n2 = atof(A[1]) / 60.0;	
	char buffer[2][20];
	sprintf(buffer[0], "%.6f", n1);
	sprintf(buffer[1], "%.6f", n2);      //��Num�޸�Ϊ�������ľ�γ��
	strcpy(Num[0],buffer[0]+2);
	strcpy(Num[1],buffer[1]+2);
}

void GPS_FEN(char L[2][11],char K[2][20])                           //�ֶ�ת������������γ�Ⱥ���ķ�λ�ͷ���Ŀ�����޸�K
{
	char a1[14];
	char a2[3];
	char b1[14];
	char b2[3];
	
	strncpy(a1, L[0], 9);       //������L[0]��ά��
    a1[9] = '\0'; 
	strncpy(a2, L[0]+9, 2);     //�淽��������N
    a2[2] = '\0';
	strncpy(b1, L[1], 9);   
    b1[9] = '\0'; 
	strncpy(b2, L[1]+9, 2);     //E
    b2[2] = '\0'; 
	
	char NUM[2][20];
	char A[2][14];
	strcpy(A[0],a1);            //�����ִ��ȥ�ˣ�����ά��
	strcpy(A[1],b1);
	
	GPS_min(A,NUM);             //������������޸�NUM,��ֻ�о�γ��
	
	strcat(NUM[0],a2);
	strcat(NUM[1],b2);
	strcpy(K[0],NUM[0]);
	strcpy(K[1],NUM[1]);
}

void GPS_GetReal(char dai[2][20],char REAL[2][20])            //����->����
{
	char Lo1[3];
	char Lo2[12];
	char La1[4];
	char La2[12];
	
	strncpy(Lo1, dai[1], 2);    //ǰ��λ��41
    Lo1[2] = '\0';               
    
    strncpy(Lo2, dai[1] + 2, 12);   //�ӵ���λ��ʼ11λ��39.011160,N
    Lo2[11] = '\0';                                      //12345678901
    
    strncpy(La1, dai[0], 3);    //123
	La1[3] = '\0';
    
    strncpy(La2, dai[0] + 3, 12);       //24.869769,E
    La2[11] = '\0';
	
	char L[2][11];
	strcpy(L[0],Lo2);    //��ʱ��Lװ��Ҫ�޸ĵĲ���������Ҫ������   ά��
	strcpy(L[1],La2);    //����
	char K[2][20];       //�ƻ���Kװ���޸ĵ�
	
	GPS_FEN(L,K);        //����K�Ѿ����޸�
	
	char dot[2] =".";
	
	strcat(Lo1,dot);
	strcat(Lo1,K[0]);    //��K��֮ǰ�ĺ�����
	strcat(La1,dot);
	strcat(La1,K[1]);    
	
	strcpy(REAL[0],Lo1);   //���ݸ�REAL�����������Ĳ����ѱ��޸�
	REAL[0][12]='\0';
	strcpy(REAL[1],La1);	
	REAL[1][13]='\0';
}                                                       //$GNRMC,,V,,,,,,,,,E,N*08
                                                        //$GNRMC,020539.000,A,4138.976244,N,12324.854130,E,0.464,0.00,220924,,E,A*08
//��ά�Ⱦ��ȸ���                                          $GNRMC,080633.000,A,4139.011160,N,12324.869769,E,0.099,351.29,190924,,E,A*01
void GPS_Get (char gps_FristLine[],char result[2][20])  //$GNRMC,080646.000,A,4139.030265,N,12324.860172,E,2.263,355.12,190924,,E,A*0A
{                                                       //0123456789012345678901234567890123456789012345678901234567890123456789012345
	int k;  //ʵ                                          0         1         2         3         4         5         6         7   
	int p;  //��
	char gps_Longitude[20];   //γ��             �ı�result �൱�ڸı�GPS_REce...�е�result
	char gps_Latitude[20];    //����

	
	for(k=0,p=20;k<13;k++)
	{
		gps_Latitude[k]= gps_FristLine[p+k];
	}
	
	for(k=0,p=34;k<14;k++)
	{
		gps_Longitude[k]= gps_FristLine[p+k];
	}
	gps_Longitude[14] = '\0';
	gps_Latitude[15]= '\0';
	
	char dai[2][20];
	char TRUE[2][20];
	
	strcpy(dai[0],gps_Longitude);    //γ��
	strcpy(dai[1],gps_Latitude);     //����
	
	GPS_GetReal(dai,TRUE);          //TRUE �Ǳ������
	
	strcpy(result[0],TRUE[0]);
	strcpy(result[1],TRUE[1]);      //�޸�result��ֵ
}

void GPS_Send(void)     //
{
	char gps_FristLine[BUFFER_SIZE2];
	
	memset(gps_FristLine, 0, sizeof(gps_FristLine));
	strcpy(gps_FristLine,gps_data);
	char result[2][20];
	
	//strcpy(gps_FristLine,"$GNRMC,080633.000,A,4139.011160,N,12324.869769,E,0.099,351.29,190924,,E,A*01");
	
	GPS_Get(gps_FristLine,result);                 //�õ�����ľ�γ��
	                                               //������һ�иĳ�����ֵ��ok,д��frist
	char site[50];
	char space[5] = "   ";
	strcpy(site,result[0]);
	strcat(site,space);
	strcat(site,result[1]);
	OLED_ShowString(4, 1, result[0]);
	OLED_ShowString(3, 1, result[1]);
	site[26] = '\0';

	CreateAndSendCommand(site);
		
}
