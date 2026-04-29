#include "OLED_Config.h"
#include "OLED_Port.h"

static void OLED_W_SCL(uint8_t BitValue)
{
	GPIO_WriteBit(OLED_I2C_GPIO_PORT, OLED_I2C_SCL_PIN, (BitAction)BitValue);
}

static void OLED_W_SDA(uint8_t BitValue)
{
	GPIO_WriteBit(OLED_I2C_GPIO_PORT, OLED_I2C_SDA_PIN, (BitAction)BitValue);
}

static void OLED_I2C_Start(void)
{
	OLED_W_SDA(1);
	OLED_W_SCL(1);
	OLED_W_SDA(0);
	OLED_W_SCL(0);
}

static void OLED_I2C_Stop(void)
{
	OLED_W_SDA(0);
	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

static void OLED_I2C_SendByte(uint8_t Byte)
{
	uint8_t i;

	for (i = 0; i < 8; i++)
	{
		OLED_W_SDA(!!(Byte & (0x80 >> i)));
		OLED_W_SCL(1);
		OLED_W_SCL(0);
	}

	OLED_W_SCL(1);
	OLED_W_SCL(0);
}

void OLED_Port_Init(void)
{
	uint32_t i;
	uint32_t j;
	GPIO_InitTypeDef GPIO_InitStructure;

	for (i = 0; i < 1000; i++)
	{
		for (j = 0; j < 1000; j++)
		{
		}
	}

	RCC_APB2PeriphClockCmd(OLED_I2C_GPIO_CLK, ENABLE);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = OLED_I2C_SCL_PIN;
	GPIO_Init(OLED_I2C_GPIO_PORT, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = OLED_I2C_SDA_PIN;
	GPIO_Init(OLED_I2C_GPIO_PORT, &GPIO_InitStructure);

	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

void OLED_Port_WriteCommand(uint8_t Command)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(OLED_I2C_ADDRESS);
	OLED_I2C_SendByte(0x00);
	OLED_I2C_SendByte(Command);
	OLED_I2C_Stop();
}

void OLED_Port_WriteData(const uint8_t *Data, uint8_t Count)
{
	uint8_t i;

	OLED_I2C_Start();
	OLED_I2C_SendByte(OLED_I2C_ADDRESS);
	OLED_I2C_SendByte(0x40);

	for (i = 0; i < Count; i++)
	{
		OLED_I2C_SendByte(Data[i]);
	}

	OLED_I2C_Stop();
}
