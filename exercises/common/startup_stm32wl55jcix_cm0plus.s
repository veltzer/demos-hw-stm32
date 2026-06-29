/**
  ******************************************************************************
  * @file      startup_stm32wl55jcix_cm0plus.s
  * @brief     STM32WL55xx devices Cortex-M0+ (CPU2) vector table for GCC.
  *            Mirrors the Cortex-M4 startup (startup_stm32wl55jcix.s) but with:
  *                - .cpu cortex-m0plus
  *                - the M0+ interrupt vector table (32 device IRQs, not 61),
  *                  matching the CORE_CM0PLUS IRQn_Type enum in stm32wl55xx.h.
  *            Reset_Handler is identical in spirit: set SP, SystemInit, copy
  *            .data, zero .bss, run ctors, call main().
  *
  *            Cortex-M0+ has no MemManage/BusFault/UsageFault/DebugMon
  *            exceptions (only NMI, HardFault, SVC, PendSV, SysTick), so those
  *            vector slots are Reserved (0).
  ******************************************************************************
  */

.syntax unified
.cpu cortex-m0plus
.fpu softvfp
.thumb

.global g_pfnVectors
.global Default_Handler

.word _sidata
.word _sdata
.word _edata
.word _sbss
.word _ebss

  .section .text.Reset_Handler
  .weak Reset_Handler
  .type Reset_Handler, %function
Reset_Handler:
  ldr   r0, =_estack
  mov   sp, r0          /* set stack pointer */

/* Call the clock system initialization function. */
  bl  SystemInit

/* Copy the data segment initializers from flash to SRAM */
  ldr r0, =_sdata
  ldr r1, =_edata
  ldr r2, =_sidata
  movs r3, #0
  b LoopCopyDataInit

CopyDataInit:
  ldr r4, [r2, r3]
  str r4, [r0, r3]
  adds r3, r3, #4

LoopCopyDataInit:
  adds r4, r0, r3
  cmp r4, r1
  bcc CopyDataInit

/* Zero fill the bss segment. */
  ldr r2, =_sbss
  ldr r4, =_ebss
  movs r3, #0
  b LoopFillZerobss

FillZerobss:
  str  r3, [r2]
  adds r2, r2, #4

LoopFillZerobss:
  cmp r2, r4
  bcc FillZerobss

/* Call static constructors */
  bl __libc_init_array
/* Call the application's entry point. */
  bl main

LoopForever:
  b LoopForever

  .size Reset_Handler, .-Reset_Handler

  .section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
  b Infinite_Loop
  .size Default_Handler, .-Default_Handler

/******************************************************************************
* The STM32WL55xx Cortex-M0+ vector table.
******************************************************************************/
  .section .isr_vector,"a",%progbits
  .type g_pfnVectors, %object

g_pfnVectors:
  .word _estack
  .word Reset_Handler
  .word NMI_Handler
  .word HardFault_Handler
  .word 0
  .word 0
  .word 0
  .word 0
  .word 0
  .word 0
  .word 0
  .word SVC_Handler
  .word 0
  .word 0
  .word PendSV_Handler
  .word SysTick_Handler
  /* STM32WLxx M0+ device interrupts (IRQn 0..31) */
  .word TZIC_ILA_IRQHandler             /* 0  Security illegal access            */
  .word PVD_PVM_IRQHandler              /* 1  PVD/PVM through EXTI               */
  .word RTC_LSECSS_IRQHandler           /* 2  RTC + LSECSS                       */
  .word RCC_FLASH_C1SEV_IRQHandler      /* 3  RCC, FLASH, CPU1 SEV               */
  .word EXTI1_0_IRQHandler              /* 4  EXTI line 1:0                      */
  .word EXTI3_2_IRQHandler              /* 5  EXTI line 3:2                      */
  .word EXTI15_4_IRQHandler             /* 6  EXTI line 15:4                     */
  .word ADC_COMP_DAC_IRQHandler         /* 7  ADC/COMP/DAC                       */
  .word DMA1_Channel1_2_3_IRQHandler    /* 8  DMA1 ch 1,2,3                      */
  .word DMA1_Channel4_5_6_7_IRQHandler  /* 9  DMA1 ch 4,5,6,7                    */
  .word DMA2_DMAMUX1_OVR_IRQHandler     /* 10 DMA2 + DMAMUX1 overrun             */
  .word LPTIM1_IRQHandler               /* 11 LPTIM1                             */
  .word LPTIM2_IRQHandler               /* 12 LPTIM2                             */
  .word LPTIM3_IRQHandler               /* 13 LPTIM3                             */
  .word TIM1_IRQHandler                 /* 14 TIM1                               */
  .word TIM2_IRQHandler                 /* 15 TIM2                               */
  .word TIM16_IRQHandler                /* 16 TIM16                             */
  .word TIM17_IRQHandler                /* 17 TIM17                             */
  .word IPCC_C2_RX_C2_TX_IRQHandler     /* 18 IPCC RX occupied / TX free        */
  .word HSEM_IRQHandler                 /* 19 HSEM                              */
  .word RNG_IRQHandler                  /* 20 RNG                               */
  .word AES_PKA_IRQHandler              /* 21 AES + PKA                         */
  .word I2C1_IRQHandler                 /* 22 I2C1                              */
  .word I2C2_IRQHandler                 /* 23 I2C2                              */
  .word I2C3_IRQHandler                 /* 24 I2C3                              */
  .word SPI1_IRQHandler                 /* 25 SPI1                              */
  .word SPI2_IRQHandler                 /* 26 SPI2                              */
  .word USART1_IRQHandler               /* 27 USART1                            */
  .word USART2_IRQHandler               /* 28 USART2                            */
  .word LPUART1_IRQHandler              /* 29 LPUART1                           */
  .word SUBGHZSPI_IRQHandler            /* 30 SUBGHZSPI                         */
  .word SUBGHZ_Radio_IRQHandler         /* 31 SUBGHZ Radio                      */

  .size g_pfnVectors, .-g_pfnVectors

