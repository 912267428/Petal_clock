#include "system.h"
#include "SysTick.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "OLED.h"
#include "YCKey.h"
#include "MyRTC.h"

#define GET_BIT(x, bit) ((x & (1 << bit)) >> bit)   /* 获取第bit位 */
#define CAL_ANGLE		((MyRTC_Time[3]%12)*30+(MyRTC_Time[5]*0.5))

//任务优先级
#define START_TASK_PRIO		1
//任务堆栈大小	
#define START_STK_SIZE 		128  
//任务句柄
TaskHandle_t StartTask_Handler;
//任务函数
void start_task(void *pvParameters);

//任务优先级
#define OLED_TASK_PRIO		2
//任务堆栈大小	
#define OLED_STK_SIZE 		128  
//任务句柄
TaskHandle_t OLEDTask_Handler;
//任务函数
void oled_task(void *pvParameters);

//任务优先级	//获取远程按键任务
#define Key_TASK_PRIO		1
//任务堆栈大小	
#define Key_STK_SIZE 		128  
//任务句柄
TaskHandle_t KeyTask_Handler;
//任务函数
void key_task(void *pvParameters);

//任务优先级	//以12小时开合一次对应速度运行的任务
#define Run12h_TASK_PRIO	3
//任务堆栈大小	
#define Run12h_STK_SIZE 		128  
//任务句柄
TaskHandle_t Run12h_Task_Handler;
//任务函数
void run12h_task(void *pvParameters);

//任务优先级	//以60秒开合一次对应速度运行的任务
#define Run60s_TASK_PRIO	3
//任务堆栈大小	
#define Run60s_STK_SIZE 		128  
//任务句柄
TaskHandle_t Run60s_Task_Handler;
//任务函数
void run60s_task(void *pvParameters);

static u8 mode=0;		//bit0:(1:60s mode  0:12h mode) bit1:(1:change time mode | 0:normal mode)
static float angle=0.0;		//当前角度
SemaphoreHandle_t ModeSem = NULL;

//计算当前
float cal_RealAngle()
{
	float angle = (MyRTC_Time[3]%12)*30+(MyRTC_Time[5]*0.5);
	return angle;
}

