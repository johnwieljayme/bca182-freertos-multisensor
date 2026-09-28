/*
 * Custom FreeRTOS Cortex-M3 port -- Wokwi-safe variant (no PendSV).
 *
 * Design:
 *   - SVC_Handler is used ONLY for the first task launch, and it is also
 *     where SysTick is armed (see below).
 *   - All context switches happen in SysTick_Handler.
 *   - portYIELD() sets ulPortSwitchPending and waits (outside critical
 *     sections) until SysTick has performed the switch. Latency <= 1 tick.
 *   - A yield requested inside a critical section is honored when the
 *     outermost critical section exits (avoids SVC with PRIMASK set).
 *   - A yield requested from an ISR only sets the flag.
 *
 * SysTick is deliberately NOT started in vPortSetupTimerInterrupt(). Wokwi
 * does not mask SysTick with PRIMASK, so a tick could arrive before the first
 * task is launched, while the CPU is still in thread mode on MSP with an
 * uninitialised PSP. The handler would then "save" a bogus stack pointer into
 * the first task's TCB. So the SVC handler arms SysTick as its last step,
 * right before returning into the first task.
 */

#include "FreeRTOS.h"
#include "task.h"

#ifndef PORT_WOKWI_SAFE_MACRO_HEADER
#error "Stock portmacro.h was picked up instead of include/portmacro.h"
#endif

/* Set by yields; consumed (cleared) by SysTick when it performs a switch. */
volatile uint32_t ulPortSwitchPending = 0;

/* Diagnostics, printed by main.cpp. */
volatile uint32_t g_portFirstLaunchCount = 0;
volatile uint32_t g_portYieldCount       = 0;
volatile uint32_t g_portTickSwitchCount  = 0;

static volatile UBaseType_t uxCriticalNesting = 0xaaaaaaaa;

#define portINITIAL_XPSR            ( 0x01000000UL )
#define portSTART_ADDRESS_MASK      ( ( StackType_t ) 0xfffffffeUL )

#define portNVIC_SYSTICK_CTRL_REG          ( *( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG          ( *( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE_REG ( *( ( volatile uint32_t * ) 0xe000e018 ) )
#define portNVIC_SYSTICK_CLK_BIT           ( 1UL << 2UL )
#define portNVIC_SYSTICK_INT_BIT           ( 1UL << 1UL )
#define portNVIC_SYSTICK_ENABLE_BIT        ( 1UL << 0UL )

static void prvTaskExitError( void )
{
    volatile uint32_t ulDummy = 0UL;
    portDISABLE_INTERRUPTS();
    while( ulDummy == 0UL )
    {
    }
}

/* For the status line printed by main.cpp. */
uint32_t ulPortGetCriticalNesting( void )
{
    return ( uint32_t ) uxCriticalNesting;
}

/*-----------------------------------------------------------*/

StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                      TaskFunction_t pxCode,
                                      void * pvParameters )
{
    pxTopOfStack--;
    *pxTopOfStack = portINITIAL_XPSR;
    pxTopOfStack--;
    *pxTopOfStack = ( ( StackType_t ) pxCode ) & portSTART_ADDRESS_MASK;
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) prvTaskExitError;
    pxTopOfStack -= 5;                              /* R12, R3, R2, R1 + R0 slot */
    *pxTopOfStack = ( StackType_t ) pvParameters;   /* R0 */
    pxTopOfStack -= 8;                              /* R11..R4 */

    return pxTopOfStack;
}

/*-----------------------------------------------------------*/

/* portYIELD(): request a switch and wait for SysTick to perform it. */
void vPortYield( void )
{
    g_portYieldCount++;
    ulPortSwitchPending = 1;

    if( uxCriticalNesting == 0 )
    {
        /* Interrupts are enabled here. SysTick will switch away within one
         * tick and clear the flag; when this task is resumed the flag is 0. */
        while( ulPortSwitchPending != 0 )
        {
            __asm volatile ( " nop " );
        }
    }
    /* Inside a critical section: vPortExitCritical() waits instead. */
}

/*-----------------------------------------------------------*/

/* Wokwi does not mask SysTick when PRIMASK is set, so critical sections also
 * gate the SysTick interrupt-enable bit. A tick boundary passing while gated
 * is not replayed (a few ms of drift over long runs; acceptable for this lab). */
void vPortEnterCritical( void )
{
    portDISABLE_INTERRUPTS();
    portNVIC_SYSTICK_CTRL_REG &= ~portNVIC_SYSTICK_INT_BIT;
    uxCriticalNesting++;
}

void vPortExitCritical( void )
{
    uxCriticalNesting--;
    if( uxCriticalNesting == 0 )
    {
        portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_INT_BIT;
        portENABLE_INTERRUPTS();

        /* Honor a yield that was requested inside the section. */
        while( ulPortSwitchPending != 0 )
        {
            __asm volatile ( " nop " );
        }
    }
}

