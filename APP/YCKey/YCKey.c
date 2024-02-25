#include "YCKey.h"
#include "SysTick.h"

extern void vTaskDelay( const uint32_t xTicksToDelay );

/*******************************************************************************
* �� �� ��         : YCKey_Init
* ��������		   : ������Ƶ433M������ʼ������
* ��    ��         : ��
* ��    ��         : ��
*******************************************************************************/
void YCKey_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;//����ṹ�����
	
	RCC_APB2PeriphClockCmd(YCKey_PORT_RCC,ENABLE);
	
	GPIO_InitStructure.GPIO_Pin=YCKey0_PIN | YCKey1_PIN | YCKey2_PIN| YCKey3_PIN;  //ѡ����Ҫ���õ�IO��
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPD;	 //�����������ģʽ
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;	  //���ô�������
	GPIO_Init(YCKey_PORT,&GPIO_InitStructure); 	   /* ��ʼ��GPIO */
}

u8 YCKey_GetNum(u8 mode)
{
	static u8 key=1;
	
	if(mode==1) //������������
		key=1;
	if (key==1&&(KEY0==1 || KEY1==1 || KEY2==1 || KEY3==1))	//���ⰴ������
	{
		delay_ms(10);
		key = 0;
		if (KEY0 == 1)
			return KEY0_PRESS;
		else if(KEY1 == 1)
			return KEY1_PRESS;
		else if(KEY2 == 1)
			return KEY2_PRESS;
		else if(KEY3 == 1)
			return KEY3_PRESS;
	}
	else if (!(KEY0==1||KEY1==1||KEY2==1||KEY3==1)) //�ް�������
		key = 1;
	return KEY_NO;
}




