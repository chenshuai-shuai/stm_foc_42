

#include "stm32f10x.h"
#include "OLED.h"
#include "OLED_Port.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>




static uint8_t OLED_DisplayBuf[OLED_PAGE_COUNT][OLED_WIDTH];





static void OLED_WriteCommand(uint8_t Command)
{
    OLED_Port_WriteCommand(Command);
}

static void OLED_WriteData(const uint8_t *Data, uint8_t Count)
{
    OLED_Port_WriteData(Data, Count);
}





void OLED_Init(void)
{
	OLED_Port_Init();			//鍏堣皟鐢ㄥ簳灞傜殑绔彛鍒濆鍖?
	
	OLED_WriteCommand(0xAE);	//璁剧疆鏄剧ず寮€鍚?鍏抽棴锛?xAE鍏抽棴锛?xAF寮€鍚?
	
	OLED_WriteCommand(0xD5);	//璁剧疆鏄剧ず鏃堕挓鍒嗛姣?鎸崱鍣ㄩ鐜?
	OLED_WriteCommand(OLED_CMD_DISPLAY_CLOCK_DIV);	//0x00~0xFF
	
	OLED_WriteCommand(0xA8);	//璁剧疆澶氳矾澶嶇敤鐜?
	OLED_WriteCommand(OLED_CMD_MULTIPLEX_RATIO);	//0x0E~0x3F
	
	OLED_WriteCommand(0xD3);	//璁剧疆鏄剧ず鍋忕Щ
	OLED_WriteCommand(OLED_CMD_DISPLAY_OFFSET);	//0x00~0x7F
	
	OLED_WriteCommand(OLED_CMD_DISPLAY_START_LINE);	//璁剧疆鏄剧ず寮€濮嬭锛?x40~0x7F
	
	OLED_WriteCommand(OLED_CMD_SEG_REMAP);	//璁剧疆宸﹀彸鏂瑰悜锛?xA1姝ｅ父锛?xA0宸﹀彸鍙嶇疆
	
	OLED_WriteCommand(OLED_CMD_COM_SCAN_DIR);	//璁剧疆涓婁笅鏂瑰悜锛?xC8姝ｅ父锛?xC0涓婁笅鍙嶇疆

	OLED_WriteCommand(0xDA);	//璁剧疆COM寮曡剼纭欢閰嶇疆
	OLED_WriteCommand(OLED_CMD_COM_PINS_CONFIG);
	
	OLED_WriteCommand(0x81);	//璁剧疆瀵规瘮搴?
	OLED_WriteCommand(OLED_CMD_CONTRAST);	//0x00~0xFF

	OLED_WriteCommand(0xD9);	//璁剧疆棰勫厖鐢靛懆鏈?
	OLED_WriteCommand(OLED_CMD_PRECHARGE_PERIOD);

	OLED_WriteCommand(0xDB);	//璁剧疆VCOMH鍙栨秷閫夋嫨绾у埆
	OLED_WriteCommand(OLED_CMD_VCOMH_LEVEL);

	OLED_WriteCommand(0xA4);	//璁剧疆鏁翠釜鏄剧ず鎵撳紑/鍏抽棴

	OLED_WriteCommand(0xA6);	//璁剧疆姝ｅ父/鍙嶈壊鏄剧ず锛?xA6姝ｅ父锛?xA7鍙嶈壊

	OLED_WriteCommand(0x8D);	//璁剧疆鍏呯數娉?
	OLED_WriteCommand(OLED_CMD_CHARGE_PUMP);

	OLED_WriteCommand(0xAF);	//寮€鍚樉绀?
	
	OLED_Clear();				//娓呯┖鏄惧瓨鏁扮粍
	OLED_Update();				//鏇存柊鏄剧ず锛屾竻灞忥紝闃叉鍒濆鍖栧悗鏈樉绀哄唴瀹规椂鑺卞睆
}

