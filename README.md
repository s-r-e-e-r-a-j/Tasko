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
