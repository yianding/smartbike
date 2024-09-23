#include "stm32f10x.h"                  // Device header
#include <string.h>
#include "DELAY.h"
#include "OLED.h"
#include <stdlib.h> 
#include <stdio.h>

#define BUFFER_SIZE 128         //4G传输缓存
#define BUFFER_SIZE2 128        //GPS传输缓存

volatile uint32_t timer_counter = 0;
volatile uint8_t timer_flag = 0;


void Serial_Init2(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;   //复用推挽输出
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;         //TX
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;    //浮空输入模式
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

void Send_Command(const char* command)   //用于传输命令
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

//-------------------------------------用于服务器初始化，不设置检测------------------------------------

void AT_Distrubute(void)     //配置mqttAPN，用于连接服务器的开始
{
	const char* at_Distrubute = "AT+QICSGP=1,1,\"\",\"\",\"\""; 
	Send_Command(at_Distrubute);
	
}

void AT_NETOPEN(void)       //打开模块蜂窝网络
{
	const char* at_OpenNet = "AT+NETOPEN";
	Send_Command(at_OpenNet);
	
}

void AT_ClienID(void)       //设置客户端ID
{
	const char* at_ClientID = "AT+MCONFIG=\"STM32_TEST\"";
	Send_Command(at_ClientID);
	
}

void AT_MQTTMesseage(void)       //设置MQTT服务器信息
{
	const char* at_MQTTS = "AT+MIPSTART=\"broker.emqx.io\",1883";
	Send_Command(at_MQTTS);
	
}

void AT_MQTTConnect(void)       //连接MQTT服务器
{
	const char* at_MQTTC = "AT+MCONNECT=1,60";
	Send_Command(at_MQTTC);

}

void AT_TopicOut(void)       //订阅MQTT服务器
{
	const char* at_TopicOutC = "AT+MSUB=\"phone\",0";
	Send_Command(at_TopicOutC);
	
}

void AT_Init(void)           //封装模块连接到服务器的一套流程
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

void CreateAndSendCommand(char *input)           //用于发送数据到HH
{
	char command[100];
	strcpy(command, "AT+MPUB=\"HH\",0,0,\"");    //"AT+MPUB=\"HH\",0,0,\"Connection successful\"";
	strcat(command, input);
	strcat(command, "\"");
	
	Send_Command(command);
}

void AT_Out(void)                                //测试用，有时连不上网
{
	const char* AT_Out = "AT+MPUB=\"HH\",0,0,\"Connecting\"";
	Send_Command(AT_Out);
	
}

//------------------------------------------接收部分---------------------------------

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
            if (bufferIndex > 0)       // 确保缓冲区中有有效数据
            {
                rxBuffer[bufferIndex] = '\0';                      //如果不是GPS信号，存在BUFFER_SIZE里面
				
				if(strstr(rxBuffer, "MSUB") != NULL)               //如果在不断输入，lock_data也不断更新
                {
					memset(Lock_data, 0, BUFFER_SIZE);
					strncpy(Lock_data, rxBuffer, BUFFER_SIZE - 1);
					Lock_data[bufferIndex] = '\0';
					memset(rxBuffer, 0, BUFFER_SIZE);
				}
				if (strstr(rxBuffer,"$GNRMC") != NULL)
				{
					memset(gps_data, 0, BUFFER_SIZE2);             //清除缓存
					strncpy(gps_data, rxBuffer, BUFFER_SIZE2 - 1);
					gps_data[bufferIndex] = '\n';
					gps_data[bufferIndex+1] = '\0';                //存在BUFFER_SIZE2中了。每秒更新，这样读取无需等待
					memset(rxBuffer, 0, BUFFER_SIZE);
				}
            }
            bufferIndex = 0;           //较为简单可以直接用
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


//----------------------------------------接收经纬度并传输给手机---------------------
void GPS_open(void)
{
	const char* GPS_OpenCommand = "AT+MGPSC=1";
	Send_Command(GPS_OpenCommand);
//	Delay_s(30);                                 //30+60=1.5 min
}


void GPS_min(char A[2][14],char Num[2][20])  //A应该是未转换的经纬度
{
	double n1 = atof(A[0]) / 60.0;     //39.011160
	double n2 = atof(A[1]) / 60.0;	
	char buffer[2][20];
	sprintf(buffer[0], "%.6f", n1);
	sprintf(buffer[1], "%.6f", n2);      //把Num修改为了真正的经纬度
	strcpy(Num[0],buffer[0]+2);
	strcpy(Num[1],buffer[1]+2);
}

void GPS_FEN(char L[2][11],char K[2][20])                           //分度转化，用来处理经纬度后面的分位和方向，目的是修改K
{
	char a1[14];
	char a2[3];
	char b1[14];
	char b2[3];
	
	strncpy(a1, L[0], 9);       //存数字L[0]是维度
    a1[9] = '\0'; 
	strncpy(a2, L[0]+9, 2);     //存方向，这里是N
    a2[2] = '\0';
	strncpy(b1, L[1], 9);   
    b1[9] = '\0'; 
	strncpy(b2, L[1]+9, 2);     //E
    b2[2] = '\0'; 
	
	char NUM[2][20];
	char A[2][14];
	strcpy(A[0],a1);            //把数字存进去了，这是维度
	strcpy(A[1],b1);
	
	GPS_min(A,NUM);             //这里的作用是修改NUM,还只有经纬度
	
	strcat(NUM[0],a2);
	strcat(NUM[1],b2);
	strcpy(K[0],NUM[0]);
	strcpy(K[1],NUM[1]);
}

void GPS_GetReal(char dai[2][20],char REAL[2][20])            //抽象->形象
{
	char Lo1[3];
	char Lo2[12];
	char La1[4];
	char La2[12];
	
	strncpy(Lo1, dai[1], 2);    //前两位，41
    Lo1[2] = '\0';               
    
    strncpy(Lo2, dai[1] + 2, 12);   //从第三位开始11位，39.011160,N
    Lo2[11] = '\0';                                      //12345678901
    
    strncpy(La1, dai[0], 3);    //123
	La1[3] = '\0';
    
    strncpy(La2, dai[0] + 3, 12);       //24.869769,E
    La2[11] = '\0';
	
	char L[2][11];
	strcpy(L[0],Lo2);    //此时用L装着要修改的参数，等下要传回来   维度
	strcpy(L[1],La2);    //经度
	char K[2][20];       //计划用K装被修改的
	
	GPS_FEN(L,K);        //这里K已经被修改
	
	char dot[2] =".";
	
	strcat(Lo1,dot);
	strcat(Lo1,K[0]);    //把K和之前的合起来
	strcat(La1,dot);
	strcat(La1,K[1]);    
	
	strcpy(REAL[0],Lo1);   //传递给REAL，即传进来的参数已被修改
	REAL[0][12]='\0';
	strcpy(REAL[1],La1);	
	REAL[1][13]='\0';
}                                                       //$GNRMC,,V,,,,,,,,,E,N*08
                                                        //$GNRMC,020539.000,A,4138.976244,N,12324.854130,E,0.464,0.00,220924,,E,A*08
//给维度经度复制                                          $GNRMC,080633.000,A,4139.011160,N,12324.869769,E,0.099,351.29,190924,,E,A*01
void GPS_Get (char gps_FristLine[],char result[2][20])  //$GNRMC,080646.000,A,4139.030265,N,12324.860172,E,2.263,355.12,190924,,E,A*0A
{                                                       //0123456789012345678901234567890123456789012345678901234567890123456789012345
	int k;  //实                                          0         1         2         3         4         5         6         7   
	int p;  //虚
	char gps_Longitude[20];   //纬度             改变result 相当于改变GPS_REce...中的result
	char gps_Latitude[20];    //经度

	
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
	
	strcpy(dai[0],gps_Longitude);    //纬度
	strcpy(dai[1],gps_Latitude);     //经度
	
	GPS_GetReal(dai,TRUE);          //TRUE 是被改完的
	
	strcpy(result[0],TRUE[0]);
	strcpy(result[1],TRUE[1]);      //修改result的值
}

void GPS_Send(void)     //
{
	char gps_FristLine[BUFFER_SIZE2];
	
	memset(gps_FristLine, 0, sizeof(gps_FristLine));
	strcpy(gps_FristLine,gps_data);
	char result[2][20];
	
	//strcpy(gps_FristLine,"$GNRMC,080633.000,A,4139.011160,N,12324.869769,E,0.099,351.29,190924,,E,A*01");
	
	GPS_Get(gps_FristLine,result);                 //得到抽象的经纬度
	                                               //所以这一行改成正常值就ok,写到frist
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
