#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "stm32f1xx.h"


/* =========================================================
 * BCA182 FreeRTOS Multisensor
 * STM32F103C8T6 / Cortex-M3
 * Custom Wokwi-safe port (see lib/freertos_port_patch/port.c
 * and include/portmacro.h): no PendSV, PRIMASK-based critical
 * sections, synchronous SVC/SysTick context switches.
 * ========================================================= */


/* =========================================================
 * Scheduler
 * ========================================================= */

#define configUSE_PREEMPTION                    1

#define configUSE_IDLE_HOOK                     1

#define configUSE_TICK_HOOK                     0

#define configCPU_CLOCK_HZ                      (8000000UL)

#define configTICK_RATE_HZ                      ((TickType_t)1000)

#define configMAX_PRIORITIES                    5

#define configMINIMAL_STACK_SIZE                ((unsigned short)128)

#define configTOTAL_HEAP_SIZE                   ((size_t)(8 * 1024))

#define configMAX_TASK_NAME_LEN                 16

#define configUSE_16_BIT_TICKS                  0

#define configIDLE_SHOULD_YIELD                 1


/* =========================================================
 * Synchronization
 * ========================================================= */

#define configUSE_MUTEXES                       1

#define configUSE_COUNTING_SEMAPHORES           1

#define configUSE_RECURSIVE_MUTEXES             1

#define configUSE_TASK_NOTIFICATIONS            1


/* =========================================================
 * Memory
 * ========================================================= */

#define configSUPPORT_STATIC_ALLOCATION         0

#define configSUPPORT_DYNAMIC_ALLOCATION        1

#define configUSE_MALLOC_FAILED_HOOK            0

#define configCHECK_FOR_STACK_OVERFLOW          2


/* =========================================================
 * FreeRTOS Assertions
 *
 * Safe to re-enable now: the only thing that made configASSERT
 * unusable before was the STOCK port.c's priority-bit self-test,
 * which our custom port.c does not include at all. Every other
 * configASSERT call throughout the kernel (tasks.c, queue.c, etc.)
 * works normally and is genuinely useful.
 * ========================================================= */

#define configASSERT_DEFINED                    1
#ifdef __cplusplus
extern "C" void vAssertCalled(const char* file, int line);
#else
extern void vAssertCalled(const char* file, int line);
#endif
#define configASSERT( x )   if( ( x ) == 0 ) vAssertCalled( __FILE__, __LINE__ )


/* =========================================================
 * FreeRTOS API
 * ========================================================= */

#define INCLUDE_vTaskPrioritySet                1

#define INCLUDE_uxTaskPriorityGet               1

#define INCLUDE_vTaskDelete                     1

#define INCLUDE_vTaskCleanUpResources            1

#define INCLUDE_vTaskSuspend                    1

#define INCLUDE_vTaskDelayUntil                 1

#define INCLUDE_vTaskDelay                      1

#define INCLUDE_xTaskGetSchedulerState          1


/* =========================================================
 * FreeRTOS -> STM32 CMSIS Handler Mapping
 *
 * NOTE: PendSV is deliberately NOT mapped here -- this port
 * never uses it (see lib/freertos_port_patch/port.c). SVC and
 * SysTick are implemented directly under their real CMSIS
 * names in port.c (not via macro renaming), so no #define is
 * needed for them either. This section is intentionally empty.
 * ========================================================= */


#endif /* FREERTOS_CONFIG_H */