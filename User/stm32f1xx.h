#ifndef STM32F1XX_H
#define STM32F1XX_H

#include "board.h"
#include "stm32f10x.h"

#ifndef __STATIC_INLINE
#define __STATIC_INLINE static __INLINE
#endif

#ifndef __STATIC_FORCEINLINE
#if defined(__CC_ARM)
#define __STATIC_FORCEINLINE static __forceinline
#else
#define __STATIC_FORCEINLINE static __INLINE
#endif
#endif

#ifndef __get_IPSR
__STATIC_INLINE uint32_t __get_IPSR(void) {
#if defined(__CC_ARM)
  register uint32_t ipsr __ASM("ipsr");
  return ipsr;
#else
  return 0U;
#endif
}
#endif

#ifndef DWT_BASE
typedef struct {
  __IO uint32_t CTRL;
  __IO uint32_t CYCCNT;
} DWT_Type;

#define DWT_BASE                    (0xE0001000UL)
#define DWT                         ((DWT_Type *)DWT_BASE)
#define DWT_CTRL_CYCCNTENA_Pos      0U
#define DWT_CTRL_CYCCNTENA_Msk      (1UL << DWT_CTRL_CYCCNTENA_Pos)
#endif

#endif /* STM32F1XX_H */
