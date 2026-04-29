#include "stm32f10x.h"                  // Device header

//编码格式：UTF-8
 #include "OLED.h"
int main(void)
{
	 OLED_Init();
	 OLED_ShowString(0,0,"Hello!",OLED_8X16);
	 OLED_ShowString(0,16,"World!",OLED_8X16);
	 OLED_ReverseArea(0,16,64,16);
	 OLED_Update();
	
	while (1)
	{
		
	}
}