/*******************************************************************************
* Weak aliases for each handler -> Default_Handler. Any same-named function
* in the application overrides these.
*******************************************************************************/
  .weak NMI_Handler
  .thumb_set NMI_Handler,Default_Handler

  .weak HardFault_Handler
  .thumb_set HardFault_Handler,Default_Handler

  .weak SVC_Handler
  .thumb_set SVC_Handler,Default_Handler

  .weak PendSV_Handler
  .thumb_set PendSV_Handler,Default_Handler

  .weak SysTick_Handler
  .thumb_set SysTick_Handler,Default_Handler

  .weak TZIC_ILA_IRQHandler
  .thumb_set TZIC_ILA_IRQHandler,Default_Handler

  .weak PVD_PVM_IRQHandler
  .thumb_set PVD_PVM_IRQHandler,Default_Handler

  .weak RTC_LSECSS_IRQHandler
  .thumb_set RTC_LSECSS_IRQHandler,Default_Handler

  .weak RCC_FLASH_C1SEV_IRQHandler
  .thumb_set RCC_FLASH_C1SEV_IRQHandler,Default_Handler

  .weak EXTI1_0_IRQHandler
  .thumb_set EXTI1_0_IRQHandler,Default_Handler

  .weak EXTI3_2_IRQHandler
  .thumb_set EXTI3_2_IRQHandler,Default_Handler

  .weak EXTI15_4_IRQHandler
  .thumb_set EXTI15_4_IRQHandler,Default_Handler

  .weak ADC_COMP_DAC_IRQHandler
  .thumb_set ADC_COMP_DAC_IRQHandler,Default_Handler

  .weak DMA1_Channel1_2_3_IRQHandler
  .thumb_set DMA1_Channel1_2_3_IRQHandler,Default_Handler

  .weak DMA1_Channel4_5_6_7_IRQHandler
  .thumb_set DMA1_Channel4_5_6_7_IRQHandler,Default_Handler

  .weak DMA2_DMAMUX1_OVR_IRQHandler
  .thumb_set DMA2_DMAMUX1_OVR_IRQHandler,Default_Handler

  .weak LPTIM1_IRQHandler
  .thumb_set LPTIM1_IRQHandler,Default_Handler

  .weak LPTIM2_IRQHandler
  .thumb_set LPTIM2_IRQHandler,Default_Handler

  .weak LPTIM3_IRQHandler
  .thumb_set LPTIM3_IRQHandler,Default_Handler

  .weak TIM1_IRQHandler
  .thumb_set TIM1_IRQHandler,Default_Handler

  .weak TIM2_IRQHandler
  .thumb_set TIM2_IRQHandler,Default_Handler

  .weak TIM16_IRQHandler
  .thumb_set TIM16_IRQHandler,Default_Handler

  .weak TIM17_IRQHandler
  .thumb_set TIM17_IRQHandler,Default_Handler

  .weak IPCC_C2_RX_C2_TX_IRQHandler
  .thumb_set IPCC_C2_RX_C2_TX_IRQHandler,Default_Handler

  .weak HSEM_IRQHandler
  .thumb_set HSEM_IRQHandler,Default_Handler

  .weak RNG_IRQHandler
  .thumb_set RNG_IRQHandler,Default_Handler

  .weak AES_PKA_IRQHandler
  .thumb_set AES_PKA_IRQHandler,Default_Handler

  .weak I2C1_IRQHandler
  .thumb_set I2C1_IRQHandler,Default_Handler

  .weak I2C2_IRQHandler
  .thumb_set I2C2_IRQHandler,Default_Handler

  .weak I2C3_IRQHandler
  .thumb_set I2C3_IRQHandler,Default_Handler

  .weak SPI1_IRQHandler
  .thumb_set SPI1_IRQHandler,Default_Handler

  .weak SPI2_IRQHandler
  .thumb_set SPI2_IRQHandler,Default_Handler

  .weak USART1_IRQHandler
  .thumb_set USART1_IRQHandler,Default_Handler

  .weak USART2_IRQHandler
  .thumb_set USART2_IRQHandler,Default_Handler

  .weak LPUART1_IRQHandler
  .thumb_set LPUART1_IRQHandler,Default_Handler

  .weak SUBGHZSPI_IRQHandler
  .thumb_set SUBGHZSPI_IRQHandler,Default_Handler

  .weak SUBGHZ_Radio_IRQHandler
  .thumb_set SUBGHZ_Radio_IRQHandler,Default_Handler

  .weak SystemInit
