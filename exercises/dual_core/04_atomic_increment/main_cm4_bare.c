#include "stm32wl55xx.h"

// 04_atomic_increment -- M4 (CPU1) side, bare metal.
//
// Both cores hammer the SAME integer (`counter`) in shared SRAM2, each
// incrementing it N times per round. With a non-atomic `counter++` increments
// are LOST to the load/modify/store interleave; serialising the ++ with the
// HSEM hardware semaphore fixes it. (The M0+ has no LDREX/STREX, so HSEM -- not
// a lock-free atomic -- is the chip's way to do M4<->M0+ mutual exclusion.)
//
//   M4  : drive the round handshake, increment its N-share, watch button B1
//         (PA0, active-low) and toggle the RACY<->ATOMIC mode on each press.
//   M0+ : increment its N-share, then print expected/actual/lost per round.
//
// The M4 owns the bootstrap (PWR.C2BOOT) that releases the M0+. It reaches the
// shared block at the absolute SRAM2 base 0x20008000 through a volatile struct
// pointer; the M0+ image is what actually DEFINES the block there (see
// main_cm0p_bare.c). The layout below must match the M0+'s `shared_t`.

#define N 1000000u   // increments PER CORE per round; total should reach 2*N

typedef struct {
    volatile uint32_t counter;   // the contended integer (both cores ++)
    volatile uint32_t mode;      // 0 = RACY (non-atomic), 1 = ATOMIC
    volatile uint32_t round;     // bumped by M4 to start each round
    volatile uint32_t m4_done;   // M4 finished its N increments this round
    volatile uint32_t m0p_done;  // M0+ finished its N increments this round
    volatile uint32_t m0p_ready; // M0+ has booted & is waiting for round 1
} shared_t;

#define SHARED ((volatile shared_t*)0x20008000UL)

// HSEM semaphore index used to guard `counter`. Both cores must agree on it.
#define HSEM_ID 0u

static void delay(volatile uint32_t count) { while (count--); }

// 1-step HSEM lock: reading RLR[id] atomically attempts the lock; it reads back
// as (LOCK | our COREID) iff we now hold it. HSEM_CR_COREID_CURRENT is this
// core's ID (CPU1 on the M4), resolved from the CORE_CM4 define.
static inline void hsem_lock(void) {
    while (HSEM->RLR[HSEM_ID] != (HSEM_RLR_LOCK | HSEM_CR_COREID_CURRENT)) { /* spin */ }
}
static inline void hsem_unlock(void) {
    HSEM->R[HSEM_ID] = HSEM_CR_COREID_CURRENT;   // process 0, our core -> release
}

// Do N increments of the shared counter, serialised by HSEM or not per `atomic`.
static void do_increments(int atomic) {
    if (atomic) {
        for (uint32_t i = 0; i < N; i++) {
            hsem_lock();
            SHARED->counter = SHARED->counter + 1u;
            hsem_unlock();
        }
    } else {
        // Deliberately unguarded: load; add; store, interruptible by the M0+.
        for (uint32_t i = 0; i < N; i++)
            SHARED->counter = SHARED->counter + 1u;
    }
}

int main(void) {
    // Initialise the whole shared block to a defined state before booting CPU2,
    // so the M0+ never observes garbage on its first read.
    SHARED->counter   = 0;
    SHARED->mode      = 0;           // start in RACY mode (the interesting one)
    SHARED->round     = 0;
    SHARED->m4_done   = 0;
    SHARED->m0p_done  = 0;
    SHARED->m0p_ready = 0;

    // Clock the HSEM peripheral from the M4 (AHB3). The M0+ clocks it from its
    // own C2AHB3ENR; both cores need it for the shared semaphore to work.
    RCC->AHB3ENR |= RCC_AHB3ENR_HSEMEN;

    // Boot the M0+: SBRV already points at flash bank 2 where the M0+ image is
    // linked, so setting C2BOOT starts CPU2 from its reset vector.
    PWR->CR4 |= PWR_CR4_C2BOOT;

    // B1 = PA0 as input with a pull-up: idle HIGH, reads LOW while pressed.
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    GPIOA->MODER &= ~GPIO_MODER_MODE0;                 // input
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD0;
    GPIOA->PUPDR |=  GPIO_PUPDR_PUPD0_0;               // pull-up

    int prev = (GPIOA->IDR & GPIO_IDR_ID0) ? 1 : 0;

    // Wait for the M0+ to finish booting and sample round 0 before we start
    // bumping `round`. Without this the M0+ could sample `round` after our first
    // bump and then wait forever for a transition we never make (we would be
    // blocked on its `m0p_done`). This handshake makes round 1 race-free.
    while (!SHARED->m0p_ready) { /* spin */ }

    while (1) {
        // ---- start a new round ------------------------------------------
        // Zero the counter and clear both done-flags, THEN bump `round`. The
        // M0+ is spinning on `round` changing, so it must see the cleared state
        // before it sees the new round number -- write order matters, and the
        // `volatile` stores keep that order.
        SHARED->counter  = 0;
        SHARED->m4_done  = 0;
        SHARED->m0p_done = 0;
        SHARED->round   += 1;

        int atomic = SHARED->mode ? 1 : 0;

        // Our share of the work, then announce done and STOP touching counter
        // (so we don't keep contending while the M0+ is still incrementing).
        do_increments(atomic);
        SHARED->m4_done = 1;

        // Wait for the M0+ to finish its share too; now the total is final and
        // the M0+ will print it.
        while (!SHARED->m0p_done) { /* spin */ }

        // ---- poll the button between rounds, to toggle the mode ----------
        // We only check the button here (not during the hot increment loop) to
        // keep the increment window as tight and contended as possible.
        for (int t = 0; t < 100; t++) {
            int level = (GPIOA->IDR & GPIO_IDR_ID0) ? 1 : 0;
            if (prev == 1 && level == 0) {             // press edge
                SHARED->mode ^= 1u;                    // RACY <-> ATOMIC
            }
            prev = level;
            delay(2000);                               // crude debounce / poll
        }
    }
}