static void OLED_SetCursor(uint8_t Page, uint8_t X)
{
    X += OLED_COLUMN_OFFSET;
	
	OLED_WriteCommand(0xB0 | Page);					//璁剧疆椤典綅缃?
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));	//璁剧疆X浣嶇疆楂?浣?
	OLED_WriteCommand(0x00 | (X & 0x0F));			//璁剧疆X浣嶇疆浣?浣?
}





uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;	//缁撴灉榛樿涓?
	while (Y --)			//绱箻Y娆?
	{
		Result *= X;		//姣忔鎶奨绱箻鍒扮粨鏋滀笂
	}
	return Result;
}

uint8_t OLED_pnpoly(uint8_t nvert, int16_t *vertx, int16_t *verty, int16_t testx, int16_t testy)
{
	int16_t i, j, c = 0;
	
	for (i = 0, j = nvert - 1; i < nvert; j = i++)
	{
		if (((verty[i] > testy) != (verty[j] > testy)) &&
			(testx < (vertx[j] - vertx[i]) * (testy - verty[i]) / (verty[j] - verty[i]) + vertx[i]))
		{
			c = !c;
		}
	}
	return c;
}

uint8_t OLED_IsInAngle(int16_t X, int16_t Y, int16_t StartAngle, int16_t EndAngle)
{
	int16_t PointAngle;
	PointAngle = atan2(Y, X) / 3.14 * 180;	//璁＄畻鎸囧畾鐐圭殑寮у害锛屽苟杞崲涓鸿搴﹁〃绀?
	if (StartAngle < EndAngle)	//璧峰瑙掑害灏忎簬缁堟瑙掑害鐨勬儏鍐?
	{
		if (PointAngle >= StartAngle && PointAngle <= EndAngle)
		{
			return 1;
		}
	}
	else			//璧峰瑙掑害澶т簬浜庣粓姝㈣搴︾殑鎯呭喌
	{
		if (PointAngle >= StartAngle || PointAngle <= EndAngle)
		{
			return 1;
		}
	}
	return 0;		//涓嶆弧瓒充互涓婃潯浠讹紝鍒欏垽鏂垽瀹氭寚瀹氱偣涓嶅湪鎸囧畾瑙掑害
}




void OLED_Update(void)
{
	uint8_t j;
	for (j = 0; j < OLED_PAGE_COUNT; j ++)
	{
		OLED_SetCursor(j, 0);
		OLED_WriteData(OLED_DisplayBuf[j], OLED_WIDTH);
	}
}

void OLED_UpdateArea(uint8_t X, uint8_t Y, uint8_t Width, uint8_t Height)
{
	uint8_t j;
	
	if (X > OLED_MAX_X) {return;}
	if (Y > OLED_MAX_Y) {return;}
	if (X + Width > OLED_WIDTH) {Width = OLED_WIDTH - X;}
	if (Y + Height > OLED_HEIGHT) {Height = OLED_HEIGHT - Y;}
	
	for (j = Y / 8; j < (Y + Height - 1) / 8 + 1; j ++)
	{
		OLED_SetCursor(j, X);
		OLED_WriteData(&OLED_DisplayBuf[j][X], Width);
	}
}

void OLED_Clear(void)
{
	uint8_t i, j;
	for (j = 0; j < OLED_PAGE_COUNT; j ++)				//閬嶅巻8椤?
	{
		for (i = 0; i < OLED_WIDTH; i ++)			//閬嶅巻128鍒?
		{
			OLED_DisplayBuf[j][i] = 0x00;	//灏嗘樉瀛樻暟缁勬暟鎹叏閮ㄦ竻闆?
		}
	}
}