/*-----------------------------------------------------------*/

/* SVC: first task launch ONLY, and the place SysTick gets armed.
 * Registers r1/r2 are free scratch here (nothing has been restored yet). */
void SVC_Handler( void ) __attribute__( ( naked ) );
void SVC_Handler( void )
{
    __asm volatile
    (
    "   ldr  r1, =g_portFirstLaunchCount   \n"
    "   ldr  r2, [r1]                      \n"
    "   adds r2, r2, #1                    \n"
    "   str  r2, [r1]                      \n"
    "   ldr  r3, =pxCurrentTCB             \n"
    "   ldr  r1, [r3]                      \n"
    "   ldr  r0, [r1]                      \n"
    "   ldmia r0!, {r4-r11}                \n"
    "   msr  psp, r0                       \n"
    "   isb                                \n"
    "   ldr  r1, =0xE000E010               \n"   /* SysTick CTRL */
    "   movs r2, #7                        \n"   /* CLKSOURCE | TICKINT | ENABLE */
    "   str  r2, [r1]                      \n"   /* arm the tick only now */
    "   orr  r14, r14, #0xd                \n"
    "   bx   r14                           \n"
    "   .ltorg                             \n"
    );
}

/*-----------------------------------------------------------*/

/* SysTick: count the tick, then switch if the tick or a pending yield
 * request says so. If we interrupted another handler (EXC_RETURN bit 3 == 0,
 * e.g. the HAL tick ISR), we must NOT switch: defer to the next tick. */
void SysTick_Handler( void ) __attribute__( ( naked ) );
void SysTick_Handler( void )
{
    __asm volatile
    (
    "   push {r0, lr}                      \n"
    "   bl   xTaskIncrementTick            \n"
    "   mov  r1, r0                        \n"
    "   pop  {r0, lr}                      \n"
    "   ldr  r2, =ulPortSwitchPending      \n"
    "   ldr  r3, [r2]                      \n"
    "   orr  r1, r1, r3                    \n"
    "   cmp  r1, #0                        \n"
    "   beq  systick_no_switch             \n"
    "                                      \n"
    "   tst  r14, #8                       \n"   /* returning to thread mode? */
    "   bne  systick_do_switch             \n"
    "   movs r3, #1                        \n"   /* nested in a handler: defer */
    "   str  r3, [r2]                      \n"
    "   b    systick_no_switch             \n"
    "                                      \n"
    "systick_do_switch:                    \n"
    "   movs r3, #0                        \n"
    "   str  r3, [r2]                      \n"   /* consume the request */
    "   ldr  r1, =g_portTickSwitchCount    \n"
    "   ldr  r2, [r1]                      \n"
    "   adds r2, r2, #1                    \n"
    "   str  r2, [r1]                      \n"
    "   mrs  r0, psp                       \n"
    "   isb                                \n"
    "   ldr  r3, =pxCurrentTCB             \n"
    "   ldr  r2, [r3]                      \n"
    "   stmdb r0!, {r4-r11}                \n"
    "   str  r0, [r2]                      \n"
    "   stmdb sp!, {r3, r14}               \n"
    "   bl   vTaskSwitchContext            \n"
    "   ldmia sp!, {r3, r14}               \n"
    "   ldr  r1, [r3]                      \n"
    "   ldr  r0, [r1]                      \n"
    "   ldmia r0!, {r4-r11}                \n"
    "   msr  psp, r0                       \n"
    "   isb                                \n"
    "                                      \n"
    "systick_no_switch:                    \n"
    "   bx   r14                           \n"
    "   .ltorg                             \n"
    );
}

/*-----------------------------------------------------------*/

static void prvPortStartFirstTask( void ) __attribute__( ( naked ) );
static void prvPortStartFirstTask( void )
{
    __asm volatile
    (
    " ldr r0, =0xE000ED08   \n"   /* VTOR */
    " ldr r0, [r0]          \n"
    " ldr r0, [r0]          \n"   /* initial MSP from vector table */
    " msr msp, r0           \n"
    " cpsie i               \n"
    " dsb                   \n"
    " isb                   \n"
    " svc 0                 \n"
    " nop                   \n"
    " .ltorg                \n"
    );
}

/*-----------------------------------------------------------*/

/* Programs the SysTick reload but leaves it DISABLED: SVC_Handler arms it
 * at the moment the first task launches (see the header comment). */
void vPortSetupTimerInterrupt( void )
{
    portNVIC_SYSTICK_CTRL_REG = 0UL;
    portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;
    portNVIC_SYSTICK_LOAD_REG = ( configCPU_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
}

/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
    vPortSetupTimerInterrupt();

    uxCriticalNesting = 0;
    ulPortSwitchPending = 0;

    prvPortStartFirstTask();

    prvTaskExitError();
    return 0;
}

/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    for( ;; )
    {
    }
}