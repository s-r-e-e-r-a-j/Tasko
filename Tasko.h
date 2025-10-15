#ifndef TASKO_H
#define TASKO_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ARDUINO
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#else
#include <stdio.h>
#include <stdint.h>
#include <stddef.h> 
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#endif

// Maximum tasks allowed
#ifndef TASKO_MAX_TASKS
#define TASKO_MAX_TASKS 16
#endif

typedef void (*TaskoCallback)(void*);
typedef void (*TaskoHook)(int id);

typedef struct {
    TaskHandle_t handle;
    TimerHandle_t timer;
    TaskoCallback callback;
    void* arg;
    TaskoHook onStart;
    TaskoHook onStop;
    uint32_t intervalMs;
    uint8_t priority;
    uint8_t core;
    bool repeating;
    bool active;
    bool pendingRemove;
    bool used;
} TaskoTask;

static TaskoTask taskList[TASKO_MAX_TASKS];
static uint8_t taskCount = 0;
static bool taskoDebug = false;

// Enable debug logging
static inline void TaskoEnableDebug(bool enable) { taskoDebug = enable; }
static inline void TaskoLog(const char* msg, int id) {
    if (taskoDebug) {
        if (id >= 0) Serial.printf("[Tasko] %s (id=%d)\n", msg, id);
        else Serial.printf("[Tasko] %s\n", msg);
    }
}

// One-time task wrapper
static void TaskoOneTimeWrapper(void* param) {
    int idx = (int)(intptr_t)param;
    if (idx < 0 || idx >= TASKO_MAX_TASKS) return;

    TaskoTask* t = &taskList[idx];
    if (!t->used) return;

    if (t->onStart) t->onStart(idx);
    if (t->callback) t->callback(t->arg);
    if (t->onStop) t->onStop(idx);

    t->active = false;
    t->used = false;
    taskCount--;

    vTaskDelete(NULL); // safe deletion
}

// Timer callback wrapper
static void TaskoTimerCallback(TimerHandle_t xTimer) {
    int idx = (int)(intptr_t)pvTimerGetTimerID(xTimer);
    if (idx < 0 || idx >= TASKO_MAX_TASKS) return;

    TaskoTask* t = &taskList[idx];
    if (!t->used || !t->active) return;

    if (t->pendingRemove) {
        if (t->timer != NULL) {
            xTimerDelete(t->timer, 0);
            t->timer = NULL;
        }
        t->used = false;
        t->pendingRemove = false;
        taskCount--;
        TaskoLog("Removed pending repeating task", idx);
        return;
    }

    if (t->onStart) t->onStart(idx);
    if (t->callback) t->callback(t->arg);
    if (t->onStop) t->onStop(idx);
}

// Add task
static int TaskoAdd(TaskoCallback func, void* arg, uint32_t intervalMs, bool repeat,
                    uint8_t priority, uint8_t core,
                    TaskoHook startHook, TaskoHook stopHook,
                    size_t stackSize) {

    if (taskCount >= TASKO_MAX_TASKS) return -1;

    int id = -1;
    for (int i = 0; i < TASKO_MAX_TASKS; i++) {
        if (!taskList[i].used) { id = i; break; }
    }
    if (id == -1) return -1;

    TaskoTask* t = &taskList[id];
    t->callback = func;
    t->arg = arg;
    t->intervalMs = intervalMs;
    t->repeating = repeat;
    t->priority = priority;
    t->core = core;
    t->active = true;
    t->onStart = startHook;
    t->onStop = stopHook;
    t->pendingRemove = false;
    t->used = true;

    if (repeat) {
        t->timer = xTimerCreate("TaskoTimer", pdMS_TO_TICKS(intervalMs), pdTRUE,
                                (void*)(intptr_t)id, TaskoTimerCallback);
        if (t->timer) xTimerStart(t->timer, 0);
        t->handle = NULL;
        TaskoLog("Added repeating task", id);
    } else {
        TaskHandle_t tmpHandle = NULL;
        xTaskCreatePinnedToCore(TaskoOneTimeWrapper, "TaskoOnce",
                                stackSize, (void*)(intptr_t)id,
                                priority, &tmpHandle, core);
        t->handle = tmpHandle;
        TaskoLog("Added one-time task", id);
    }

    taskCount++;
    return id;
}

// Remove task safely
static void TaskoRemove(int id) {
    if (id < 0 || id >= TASKO_MAX_TASKS) return;

    TaskoTask* t = &taskList[id];
    if (!t->used) return;

    t->active = false;

    if (t->handle == xTaskGetCurrentTaskHandle()) {
        t->pendingRemove = true;
        TaskoLog("Pending removal of running task", id);
        return;
    }

    if (t->timer != NULL) {
        xTimerDelete(t->timer, 0);
        t->timer = NULL;
    }

    if (t->handle != NULL) {
        vTaskDelete(t->handle);
        t->handle = NULL;
    }

    t->used = false;
    taskCount--;
    TaskoLog("Removed task immediately", id);
}

// Clear all tasks
static void TaskoClearAll() {
    for (int i = 0; i < TASKO_MAX_TASKS; i++) {
        if (taskList[i].used) TaskoRemove(i);
    }
}

// Pause / Resume
static void TaskoPause(int id) {
    if (id < 0 || id >= TASKO_MAX_TASKS) return;
    TaskoTask* t = &taskList[id];
    if (!t->used) return;
    t->active = false;
    if (t->timer) xTimerStop(t->timer, 0);
    TaskoLog("Paused task", id);
}

static void TaskoResume(int id) {
    if (id < 0 || id >= TASKO_MAX_TASKS) return;
    TaskoTask* t = &taskList[id];
    if (!t->used) return;
    t->active = true;
    if (t->timer) xTimerStart(t->timer, 0);
    TaskoLog("Resumed task", id);
}

#ifdef __cplusplus
}
#endif

#endif // TASKO_H
