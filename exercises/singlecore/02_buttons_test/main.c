#include "stm32wl55xx.h"
#include <string.h>

// Buttons diagnostic: watch all three user buttons and print every state
// change to the UART, reporting the actual pin LEVEL on each transition. This
// makes no assumption about active-high vs active-low or which solder-bridge
// configuration the board uses -- it just tells you what each pin does when you
// press, which is exactly what you need to wire up the other button exercises.
//
// NUCLEO-WL55JC user buttons (per UM2592):
//   B1 = PA0 (default) / PC13 (alt)   B2 = PA1   B3 = PC6
// We watch PA0, PA1, PC6 and PC13 so B1 is caught in either bridge config.
//
// Output (9600 8N1 on the ST-LINK VCP, /dev/ttyACM0):
//   B1 (PA0) HIGH
//   B3 (PC6) LOW
//   ...

void delay(volatile uint32_t count) { while (count--); }

void LPUART1_Init(void) {
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;
    RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;

    // PA2 (TX) / PA3 (RX) -> AF8, routed to the ST-LINK virtual COM port.
    GPIOA->MODER &= ~(GPIO_MODER_MODE2 | GPIO_MODER_MODE3);
    GPIOA->MODER |= (GPIO_MODER_MODE2_1 | GPIO_MODER_MODE3_1);
    GPIOA->AFR[0] &= ~(GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3);
    GPIOA->AFR[0] |= (8 << GPIO_AFRL_AFSEL2_Pos) | (8 << GPIO_AFRL_AFSEL3_Pos);

    LPUART1->BRR = 106667;                 // 256 * 4MHz / 9600
    LPUART1->CR1 |= USART_CR1_TE | USART_CR1_UE;
}

void uart_puts(const char* s) {
    for (size_t i = 0; i < strlen(s); i++) {
        // No TX FIFO enabled -> poll TXE_TXFNF (not TXFE).
        while (!(LPUART1->ISR & USART_ISR_TXE_TXFNF));
        LPUART1->TDR = s[i];
    }
}

// One button we monitor: a label, the GPIO port, and the pin's IDR mask.
typedef struct {
    const char*       name;
    GPIO_TypeDef*     port;
    uint32_t          idr_mask;
} button_t;

int main(void) {
    LPUART1_Init();

    // Enable clocks for ports A and C (B inputs live on A and C).
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOCEN;

    // PA0, PA1 -> input (clear MODER bits); PC6, PC13 -> input.
    GPIOA->MODER &= ~(GPIO_MODER_MODE0 | GPIO_MODER_MODE1);
    GPIOC->MODER &= ~(GPIO_MODER_MODE6 | GPIO_MODER_MODE13);

    // Enable internal pull-UPS so every monitored pin has a defined idle level
    // (HIGH) instead of floating. A button that shorts the pin to GND then reads
    // LOW while pressed. (If a button is actually active-high on your board, you
    // will simply see the inverse -- the point of this test is to find out.)
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD0 | GPIO_PUPDR_PUPD1);
    GPIOA->PUPDR |=  (GPIO_PUPDR_PUPD0_0 | GPIO_PUPDR_PUPD1_0);
    GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPD6 | GPIO_PUPDR_PUPD13);
    GPIOC->PUPDR |=  (GPIO_PUPDR_PUPD6_0 | GPIO_PUPDR_PUPD13_0);

    button_t buttons[] = {
        { "B1 (PA0) ",  GPIOA, GPIO_IDR_ID0  },
        { "B2 (PA1) ",  GPIOA, GPIO_IDR_ID1  },
        { "B3 (PC6) ",  GPIOC, GPIO_IDR_ID6  },
        { "B1 (PC13)",  GPIOC, GPIO_IDR_ID13 },
    };
    const int N = sizeof(buttons) / sizeof(buttons[0]);

    // Remember each pin's last seen level so we only print on change.
    int last[sizeof(buttons) / sizeof(buttons[0])];
    for (int i = 0; i < N; i++) {
        last[i] = (buttons[i].port->IDR & buttons[i].idr_mask) ? 1 : 0;
    }

    uart_puts("buttons_test: press B1/B2/B3 -- transitions print below\r\n");

    while (1) {
        for (int i = 0; i < N; i++) {
            int level = (buttons[i].port->IDR & buttons[i].idr_mask) ? 1 : 0;
            if (level != last[i]) {
                last[i] = level;
                uart_puts(buttons[i].name);
                uart_puts(level ? " HIGH\r\n" : " LOW\r\n");
            }
        }
        delay(20000); // crude debounce / poll rate
    }
}
