#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* FreeRTOS configuration for the GLCD project.
 *
 * RTOS runs ONLY on Core 1 (single-core scheduler). Core 0 stays bare-metal
 * for the real-time control loop and must never call FreeRTOS APIs. */

/* ----- Scheduler ----- */
#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  1
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      125000000   /* Pico 1 default */
#define configTICK_RATE_HZ                      1000        /* 1 ms tick */
#define configMAX_PRIORITIES                    8
#define configMINIMAL_STACK_SIZE                256
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1

/* ----- Synchronization primitives ----- */
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   3
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_QUEUE_SETS                    0
#define configENABLE_BACKWARD_COMPATIBILITY     0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 0

/* ----- Memory management ----- */
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configTOTAL_HEAP_SIZE                   (32 * 1024)
#define configAPPLICATION_ALLOCATED_HEAP        0

/* ----- Hooks and diagnostics ----- */
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_TRACE_FACILITY                0
#define configUSE_STATS_FORMATTING_FUNCTIONS    0
#define configRECORD_STACK_HIGH_ADDRESS         1

/* ----- Co-routines (unused) ----- */
#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         1

/* ----- Software timers ----- */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               3
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            512

/* ----- Optional API functions ----- */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_eTaskGetState                   1

/* ----- Single-core mode — RTOS runs on Core 1 only ----- */
#define configNUMBER_OF_CORES                   1
#define configUSE_CORE_AFFINITY                 0

/* ----- Cortex-M0+ specific ----- */
#define configKERNEL_INTERRUPT_PRIORITY         (1 << 6)   /* lowest priority */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    0          /* not used on M0+ */

/* ----- Cortex-M33 specific (RP2350 / Pico 2) -----
 * These macros are required by the RP2350_ARM_NTZ port and ignored by the
 * RP2040 (M0+) port, so it is safe to define them unconditionally. */
#define configENABLE_FPU                        1   /* use the single-precision FPU */
#define configENABLE_MPU                        0   /* MPU not used */
#define configENABLE_TRUSTZONE                  0   /* non-secure only (NTZ port) */
#define configRUN_FREERTOS_SECURE_ONLY          1   /* run entirely in non-secure world */

/* ----- Asserts -----
 * Use portDISABLE_INTERRUPTS (not task-level) because configASSERT is
 * expanded inside portmacro.h before task.h has been included. */
#define configASSERT(x)                                                  \
    do {                                                                 \
        if ((x) == 0) {                                                  \
            portDISABLE_INTERRUPTS();                                    \
            for (;;) { }                                                 \
        }                                                                \
    } while (0)

/* ----- Raspberry Pi Pico SDK specific ----- */
#define configSUPPORT_PICO_SYNC_INTEROP         1
#define configSUPPORT_PICO_TIME_INTEROP         1

#endif /* FREERTOS_CONFIG_H */
