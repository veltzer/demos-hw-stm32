// 09_os_coop_single_stack -- HAL. Compare with main_bare.c.
//
// The same naive coop1.cc port (peer-to-peer setjmp/longjmp, no scheduler, one
// shared stack), but LED and timing go through the HAL. The setjmp/longjmp
// mechanism is plain C, identical to the bare version; only the task bodies
// (HAL_GPIO_TogglePin / HAL_Delay vs register writes) differ.
#include "stm32wlxx_hal.h"
#include <setjmp.h>

void SysTick_Handler(void) { HAL_IncTick(); } // so HAL_Delay works

static jmp_buf task1_context;
static jmp_buf task2_context;
static jmp_buf main_context;

static void task_one(void) {
    if (setjmp(task1_context) == 0) {
        longjmp(main_context, 1);
    }
    while (1) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_15); // LD1
        HAL_Delay(250);
        if (setjmp(task1_context) == 0) {
            longjmp(task2_context, 1);
        }
    }
}

static void task_two(void) {
    if (setjmp(task2_context) == 0) {
        longjmp(main_context, 1);
    }
    while (1) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_15); // LD1
        HAL_Delay(250);
        if (setjmp(task2_context) == 0) {
            longjmp(task1_context, 1);
        }
    }
}

int main(void) {
    HAL_Init();

    // LD1 = PB15 output.
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef led = {0};
    led.Pin   = GPIO_PIN_15;
    led.Mode  = GPIO_MODE_OUTPUT_PP;
    led.Pull  = GPIO_NOPULL;
    led.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &led);

    if (setjmp(main_context) == 0) {
        task_one();
    }
    if (setjmp(main_context) == 0) {
        task_two();
    }

    longjmp(task1_context, 1);

    while (1); // unreachable
}
