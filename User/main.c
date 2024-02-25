#include "system.h"
#include "SysTick.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "OLED.h"
#include "YCKey.h"
#include "MyRTC.h"

#define GET_BIT(x, bit) ((x & (1 << bit)) >> bit)   /* ��ȡ��bitλ */
#define CAL_ANGLE		((MyRTC_Time[3]%12)*30+(MyRTC_Time[5]*0.5))

//�������ȼ�
#define START_TASK_PRIO		1
//�����ջ��С	
#define START_STK_SIZE 		128  
//������
TaskHandle_t StartTask_Handler;
//������
void start_task(void *pvParameters);

//�������ȼ�
#define OLED_TASK_PRIO		2
//�����ջ��С	
#define OLED_STK_SIZE 		128  
//������
TaskHandle_t OLEDTask_Handler;
//������
void oled_task(void *pvParameters);

//�������ȼ�	//��ȡԶ�̰�������
#define Key_TASK_PRIO		1
//�����ջ��С	
#define Key_STK_SIZE 		128  
//������
TaskHandle_t KeyTask_Handler;
//������
void key_task(void *pvParameters);

//�������ȼ�	//��12Сʱ����һ�ζ�Ӧ�ٶ����е�����
#define Run12h_TASK_PRIO	3
//�����ջ��С	
#define Run12h_STK_SIZE 		128  
//������
TaskHandle_t Run12h_Task_Handler;
//������
void run12h_task(void *pvParameters);

//�������ȼ�	//��60�뿪��һ�ζ�Ӧ�ٶ����е�����
#define Run60s_TASK_PRIO	3
//�����ջ��С	
#define Run60s_STK_SIZE 		128  
//������
TaskHandle_t Run60s_Task_Handler;
//������
void run60s_task(void *pvParameters);

static u8 mode=0;		//bit0:(1:60s mode  0:12h mode) bit1:(1:change time mode | 0:normal mode)
static float angle=0.0;		//��ǰ�Ƕ�
SemaphoreHandle_t ModeSem = NULL;

//���㵱ǰ
float cal_RealAngle()
{
	float angle = (MyRTC_Time[3]%12)*30+(MyRTC_Time[5]*0.5);
	return angle;
}

/*******************************************************************************
* �� �� ��         : main
* ��������		   : ������
* ��    ��         : ��
* ��    ��         : ��
*******************************************************************************/
int main()
{
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//����ϵͳ�ж����ȼ�����4

	USART1_Init(9600);
	YCKey_Init();
	
	
	//������ʼ����
    xTaskCreate((TaskFunction_t )start_task,            //������
                (const char*    )"start_task",          //��������
                (uint16_t       )START_STK_SIZE,        //�����ջ��С
                (void*          )NULL,                  //���ݸ��������Ĳ���
                (UBaseType_t    )START_TASK_PRIO,       //�������ȼ�
                (TaskHandle_t*  )&StartTask_Handler);   //������              
    vTaskStartScheduler();          //�����������
}

//��ʼ����������
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           //�����ٽ���
    
	ModeSem = xSemaphoreCreateMutex();
	
	//��ȡrtcʱ��
	
	//
	//�������������Ϊ��ǰʱ��
	
	//
	
    //����OLED����
    xTaskCreate((TaskFunction_t )oled_task,     
                (const char*    )"oled_task",   
                (uint16_t       )OLED_STK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )OLED_TASK_PRIO,
                (TaskHandle_t*  )&OLEDTask_Handler);
				
				
	//����Զ�̰�������
    xTaskCreate((TaskFunction_t )key_task,     
                (const char*    )"key_task",   
                (uint16_t       )Key_STK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )Key_TASK_PRIO,
                (TaskHandle_t*  )&KeyTask_Handler);
				
	//����12h��������
    xTaskCreate((TaskFunction_t )run12h_task,     
                (const char*    )"run12h_task",   
                (uint16_t       )Run12h_STK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )Run12h_TASK_PRIO,
                (TaskHandle_t*  )&Run12h_Task_Handler); 
				
	//����60s��������
    xTaskCreate((TaskFunction_t )run60s_task,     
                (const char*    )"run60s_task",   
                (uint16_t       )Run60s_STK_SIZE, 
                (void*          )NULL,
                (UBaseType_t    )Run60s_TASK_PRIO,
                (TaskHandle_t*  )&Run60s_Task_Handler);
					
    vTaskDelete(StartTask_Handler); //ɾ����ʼ����	
    taskEXIT_CRITICAL();            //�˳��ٽ���
				
} 

//OLED������
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
		{//������Ƶ����A��������һ��
			printf("KEY0");
			vTaskSuspend(Run12h_Task_Handler);
			//תһȦ
			
			//��ȡʱ��
			MyRTC_ReadTime();
			//����λ��
			
			vTaskResume(Run12h_Task_Handler);
		}
		else if(KeyNum == KEY1_PRESS)
		{//������Ƶ����C����h+1
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
						//��ʵʱʱ���м�1Сʱ
						MyRTC_Time[3] = (MyRTC_Time[3]+1)%24;
						MyRTC_SetTime();
						//���ݵ�ǰʱ�����ò������λ��
						

						break;
					default:
						mode = 0;
						break;
				}
				xSemaphoreGive(ModeSem);
			}
			
		}
		else if(KeyNum == KEY2_PRESS)
		{//������Ƶ����D����min+1
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
						//��ʵʱʱ���м�1����
						MyRTC_Time[4] = (MyRTC_Time[4]+1)%60;
						MyRTC_SetTime();
						//���ݵ�ǰʱ�����ò������λ��
						
						break;
					default:
						mode = 0;
						break;
				}
				xSemaphoreGive(ModeSem);
			}
		}
		else if(KeyNum == KEY3_PRESS)
		{//������Ƶ����B����change mode
			printf("KEY3");
			xReturn = xSemaphoreTake(ModeSem,0);
			if (xReturn == pdTRUE)
			{//��������ȡ�ɹ�
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
						//��ȡ��ǰʱ��
						MyRTC_ReadTime();
						//����λ��
					
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
{//60sת��360��=60000�����ʱ��ת��200����λ�Ƕȣ�ÿ300���δ�ʱ��ת��һ���Ƕ�
	vTaskSuspend(NULL);	//�ȹ����Լ�
	
	while(1)
	{
		vTaskDelay(300);
		// ����һ���������ת������
		
		//
		angle += 1.8;
		if(angle >= 360.0)
			angle = 0.0;
		
		printf("60angle:%.1f\r\n",angle);
	}
}

void run12h_task(void *pvParameters)
{//12hת��360��=43200000�����ʱ��ת��200����λ�Ƕȣ�ÿ216000���δ�ʱ��ת��һ���Ƕ�
	
	while(1)
	{
		vTaskDelay(216000);
		// ����һ���������ת������
		
		//
		angle += 1.8;
		if(angle >= 360.0)
			angle = 0.0;
		
		printf("12angle:%.1f\r\n",angle);
	}
}
