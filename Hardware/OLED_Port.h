#ifndef __OLED_PORT_H
#define __OLED_PORT_H

#include <stdint.h>

void OLED_Port_Init(void);
void OLED_Port_WriteCommand(uint8_t Command);
void OLED_Port_WriteData(const uint8_t *Data, uint8_t Count);

#endif