void OLED_ClearArea(uint8_t X, uint8_t Y, uint8_t Width, uint8_t Height)
{
	uint8_t i, j;
	
	if (X > OLED_MAX_X) {return;}
	if (Y > OLED_MAX_Y) {return;}
	if (X + Width > OLED_WIDTH) {Width = OLED_WIDTH - X;}
	if (Y + Height > OLED_HEIGHT) {Height = OLED_HEIGHT - Y;}
	
	for (j = Y; j < Y + Height; j ++)		//閬嶅巻鎸囧畾椤?
	{
		for (i = X; i < X + Width; i ++)	//閬嶅巻鎸囧畾鍒?
		{
			OLED_DisplayBuf[j / 8][i] &= ~(0x01 << (j % 8));	//灏嗘樉瀛樻暟缁勬寚瀹氭暟鎹竻闆?
		}
	}
}

void OLED_Reverse(void)
{
	uint8_t i, j;
	for (j = 0; j < OLED_PAGE_COUNT; j ++)				//閬嶅巻8椤?
	{
		for (i = 0; i < OLED_WIDTH; i ++)			//閬嶅巻128鍒?
		{
			OLED_DisplayBuf[j][i] ^= 0xFF;	//灏嗘樉瀛樻暟缁勬暟鎹叏閮ㄥ彇鍙?
		}
	}
}
	
void OLED_ReverseArea(uint8_t X, uint8_t Y, uint8_t Width, uint8_t Height)
{
	uint8_t i, j;
	
	if (X > OLED_MAX_X) {return;}
	if (Y > OLED_MAX_Y) {return;}
	if (X + Width > OLED_WIDTH) {Width = OLED_WIDTH - X;}
	if (Y + Height > OLED_HEIGHT) {Height = OLED_HEIGHT - Y;}
	
	for (j = Y; j < Y + Height; j ++)		//閬嶅巻鎸囧畾椤?
	{
		for (i = X; i < X + Width; i ++)	//閬嶅巻鎸囧畾鍒?
		{
			OLED_DisplayBuf[j / 8][i] ^= 0x01 << (j % 8);	//灏嗘樉瀛樻暟缁勬寚瀹氭暟鎹彇鍙?
		}
	}
}

void OLED_ShowChar(uint8_t X, uint8_t Y, char Char, uint8_t FontSize)
{
	if (FontSize == OLED_8X16)		//瀛椾綋涓哄8鍍忕礌锛岄珮16鍍忕礌
	{
		OLED_ShowImage(X, Y, 8, 16, OLED_F8x16[Char - ' ']);
	}
	else if(FontSize == OLED_6X8)	//瀛椾綋涓哄6鍍忕礌锛岄珮8鍍忕礌
	{
		OLED_ShowImage(X, Y, 6, 8, OLED_F6x8[Char - ' ']);
	}
}

void OLED_ShowString(uint8_t X, uint8_t Y, char *String, uint8_t FontSize)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)		//閬嶅巻瀛楃涓茬殑姣忎釜瀛楃
	{
		OLED_ShowChar(X + i * FontSize, Y, String[i], FontSize);
	}
}

void OLED_ShowNum(uint8_t X, uint8_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
	uint8_t i;
	for (i = 0; i < Length; i++)		//閬嶅巻鏁板瓧鐨勬瘡涓€浣?						
	{
		OLED_ShowChar(X + i * FontSize, Y, Number / OLED_Pow(10, Length - i - 1) % 10 + '0', FontSize);
	}
}

