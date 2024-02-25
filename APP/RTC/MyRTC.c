#include "stm32f10x.h"
#include <time.h>

uint16_t MyRTC_Time[] = {2024, 1, 1, 0, 0, 0};
void MyRTC_SetTime(void);

void MyRTC_Init(void)
{
	//开启时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP,ENABLE);
	
	//使能BKP和RTC的访问
	PWR_BackupAccessCmd(ENABLE);
	
	if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)
	{
		//开启LSE（内部低俗时钟）时钟，并等待LSE启动完成
		RCC_LSEConfig(RCC_LSE_ON);
		while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);
	
		//选择RTCCLK时钟源
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
		RCC_RTCCLKCmd(ENABLE);
	
		//等待同步
		RTC_WaitForSynchro();
		//等待上一次操作完成
		RTC_WaitForLastTask();
	
		//配置预分频器
		RTC_SetPrescaler(32768-1);
		RTC_WaitForLastTask();
	
		//设置初始时间
		MyRTC_SetTime();
		
		BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
	}
	else
	{
		//等待同步
		RTC_WaitForSynchro();
		//等待上一次操作完成
		RTC_WaitForLastTask();
	}
	
	
}

void MyRTC_SetTime(void)
{
	time_t time_cnt;
	struct tm time_date;
	
	time_date.tm_year = MyRTC_Time[0] - 1900;
	time_date.tm_mon = MyRTC_Time[1] - 1;
	time_date.tm_mday = MyRTC_Time[2];
	time_date.tm_hour = MyRTC_Time[3];
	time_date.tm_min = MyRTC_Time[4];
	time_date.tm_sec = MyRTC_Time[5];
	
	time_cnt = mktime(&time_date) - 8 * 60 * 60;
	
	RTC_SetCounter(time_cnt);
	RTC_WaitForLastTask();
}

void MyRTC_ReadTime(void)
{
	time_t time_cnt;
	struct tm time_date;
	
	time_cnt = RTC_GetCounter() + 8 * 60 * 60;
	
	time_date = *localtime(&time_cnt);
	
	MyRTC_Time[0] = time_date.tm_year + 1900;
	MyRTC_Time[1] = time_date.tm_mon + 1;
	MyRTC_Time[2] = time_date.tm_mday;
	MyRTC_Time[3] = time_date.tm_hour;
	MyRTC_Time[4] = time_date.tm_min;
	MyRTC_Time[5] = time_date.tm_sec;
}
