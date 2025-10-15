## Tasko

**Tasko** is a **header-only, lightweight**, **and powerful task scheduler library for ESP32**. It allows you to manage **repeating tasks**, **one-time tasks**, **dual-core scheduling**, and **task hooks** with a **clean, human-readable API.**

## Features

- Header-only library, no additional `.c` files required.

- Supports both repeating and one-time tasks.

- Dual-core scheduling on ESP32, allowing tasks to be pinned to core 0 or core 1.

- Task priorities compatible with FreeRTOS scheduling.

- Start and stop hooks for running custom code before and after each task.

- Pause and resume individual tasks at any time.

- Remove specific tasks safely or clear all tasks at once.

- Built-in debug logging to make testing and troubleshooting easier.

- Fully compatible with Arduino IDE, PlatformIO, and ESP-IDF-based projects.

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
             uint8_t priority, uint8_t core,
             TaskoHook startHook, TaskoHook stopHook, size_t stackSize);
```


The function `TaskoAdd` **creates and starts a task** in Tasko. It can either be:

- **A one-time task** that runs once, or

- **A repeating task** that runs periodically like a timer.

Returns the task ID of the newly created task on success (from `0` to `TASKO_MAX_TASKS - 1`), or `-1` if the maximum number of tasks has been reached.

- **func** → The task callback function that will be executed by Tasko when the task runs

- **arg** → A pointer to a single piece of data that will be passed to the task callback function when it runs. Use `NULL` if no data needs to be passed. Only one argument is directly supported.

- **intervalMs** → The time in **milliseconds** before the task runs again. For one-time tasks, it represents the delay before execution.

- **repeat** → Set to `true` if the task should repeat at regular intervals. Set to `false` if the task should run only once.

- **priority** → Defines how important the task is compared to others. Tasks with higher priority values are given more CPU time.

- **core** → The CPU core number on which the task will run. On dual-core boards like the ESP32, you can choose core 0 or core 1. On single-core boards, FreeRTOS runs the task on the only available core.

- **startHook** → Optional function called automatically before the task callback function runs. Tasko passes the task ID as an argument. **You must define the function with `int id` as a parameter**.pass `NULL` if not needed 

- **stopHook** → Optional function called automatically after the task callback function finishes. Tasko passes the task ID as an argument. **You must define the function with `int id` as a parameter**.pass `NULL` if not needed

- **stackSize** → The amount of memory (in bytes) the task can use for its variables and function calls. Make it big enough so the task doesn’t crash, but not too big to waste memory. Example: `4096`.
- 

**Example**

```c

#include <Arduino.h>
#include "Tasko.h"

// Define the LED pin for boards that do not have LED_BUILTIN defined
#ifndef LED_BUILTIN
#define LED_BUILTIN 2  // Change this to your board's built-in LED pin if necessary
#endif

// This function blinks the built-in LED
void blinkTask(void* arg) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

// This function runs only once and prints a message to the Serial monitor
void runOnce(void* arg) {
    Serial.println("One-time task executed");
}

// This function is called before a task starts
void onStart(int id) {
    Serial.printf("Task %d started\n", id);
}

// This function is called after a task finishes
void onStop(int id) {
    Serial.printf("Task %d stopped\n", id);
}

void setup() {
    // Start the Serial monitor for output
    Serial.begin(115200);

    delay(1000);
    // Set the built-in LED pin as output
    pinMode(LED_BUILTIN, OUTPUT);

    // Enable debug messages from Tasko
    TaskoEnableDebug(true);

    // Add a repeating task that blinks the LED every 1000 milliseconds
    // The startHook and stopHook functions are called before and after each execution
    int blinkId = TaskoAdd(blinkTask, NULL, 1000, true, 1, 1, onStart, onStop, 4096);

    // Add a one-time task that prints a message
    // The startHook and stopHook functions are also called for this task
    int onceId = TaskoAdd(runOnce, NULL, 10, false, 1, 1, onStart, onStop, 4096);
}

void loop() {
    // Nothing needs to be done in the loop because Tasko handles tasks automatically
}


```

3. **Remove a Task**
```c
void TaskoRemove(int id);
```

**Parameters:**

**id** → Task ID returned by `TaskoAdd`.

**Example:**
```c
TaskoRemove(blinkTaskId); // Removes the repeating blink task
```

4. **Clear All Tasks**
```c
void TaskoClearAll();
```

**Description**: Removes all tasks currently managed by Tasko.

**Example:**
```c
TaskoClearAll();
```

5. **Pause a Task**
```c
void TaskoPause(int id);
```

**Parameters**:

**id** → Task ID returned by `TaskoAdd`.

**Example:**
```c
TaskoPause(blinkTaskId); // Pauses the blink task
```

6. **Resume a Task**
```c
void TaskoResume(int id);
```
**Parameters:**

**id** → Task ID returned by `TaskoAdd`.

**Example**:
```c
TaskoResume(blinkTaskId); // Resumes the blink task
```

## License
This project is licensed under the MIT License