/*******************************************************************************
* 函 数 名         : main
* 函数功能		   : 主函数
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
int main()
{
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//设置系统中断优先级分组4

	USART1_Init(9600);
	YCKey_Init();
	
	
	//创建开始任务
    xTaskCreate((TaskFunction_t )start_task,            //任务函数
                (const char*    )"start_task",          //任务名称
                (uint16_t       )START_STK_SIZE,        //任务堆栈大小
                (void*          )NULL,                  //传递给任务函数的参数
                (UBaseType_t    )START_TASK_PRIO,       //任务优先级
                (TaskHandle_t*  )&StartTask_Handler);   //任务句柄              
    vTaskStartScheduler();          //开启任务调度
}

//开始任务任务函数
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           //进入临界区
    
	ModeSem = xSemaphoreCreateMutex();
	
	//读取rtc时间
	
	//
	//将步进电机设置为当前时间
	
	//
	
    //创建OLED任务
    xTaskCreate((TaskFunction_t )oled_task,     
                (const char*    )"oled_task",   
                (uint16_t       )OLED_STK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )OLED_TASK_PRIO,
                (TaskHandle_t*  )&OLEDTask_Handler);
				
				
	//创建远程按键任务
    xTaskCreate((TaskFunction_t )key_task,     
                (const char*    )"key_task",   
                (uint16_t       )Key_STK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )Key_TASK_PRIO,
                (TaskHandle_t*  )&KeyTask_Handler);
				
	//创建12h运行任务
    xTaskCreate((TaskFunction_t )run12h_task,     
                (const char*    )"run12h_task",   
                (uint16_t       )Run12h_STK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )Run12h_TASK_PRIO,
                (TaskHandle_t*  )&Run12h_Task_Handler); 
				
	//创建60s运行任务
    xTaskCreate((TaskFunction_t )run60s_task,     
                (const char*    )"run60s_task",   
                (uint16_t       )Run60s_STK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )Run60s_TASK_PRIO,
                (TaskHandle_t*  )&Run60s_Task_Handler);
					
    vTaskDelete(StartTask_Handler); //删除开始任务	
    taskEXIT_CRITICAL();            //退出临界区
				
} 

//OLED任务函数
void oled_task(void *pvParameters)
{
	MyRTC_Init();
	OLED_Init();
	OLED_ShowString(1,1, "Petal Clock");
	OLED_ShowString(2,1,"Date:xxxx-xx-xx");
	OLED_ShowString(3,1,"Time:xx:xx:xx");
	OLED_ShowString(4,1,"mode:");
//	vTaskDelete(OLEDTask_Handler);
    while(1)
    {
		MyRTC_ReadTime();
		
		OLED_ShowNum(2, 6, MyRTC_Time[0], 4);
		OLED_ShowNum(2, 11, MyRTC_Time[1], 2);
		OLED_ShowNum(2, 14, MyRTC_Time[2], 2);
		OLED_ShowNum(3, 6, MyRTC_Time[3], 2);
		OLED_ShowNum(3, 9, MyRTC_Time[4], 2);
		OLED_ShowNum(3, 12, MyRTC_Time[5], 2);
		
		OLED_ShowNum(4, 6, mode, 2);
		vTaskDelay(1000);
    }
}

void key_task(void *pvParameters)
{
	u8 KeyNum = 0;
	BaseType_t xReturn = pdTRUE;

	while(1)
	{
//		MyRTC_ReadTime();
//		printf("%d:%d:%d\r\n",MyRTC_Time[3],MyRTC_Time[4],MyRTC_Time[5]);
		KeyNum = YCKey_GetNum(0);
		if (KeyNum == KEY0_PRESS)
		{//无线射频按键A——开合一次
			printf("KEY0");
			vTaskSuspend(Run12h_Task_Handler);
			//转一圈
			
			//获取时间
			MyRTC_ReadTime();
			//设置位置
			
			vTaskResume(Run12h_Task_Handler);
		}
		else if(KeyNum == KEY1_PRESS)
		{//无线射频按键C——h+1
			printf("KEY1");
			xReturn = xSemaphoreTake(ModeSem,0);
			if(xReturn == pdTRUE)
			{
				switch(mode){
					case 0:
						mode = 2;
						vTaskSuspend(Run12h_Task_Handler);
						break;
					case 1:
						break;
					case 2:
						//在实时时钟中加1小时
						MyRTC_Time[3] = (MyRTC_Time[3]+1)%24;
						MyRTC_SetTime();
						//根据当前时间设置步进电机位置
						

						break;
					default:
						mode = 0;
						break;
				}
				xSemaphoreGive(ModeSem);
			}
			
		}
		else if(KeyNum == KEY2_PRESS)
		{//无线射频按键D——min+1
			printf("KEY2");
			xReturn = xSemaphoreTake(ModeSem,0);
			if(xReturn == pdTRUE)
			{
				switch(mode){
					case 0:
						mode = 2;
						vTaskSuspend(Run12h_Task_Handler);
						break;
					case 1:
						break;
					case 2:
						//在实时时钟中加1分钟
						MyRTC_Time[4] = (MyRTC_Time[4]+1)%60;
						MyRTC_SetTime();
						//根据当前时间设置步进电机位置
						
						break;
					default:
						mode = 0;
						break;
				}
				xSemaphoreGive(ModeSem);
			}
		}
		else if(KeyNum == KEY3_PRESS)
		{//无线射频按键B——change mode
			printf("KEY3");
			xReturn = xSemaphoreTake(ModeSem,0);
			if (xReturn == pdTRUE)
			{//互斥量获取成功
				switch(mode){
					case 0:
						mode = 1;
						vTaskSuspend(Run12h_Task_Handler);
						vTaskResume(Run60s_Task_Handler);
						break;
					case 1:
					case 2:
					default:
						mode = 0;
						vTaskSuspend(Run60s_Task_Handler);
						//获取当前时间
						MyRTC_ReadTime();
						//设置位置
					
						vTaskResume(Run12h_Task_Handler);
						break;
				}
				xSemaphoreGive(ModeSem);
			}	
		}
		
		vTaskDelay(100);
	}
}


void run60s_task(void *pvParameters)
{//60s转动360度=60000个嘀嗒时间转动200个单位角度，每300个滴答时间转动一个角度
	vTaskSuspend(NULL);	//先挂起自己
	
	while(1)
	{
		vTaskDelay(300);
		// 发送一个步进电机转动脉冲
		
		//
		angle += 1.8;
		if(angle >= 360.0)
			angle = 0.0;
		
		printf("60angle:%.1f\r\n",angle);
	}
}

void run12h_task(void *pvParameters)
{//12h转动360度=43200000个嘀嗒时间转动200个单位角度，每216000个滴答时间转动一个角度
	
	while(1)
	{
		vTaskDelay(216000);
		// 发送一个步进电机转动脉冲
		
		//
		angle += 1.8;
		if(angle >= 360.0)
			angle = 0.0;
		
		printf("12angle:%.1f\r\n",angle);
	}
}
