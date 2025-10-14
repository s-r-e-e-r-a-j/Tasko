## Tasko

**Tasko** is a **header-only, lightweight**, **and powerful task scheduler library for ESP32**. It allows you to manage **repeating tasks**, **one-time tasks**, **dual-core scheduling**, and **task hooks** with a **clean, human-readable API.**

## Features

- Header-only, no `.c` files required.

- Supports **repeating and one-time tasks.**
  
- **Dual-core scheduling** for ESP32 (pin tasks to core 0 or 1).

- **Task priorities** for FreeRTOS scheduling.

- **Start/Stop hooks** for task lifecycle events.

- **Pause/Resume** individual tasks.

- **Remove tasks safely** or **clear all tasks**.

- Built-in **debug logging** for easy testing.

- Fully compatible with **Arduino IDE**, **PlatformIO**, and **ESP-IDF** based **setups**.

## Installation

1. Download `Tasko.h` and place it in your project folder.

2. Include the header in your sketch:
```cpp
#include "Tasko.h"
```

## API Examples
1. **Enable Debug**
```c
void TaskoEnableDebug(bool enable);
```

**TaskoEnableDebug** – Enables or disables debug log output to the Serial monitor.

When enabled, Tasko prints internal messages for added visibility.

**Example:**
```c
TaskoEnableDebug(true);   // Enable debug messages
TaskoEnableDebug(false);  // Disable debug messages
```

2. **Add Task**
```c
int TaskoAdd(TaskoCallback func, void* arg, uint32_t intervalMs, bool repeat,
             uint8_t priority = 1, uint8_t core = 1,
             TaskoHook startHook = NULL, TaskoHook stopHook = NULL);
```


**TaskoAdd** – Creates a new scheduled task or repeating timer managed by Tasko.

Returns the **task ID** (0–15) on success, or **-1** if the task limit is reached.

- **func** → Function to execute (callback).

- **arg** → Argument passed to the callback (can be `NULL`).

- **intervalMs** → Delay or repeat interval in milliseconds.

- **repeat** → `true` for repeating tasks, `false` for one-time execution.

- **priority** → Task priority (default = 1). Higher values = higher priority.

- **core** → CPU core to run the task on (default = 1).

- **startHook** → Optional function called before task starts.

- **stopHook** → Optional function called after task finishes.
