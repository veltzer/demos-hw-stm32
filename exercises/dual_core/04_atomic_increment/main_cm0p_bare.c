#include "stm32wl55xx.h"
#include <string.h>

// 04_atomic_increment -- M0+ (CPU2) side, bare metal.
//
// Both cores hammer the SAME integer (`counter`) in shared SRAM2, each
// incrementing it N times per round. With a non-atomic `counter++` the two
// cores' load/modify/store sequences interleave and increments are LOST; with
// a properly serialised increment none are. The M4 toggles the mode on a
// button press; this core is the second incrementer AND the printer.
//
// WHY HSEM (not LDREX/STREX): the Cortex-M0+ is ARMv6-M and has NO exclusive
// load/store (LDREX/STREX) -- so __atomic_fetch_add on a shared word cannot be
// done lock-free on this core, and there is no LDREX-based atomic that the M4
// and M0+ could even share (different architectures). The STM32WL's answer for
// M4<->M0+ mutual exclusion is the HSEM hardware semaphore peripheral: take an
// HSEM lock, do the plain ++, release. That is what "atomic mode" uses here.
//
// This core OWNS the shared struct: it is defined here in the ".shared"
// section, which the M0+ linker pins to the base of SRAM2 (0x20008000). Both
// cores reach it at that absolute address (the M4 pokes it directly -- see
// main_cm4_bare.c).
//
// Everything is `volatile` because the OTHER core mutates it: the compiler must
// re-read from / write to SRAM, never cache in a register. NOTE: `volatile`
// does NOT make `counter++` atomic -- it only forbids caching. The load/add/
// store is still three separate instructions and still interruptible by the
// other core. That is exactly the bug this exercise demonstrates.

#define N 1000000u   // increments PER CORE per round; total should reach 2*N

// The shared inter-core block, pinned to SRAM2 base by the linker (.shared).
// Layout must match the M4's volatile struct pointer in main_cm4_bare.c.
typedef struct {
    volatile uint32_t counter;   // the contended integer (both cores ++)
    volatile uint32_t mode;      // 0 = RACY (non-atomic), 1 = ATOMIC
    volatile uint32_t round;     // bumped by M4 to start each round
    volatile uint32_t m4_done;   // M4 finished its N increments this round
    volatile uint32_t m0p_done;  // M0+ finished its N increments this round
} shared_t;

volatile shared_t shared __attribute__((section(".shared")));

// HSEM semaphore index used to guard `counter`. Both cores must agree on it.
#define HSEM_ID 0u

static void delay(volatile uint32_t count) { while (count--); }

// 1-step HSEM lock: reading RLR[id] atomically attempts the lock; it reads back
// as (LOCK | our COREID) iff we now hold it. Spin until we do. HSEM_CR_COREID_CURRENT
// (HSEM_CR_COREID_CURRENT) is this core's ID -- CPU2 here, CPU1 on the M4 --
// resolved from the CORE_CM0PLUS / CORE_CM4 define.
static inline void hsem_lock(void) {
    while (HSEM->RLR[HSEM_ID] != (HSEM_RLR_LOCK | HSEM_CR_COREID_CURRENT)) { /* spin */ }
}
static inline void hsem_unlock(void) {
    HSEM->R[HSEM_ID] = HSEM_CR_COREID_CURRENT;   // process 0, our core -> release
}

// --- minimal LPUART1 bring-up (PA2 TX @ 9600, routed to the ST-LINK VCP) ---
static void LPUART1_Init(void) {
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;
    RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;

    GPIOA->MODER &= ~(GPIO_MODER_MODE2 | GPIO_MODER_MODE3);
    GPIOA->MODER |= (GPIO_MODER_MODE2_1 | GPIO_MODER_MODE3_1);
    GPIOA->AFR[0] &= ~(GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3);
    GPIOA->AFR[0] |= (8 << GPIO_AFRL_AFSEL2_Pos) | (8 << GPIO_AFRL_AFSEL3_Pos);

    LPUART1->BRR = 106667;                 // 256 * 4MHz / 9600
    LPUART1->CR1 |= USART_CR1_TE | USART_CR1_UE;
}

static void LPUART1_SendString(const char* str) {
    for (size_t i = 0; i < strlen(str); i++) {
        while (!(LPUART1->ISR & USART_ISR_TXE_TXFNF));
        LPUART1->TDR = str[i];
    }
}

// Append unsigned `v` (decimal) to `dst` at *pos, advancing *pos.
static void append_u32(char* dst, size_t* pos, uint32_t v) {
    char tmp[10];
    int n = 0;
    if (v == 0) { dst[(*pos)++] = '0'; return; }
    while (v) { tmp[n++] = (char)('0' + v % 10); v /= 10; }
    while (n--) dst[(*pos)++] = tmp[n];
}

// Do N increments of the shared counter, serialised by HSEM or not per `mode`.
static void do_increments(int atomic) {
    if (atomic) {
        // HSEM-guarded: only one core is inside load/add/store at a time, so no
        // increment is ever lost.
        for (uint32_t i = 0; i < N; i++) {
            hsem_lock();
            shared.counter = shared.counter + 1u;
            hsem_unlock();
        }
    } else {
        // Deliberately unguarded: load; add; store, interruptible by the M4.
        for (uint32_t i = 0; i < N; i++)
            shared.counter = shared.counter + 1u;
    }
}

int main(void) {
    // The M0+ has its own AHB3 clock-enable register (C2AHB3ENR). Clock the
    // HSEM peripheral from this core too, else its registers read/write as 0.
    RCC->C2AHB3ENR |= RCC_C2AHB3ENR_HSEMEN;

    LPUART1_Init();

    // Track the round we last processed so we run exactly once per M4 round.
    uint32_t last_round = shared.round;

    while (1) {
        // Wait for the M4 to start a new round (it zeroes counter, clears the
        // done-flags, then bumps `round`).
        while (shared.round == last_round) { /* spin */ }
        last_round = shared.round;

        int atomic = shared.mode ? 1 : 0;

        // Our share of the work, then announce we are done and STOP touching
        // counter -- otherwise we would keep contending while the M4 is still
        // going and the "expected == 2*N" accounting would break.
        do_increments(atomic);
        shared.m0p_done = 1;

        // Wait for the M4 to finish its share too, then the round total is final.
        while (!shared.m4_done) { /* spin */ }

        // Print the verdict for this round.
        char line[80];
        size_t p = 0;
        const char* pre = atomic ? "mode=ATOMIC expected=" : "mode=RACY   expected=";
        memcpy(line + p, pre, strlen(pre)); p += strlen(pre);
        append_u32(line, &p, 2u * N);
        memcpy(line + p, " actual=", 8); p += 8;
        append_u32(line, &p, shared.counter);
        memcpy(line + p, " lost=", 6); p += 6;
        append_u32(line, &p, 2u * N - shared.counter);
        line[p++] = '\r'; line[p++] = '\n'; line[p] = '\0';
        LPUART1_SendString(line);

        // Small gap so the output is readable; the M4 paces the rounds anyway.
        delay(200000);
    }
}
