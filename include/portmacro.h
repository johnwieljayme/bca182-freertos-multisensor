#ifndef PORTMACRO_H
#define PORTMACRO_H

/* Marker checked by src/port.c: proves THIS header (not the stock one) was used. */
#define PORT_WOKWI_SAFE_MACRO_HEADER 1

/* Force-included into every translation unit, including the assembly startup file. */
#ifndef __ASSEMBLER__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define portCHAR          char
#define portFLOAT         float
#define portDOUBLE        double
#define portLONG          long
#define portSHORT         short
#define portSTACK_TYPE    uint32_t
#define portBASE_TYPE     long

typedef portSTACK_TYPE   StackType_t;
typedef long             BaseType_t;
typedef unsigned long    UBaseType_t;

#if ( configUSE_16_BIT_TICKS == 1 )
    typedef uint16_t TickType_t;
    #define portMAX_DELAY ( TickType_t ) 0xffff
#else
    typedef uint32_t TickType_t;
    #define portMAX_DELAY ( TickType_t ) 0xffffffffUL
    #define portTICK_TYPE_IS_ATOMIC 1
#endif

#define portSTACK_GROWTH            ( -1 )
#define portTICK_PERIOD_MS          ( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT          8

/* Critical sections: PRIMASK + SysTick gating (see port.c). */
extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );

#define portDISABLE_INTERRUPTS()   __asm volatile ( " cpsid i " ::: "memory" )
#define portENABLE_INTERRUPTS()    __asm volatile ( " cpsie i " ::: "memory" )

#define portENTER_CRITICAL()       vPortEnterCritical()
#define portEXIT_CRITICAL()        vPortExitCritical()

static inline uint32_t vPortSetInterruptMaskFromISR( void )
{
    uint32_t ulOriginalMask;
    __asm volatile
    (
        " mrs %0, PRIMASK   \n"
        " cpsid i           \n"
        : "=r" ( ulOriginalMask ) :: "memory"
    );
    return ulOriginalMask;
}

static inline void vPortClearInterruptMaskFromISR( uint32_t ulMask )
{
    __asm volatile ( " msr PRIMASK, %0 " :: "r" ( ulMask ) : "memory" );
}

#define portSET_INTERRUPT_MASK_FROM_ISR()          vPortSetInterruptMaskFromISR()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR( x )      vPortClearInterruptMaskFromISR( x )

/*-----------------------------------------------------------
 * Yielding.
 *  - Task context, outside a critical section: SVC (synchronous switch).
 *  - Inside a critical section: deferred until the outermost exit.
 *  - From an ISR: sets a flag; the next SysTick performs the switch.
 *----------------------------------------------------------*/
extern void vPortYield( void );
extern volatile uint32_t ulPortSwitchPending;

#define portYIELD()                vPortYield()
#define portYIELD_WITHIN_API       portYIELD

#define portEND_SWITCHING_ISR( xSwitchRequired )   do { if( ( xSwitchRequired ) != pdFALSE ) { ulPortSwitchPending = 1; } } while( 0 )
#define portYIELD_FROM_ISR( x )                    portEND_SWITCHING_ISR( x )

#define portNOP()                  __asm volatile ( " nop " )
#define portINLINE                 __inline

#ifndef portFORCE_INLINE
    #define portFORCE_INLINE inline __attribute__(( always_inline))
#endif

#define portMEMORY_BARRIER()       __asm volatile( "" ::: "memory" )

#define portTASK_FUNCTION_PROTO( vFunction, pvParameters )    void vFunction( void *pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters )          void vFunction( void *pvParameters )

#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLER__ */

#endif /* PORTMACRO_H */