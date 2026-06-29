// Minimal glue needed by every HAL-based exercise, kept in one shared place so
// the exercise main.c files stay focused on the lesson.
//
// HAL_Delay() works by polling a millisecond counter that is incremented from
// the SysTick interrupt. HAL_Init() starts the 1 ms SysTick, but the vector
// table (in startup_stm32wl55jcix.s) routes the tick to SysTick_Handler -- so
// we must define it here and forward to HAL_IncTick(). Without this, any
// HAL_Delay() would hang forever.
#include "stm32wlxx_hal.h"

void SysTick_Handler(void) {
    HAL_IncTick();
}
