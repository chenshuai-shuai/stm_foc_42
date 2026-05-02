#ifndef DRV_GPIO_H
#define DRV_GPIO_H

#include <stdbool.h>

void drvGpioInit(void);
void drvGatePinInit(void);
void drvGatePinWrite(bool enabled);
bool drvGatePinRead(void);

#endif /* DRV_GPIO_H */
