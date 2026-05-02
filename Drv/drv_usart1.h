#ifndef DRV_USART1_H
#define DRV_USART1_H

#include <stdbool.h>

void drvUsart1Init(void);
bool drvUsart1IsReady(void);
void drvUsart1WriteChar(char ch);
void drvUsart1Write(const char *text);

#endif /* DRV_USART1_H */