void OLED_ShowSignedNum(uint8_t X, uint8_t Y, int32_t Number, uint8_t Length, uint8_t FontSize)
{
	uint8_t i;
	uint32_t Number1;
	
	if (Number >= 0)						//鏁板瓧澶т簬绛変簬0
	{
		OLED_ShowChar(X, Y, '+', FontSize);	//鏄剧ず+鍙?
		Number1 = Number;					//Number1鐩存帴绛変簬Number
	}
	else									//鏁板瓧灏忎簬0
	{
		OLED_ShowChar(X, Y, '-', FontSize);	//鏄剧ず-鍙?
		Number1 = -Number;					//Number1绛変簬Number鍙栬礋
	}
	
	for (i = 0; i < Length; i++)			//閬嶅巻鏁板瓧鐨勬瘡涓€浣?							
	{
		OLED_ShowChar(X + (i + 1) * FontSize, Y, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0', FontSize);
	}
}

void OLED_ShowHexNum(uint8_t X, uint8_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
	uint8_t i, SingleNumber;
	for (i = 0; i < Length; i++)		//閬嶅巻鏁板瓧鐨勬瘡涓€浣?
	{
		SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
		
		if (SingleNumber < 10)			//鍗曚釜鏁板瓧灏忎簬10
		{
			OLED_ShowChar(X + i * FontSize, Y, SingleNumber + '0', FontSize);
		}
		else							//鍗曚釜鏁板瓧澶т簬10
		{
			OLED_ShowChar(X + i * FontSize, Y, SingleNumber - 10 + 'A', FontSize);
		}
	}
}

void OLED_ShowBinNum(uint8_t X, uint8_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
	uint8_t i;
	for (i = 0; i < Length; i++)		//閬嶅巻鏁板瓧鐨勬瘡涓€浣?
	{
		OLED_ShowChar(X + i * FontSize, Y, Number / OLED_Pow(2, Length - i - 1) % 2 + '0', FontSize);
	}
}

void OLED_ShowFloatNum(uint8_t X, uint8_t Y, double Number, uint8_t IntLength, uint8_t FraLength, uint8_t FontSize)
{
	uint32_t PowNum, IntNum, FraNum;
	
	if (Number >= 0)						//鏁板瓧澶т簬绛変簬0
	{
		OLED_ShowChar(X, Y, '+', FontSize);	//鏄剧ず+鍙?
	}
	else									//鏁板瓧灏忎簬0
	{
		OLED_ShowChar(X, Y, '-', FontSize);	//鏄剧ず-鍙?
		Number = -Number;					//Number鍙栬礋
	}
	
	IntNum = Number;						//鐩存帴璧嬪€肩粰鏁村瀷鍙橀噺锛屾彁鍙栨暣鏁?
	Number -= IntNum;						//灏哊umber鐨勬暣鏁板噺鎺夛紝闃叉涔嬪悗灏嗗皬鏁颁箻鍒版暣鏁版椂鍥犳暟杩囧ぇ閫犳垚閿欒
	PowNum = OLED_Pow(10, FraLength);		//鏍规嵁鎸囧畾灏忔暟鐨勪綅鏁帮紝纭畾涔樻暟
	FraNum = round(Number * PowNum);		//灏嗗皬鏁颁箻鍒版暣鏁帮紝鍚屾椂鍥涜垗浜斿叆锛岄伩鍏嶆樉绀鸿宸?
	IntNum += FraNum / PowNum;				//鑻ュ洓鑸嶄簲鍏ラ€犳垚浜嗚繘浣嶏紝鍒欓渶瑕佸啀鍔犵粰鏁存暟
	
	OLED_ShowNum(X + FontSize, Y, IntNum, IntLength, FontSize);
	
	OLED_ShowChar(X + (IntLength + 1) * FontSize, Y, '.', FontSize);
	
	OLED_ShowNum(X + (IntLength + 2) * FontSize, Y, FraNum, FraLength, FontSize);
}

void OLED_ShowChinese(uint8_t X, uint8_t Y, char *Chinese)
{
	uint8_t pChinese = 0;
	uint8_t pIndex;
	uint8_t i;
	char SingleChinese[OLED_CHN_CHAR_WIDTH + 1] = {0};
	
	for (i = 0; Chinese[i] != '\0'; i ++)		//閬嶅巻姹夊瓧涓?
	{
		SingleChinese[pChinese] = Chinese[i];	//鎻愬彇姹夊瓧涓叉暟鎹埌鍗曚釜姹夊瓧鏁扮粍
		pChinese ++;							//璁℃鑷
		
		if (pChinese >= OLED_CHN_CHAR_WIDTH)
		{
			pChinese = 0;		//璁℃褰掗浂
			
			for (pIndex = 0; strcmp(OLED_CF16x16[pIndex].Index, "") != 0; pIndex ++)
			{
				if (strcmp(OLED_CF16x16[pIndex].Index, SingleChinese) == 0)
				{
					break;		//璺冲嚭寰幆锛屾鏃秔Index鐨勫€间负鎸囧畾姹夊瓧鐨勭储寮?
				}
			}
			
			OLED_ShowImage(X + ((i + 1) / OLED_CHN_CHAR_WIDTH - 1) * 16, Y, 16, 16, OLED_CF16x16[pIndex].Data);
		}
	}
}

void OLED_ShowImage(uint8_t X, uint8_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image)
{
	uint8_t i, j;
	
	if (X > OLED_MAX_X) {return;}
	if (Y > OLED_MAX_Y) {return;}
	
	OLED_ClearArea(X, Y, Width, Height);
	
	for (j = 0; j < (Height - 1) / 8 + 1; j ++)
	{
		for (i = 0; i < Width; i ++)
		{
			if (X + i > OLED_MAX_X) {break;}
			if (Y / 8 + j >= OLED_PAGE_COUNT) {return;}
			
			OLED_DisplayBuf[Y / 8 + j][X + i] |= Image[j * Width + i] << (Y % 8);
			
			if (Y / 8 + j + 1 >= OLED_PAGE_COUNT) {continue;}
			
			OLED_DisplayBuf[Y / 8 + j + 1][X + i] |= Image[j * Width + i] >> (8 - Y % 8);
		}
	}
}

void OLED_Printf(uint8_t X, uint8_t Y, uint8_t FontSize, char *format, ...)
{
	char String[30];						//瀹氫箟瀛楃鏁扮粍
	va_list arg;							//瀹氫箟鍙彉鍙傛暟鍒楄〃鏁版嵁绫诲瀷鐨勫彉閲廰rg
	va_start(arg, format);					//浠巉ormat寮€濮嬶紝鎺ユ敹鍙傛暟鍒楄〃鍒癮rg鍙橀噺
	vsprintf(String, format, arg);			//浣跨敤vsprintf鎵撳嵃鏍煎紡鍖栧瓧绗︿覆鍜屽弬鏁板垪琛ㄥ埌瀛楃鏁扮粍涓?
	va_end(arg);							//缁撴潫鍙橀噺arg
	OLED_ShowString(X, Y, String, FontSize);//OLED鏄剧ず瀛楃鏁扮粍锛堝瓧绗︿覆锛?
}

void OLED_DrawPoint(uint8_t X, uint8_t Y)
{
	if (X > OLED_MAX_X) {return;}
	if (Y > OLED_MAX_Y) {return;}
	
	OLED_DisplayBuf[Y / 8][X] |= 0x01 << (Y % 8);
}

uint8_t OLED_GetPoint(uint8_t X, uint8_t Y)
{
	if (X > OLED_MAX_X) {return 0;}
	if (Y > OLED_MAX_Y) {return 0;}
	
	if (OLED_DisplayBuf[Y / 8][X] & 0x01 << (Y % 8))
	{
		return 1;	//涓?锛岃繑鍥?
	}
	
	return 0;		//鍚﹀垯锛岃繑鍥?
}

void OLED_DrawLine(uint8_t X0, uint8_t Y0, uint8_t X1, uint8_t Y1)
{
	int16_t x, y, dx, dy, d, incrE, incrNE, temp;
	int16_t x0 = X0, y0 = Y0, x1 = X1, y1 = Y1;
	uint8_t yflag = 0, xyflag = 0;
	
	if (y0 == y1)		//妯嚎鍗曠嫭澶勭悊
	{
		if (x0 > x1) {temp = x0; x0 = x1; x1 = temp;}
		
		for (x = x0; x <= x1; x ++)
		{
			OLED_DrawPoint(x, y0);	//渚濇鐢荤偣
		}
	}
	else if (x0 == x1)	//绔栫嚎鍗曠嫭澶勭悊
	{
		if (y0 > y1) {temp = y0; y0 = y1; y1 = temp;}
		
		for (y = y0; y <= y1; y ++)
		{
			OLED_DrawPoint(x0, y);	//渚濇鐢荤偣
		}
	}
	else				//鏂滅嚎
	{
		
		if (x0 > x1)	//0鍙风偣X鍧愭爣澶т簬1鍙风偣X鍧愭爣
		{
			temp = x0; x0 = x1; x1 = temp;
			temp = y0; y0 = y1; y1 = temp;
		}
		
		if (y0 > y1)	//0鍙风偣Y鍧愭爣澶т簬1鍙风偣Y鍧愭爣
		{
			y0 = -y0;
			y1 = -y1;
			
			yflag = 1;
		}
		
		if (y1 - y0 > x1 - x0)	//鐢荤嚎鏂滅巼澶т簬1
		{
			temp = x0; x0 = y0; y0 = temp;
			temp = x1; x1 = y1; y1 = temp;
			
			xyflag = 1;
		}
		
		dx = x1 - x0;
		dy = y1 - y0;
		incrE = 2 * dy;
		incrNE = 2 * (dy - dx);
		d = 2 * dy - dx;
		x = x0;
		y = y0;
		
		if (yflag && xyflag){OLED_DrawPoint(y, -x);}
		else if (yflag)		{OLED_DrawPoint(x, -y);}
		else if (xyflag)	{OLED_DrawPoint(y, x);}
		else				{OLED_DrawPoint(x, y);}
		
		while (x < x1)		//閬嶅巻X杞寸殑姣忎釜鐐?
		{
			x ++;
			if (d < 0)		//涓嬩竴涓偣鍦ㄥ綋鍓嶇偣涓滄柟
			{
				d += incrE;
			}
			else			//涓嬩竴涓偣鍦ㄥ綋鍓嶇偣涓滃寳鏂?
			{
				y ++;
				d += incrNE;
			}
			
			if (yflag && xyflag){OLED_DrawPoint(y, -x);}
			else if (yflag)		{OLED_DrawPoint(x, -y);}
			else if (xyflag)	{OLED_DrawPoint(y, x);}
			else				{OLED_DrawPoint(x, y);}
		}	
	}
}

void OLED_DrawRectangle(uint8_t X, uint8_t Y, uint8_t Width, uint8_t Height, uint8_t IsFilled)
{
	uint8_t i, j;
	if (!IsFilled)		//鎸囧畾鐭╁舰涓嶅～鍏?
	{
		for (i = X; i < X + Width; i ++)
		{
			OLED_DrawPoint(i, Y);
			OLED_DrawPoint(i, Y + Height - 1);
		}
		for (i = Y; i < Y + Height; i ++)
		{
			OLED_DrawPoint(X, i);
			OLED_DrawPoint(X + Width - 1, i);
		}
	}
	else				//鎸囧畾鐭╁舰濉厖
	{
		for (i = X; i < X + Width; i ++)
		{
			for (j = Y; j < Y + Height; j ++)
			{
				OLED_DrawPoint(i, j);
			}
		}
	}
}

void OLED_DrawTriangle(uint8_t X0, uint8_t Y0, uint8_t X1, uint8_t Y1, uint8_t X2, uint8_t Y2, uint8_t IsFilled)
{
	uint8_t minx = X0, miny = Y0, maxx = X0, maxy = Y0;
	uint8_t i, j;
	int16_t vx[] = {X0, X1, X2};
	int16_t vy[] = {Y0, Y1, Y2};
	
	if (!IsFilled)			//鎸囧畾涓夎褰笉濉厖
	{
		OLED_DrawLine(X0, Y0, X1, Y1);
		OLED_DrawLine(X0, Y0, X2, Y2);
		OLED_DrawLine(X1, Y1, X2, Y2);
	}
	else					//鎸囧畾涓夎褰㈠～鍏?
	{
		if (X1 < minx) {minx = X1;}
		if (X2 < minx) {minx = X2;}
		if (Y1 < miny) {miny = Y1;}
		if (Y2 < miny) {miny = Y2;}
		
		if (X1 > maxx) {maxx = X1;}
		if (X2 > maxx) {maxx = X2;}
		if (Y1 > maxy) {maxy = Y1;}
		if (Y2 > maxy) {maxy = Y2;}
		
		for (i = minx; i <= maxx; i ++)
		{
			for (j = miny; j <= maxy; j ++)
			{
				if (OLED_pnpoly(3, vx, vy, i, j)) {OLED_DrawPoint(i, j);}
			}
		}
	}
}

void OLED_DrawCircle(uint8_t X, uint8_t Y, uint8_t Radius, uint8_t IsFilled)
{
	int16_t x, y, d, j;
	
	
	d = 1 - Radius;
	x = 0;
	y = Radius;
	
	OLED_DrawPoint(X + x, Y + y);
	OLED_DrawPoint(X - x, Y - y);
	OLED_DrawPoint(X + y, Y + x);
	OLED_DrawPoint(X - y, Y - x);
	
	if (IsFilled)		//鎸囧畾鍦嗗～鍏?
	{
		for (j = -y; j < y; j ++)
		{
			OLED_DrawPoint(X, Y + j);
		}
	}
	
	while (x < y)		//閬嶅巻X杞寸殑姣忎釜鐐?
	{
		x ++;
		if (d < 0)		//涓嬩竴涓偣鍦ㄥ綋鍓嶇偣涓滄柟
		{
			d += 2 * x + 1;
		}
		else			//涓嬩竴涓偣鍦ㄥ綋鍓嶇偣涓滃崡鏂?
		{
			y --;
			d += 2 * (x - y) + 1;
		}
		
		OLED_DrawPoint(X + x, Y + y);
		OLED_DrawPoint(X + y, Y + x);
		OLED_DrawPoint(X - x, Y - y);
		OLED_DrawPoint(X - y, Y - x);
		OLED_DrawPoint(X + x, Y - y);
		OLED_DrawPoint(X + y, Y - x);
		OLED_DrawPoint(X - x, Y + y);
		OLED_DrawPoint(X - y, Y + x);
		
		if (IsFilled)	//鎸囧畾鍦嗗～鍏?
		{
			for (j = -y; j < y; j ++)
			{
				OLED_DrawPoint(X + x, Y + j);
				OLED_DrawPoint(X - x, Y + j);
			}
			
			for (j = -x; j < x; j ++)
			{
				OLED_DrawPoint(X - y, Y + j);
				OLED_DrawPoint(X + y, Y + j);
			}
		}
	}
}

void OLED_DrawEllipse(uint8_t X, uint8_t Y, uint8_t A, uint8_t B, uint8_t IsFilled)
{
	int16_t x, y, j;
	int16_t a = A, b = B;
	float d1, d2;
	
	
	x = 0;
	y = b;
	d1 = b * b + a * a * (-b + 0.5);
	
	if (IsFilled)	//鎸囧畾妞渾濉厖
	{
		for (j = -y; j < y; j ++)
		{
			OLED_DrawPoint(X, Y + j);
			OLED_DrawPoint(X, Y + j);
		}
	}
	
	OLED_DrawPoint(X + x, Y + y);
	OLED_DrawPoint(X - x, Y - y);
	OLED_DrawPoint(X - x, Y + y);
	OLED_DrawPoint(X + x, Y - y);
	
	while (b * b * (x + 1) < a * a * (y - 0.5))
	{
		if (d1 <= 0)		//涓嬩竴涓偣鍦ㄥ綋鍓嶇偣涓滄柟
		{
			d1 += b * b * (2 * x + 3);
		}
		else				//涓嬩竴涓偣鍦ㄥ綋鍓嶇偣涓滃崡鏂?
		{
			d1 += b * b * (2 * x + 3) + a * a * (-2 * y + 2);
			y --;
		}
		x ++;
		
		if (IsFilled)	//鎸囧畾妞渾濉厖
		{
			for (j = -y; j < y; j ++)
			{
				OLED_DrawPoint(X + x, Y + j);
				OLED_DrawPoint(X - x, Y + j);
			}
		}
		
		OLED_DrawPoint(X + x, Y + y);
		OLED_DrawPoint(X - x, Y - y);
		OLED_DrawPoint(X - x, Y + y);
		OLED_DrawPoint(X + x, Y - y);
	}
	
	d2 = b * b * (x + 0.5) * (x + 0.5) + a * a * (y - 1) * (y - 1) - a * a * b * b;
	
	while (y > 0)
	{
		if (d2 <= 0)		//涓嬩竴涓偣鍦ㄥ綋鍓嶇偣涓滄柟
		{
			d2 += b * b * (2 * x + 2) + a * a * (-2 * y + 3);
			x ++;
			
		}
		else				//涓嬩竴涓偣鍦ㄥ綋鍓嶇偣涓滃崡鏂?
		{
			d2 += a * a * (-2 * y + 3);
		}
		y --;
		
		if (IsFilled)	//鎸囧畾妞渾濉厖
		{
			for (j = -y; j < y; j ++)
			{
				OLED_DrawPoint(X + x, Y + j);
				OLED_DrawPoint(X - x, Y + j);
			}
		}
		
		OLED_DrawPoint(X + x, Y + y);
		OLED_DrawPoint(X - x, Y - y);
		OLED_DrawPoint(X - x, Y + y);
		OLED_DrawPoint(X + x, Y - y);
	}
}

void OLED_DrawArc(uint8_t X, uint8_t Y, uint8_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled)
{
	int16_t x, y, d, j;
	
	
	d = 1 - Radius;
	x = 0;
	y = Radius;
	
	if (OLED_IsInAngle(x, y, StartAngle, EndAngle))	{OLED_DrawPoint(X + x, Y + y);}
	if (OLED_IsInAngle(-x, -y, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y - y);}
	if (OLED_IsInAngle(y, x, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y + x);}
	if (OLED_IsInAngle(-y, -x, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y - x);}
	
	if (IsFilled)	//鎸囧畾鍦嗗姬濉厖
	{
		for (j = -y; j < y; j ++)
		{
			if (OLED_IsInAngle(0, j, StartAngle, EndAngle)) {OLED_DrawPoint(X, Y + j);}
		}
	}
	
	while (x < y)		//閬嶅巻X杞寸殑姣忎釜鐐?
	{
		x ++;
		if (d < 0)		//涓嬩竴涓偣鍦ㄥ綋鍓嶇偣涓滄柟
		{
			d += 2 * x + 1;
		}
		else			//涓嬩竴涓偣鍦ㄥ綋鍓嶇偣涓滃崡鏂?
		{
			y --;
			d += 2 * (x - y) + 1;
		}
		
		if (OLED_IsInAngle(x, y, StartAngle, EndAngle)) {OLED_DrawPoint(X + x, Y + y);}
		if (OLED_IsInAngle(y, x, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y + x);}
		if (OLED_IsInAngle(-x, -y, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y - y);}
		if (OLED_IsInAngle(-y, -x, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y - x);}
		if (OLED_IsInAngle(x, -y, StartAngle, EndAngle)) {OLED_DrawPoint(X + x, Y - y);}
		if (OLED_IsInAngle(y, -x, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y - x);}
		if (OLED_IsInAngle(-x, y, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y + y);}
		if (OLED_IsInAngle(-y, x, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y + x);}
		
		if (IsFilled)	//鎸囧畾鍦嗗姬濉厖
		{
			for (j = -y; j < y; j ++)
			{
				if (OLED_IsInAngle(x, j, StartAngle, EndAngle)) {OLED_DrawPoint(X + x, Y + j);}
				if (OLED_IsInAngle(-x, j, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y + j);}
			}
			
			for (j = -x; j < x; j ++)
			{
				if (OLED_IsInAngle(-y, j, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y + j);}
				if (OLED_IsInAngle(y, j, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y + j);}
			}
		}
	}
}




