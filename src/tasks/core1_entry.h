#ifndef CORE1_ENTRY_H
#define CORE1_ENTRY_H

/* Core 1 boot function. Called via multicore_launch_core1() from Core 0.
 * Initializes Core-1-owned peripherals (LCD), creates FreeRTOS tasks,
 * and starts the scheduler. Never returns. */
void core1_entry(void);

#endif
