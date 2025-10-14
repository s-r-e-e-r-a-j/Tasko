#ifndef TASKO_H
#define TASKO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

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
} TaskoTask;

static TaskoTask taskList[TASKO_MAX_TASKS];
static uint8_t taskCount = 0;
static bool taskoDebug = false;

// Enable debug logging
static inline void TaskoEnableDebug(bool enable) { taskoDebug = enable; }

// Internal logger
static inline void TaskoLog(const char* msg) {
    if (taskoDebug) Serial.println(msg);
}

static inline int TaskoAdd(TaskoCallback func, void* arg, uint32_t intervalMs, bool repeat,
                           uint8_t priority = 1, uint8_t core = 1,
                           TaskoHook startHook = NULL, TaskoHook stopHook = NULL) {
    if (taskCount >= TASKO_MAX_TASKS) return -1;

    TaskoTask* t = &taskList[taskCount];
    t->callback = func;
    t->arg = arg;
    t->intervalMs = intervalMs;
    t->repeating = repeat;
    t->priority = priority;
    t->core = core;
    t->active = true;
    t->onStart = startHook;
    t->onStop = stopHook;

    if (repeat) {
        t->timer = xTimerCreate(
            "TaskoTimer",
            pdMS_TO_TICKS(intervalMs),
            pdTRUE,
            (void*)(intptr_t)taskCount,
            [](TimerHandle_t xTimer) {
                int idx = (int)(intptr_t)pvTimerGetTimerID(xTimer);
                if (taskList[idx].active) {
                    if (taskList[idx].onStart) taskList[idx].onStart(idx);
                    if (taskList[idx].callback) taskList[idx].callback(taskList[idx].arg);
                    if (taskList[idx].onStop) taskList[idx].onStop(idx);
                }
            }
        );
        xTimerStart(t->timer, 0);
        t->handle = NULL;
        if (taskoDebug) TaskoLog("Added repeating task");
    } else {
        t->handle = NULL;
        xTaskCreatePinnedToCore(
            [](void* param) {
                int idx = (int)(intptr_t)param;
                if (taskList[idx].onStart) taskList[idx].onStart(idx);
                if (taskList[idx].callback) taskList[idx].callback(taskList[idx].arg);
                if (taskList[idx].onStop) taskList[idx].onStop(idx);
                taskList[idx].active = false;
                vTaskDelete(NULL);
            },
            "TaskoOnce",
            2048,
            (void*)(intptr_t)taskCount,
            priority,
            NULL,
            core
        );
        if (taskoDebug) TaskoLog("Added one-time task");
    }

    return taskCount++;
}

static inline void TaskoRemove(int id) {
    if (id < 0 || id >= taskCount) return;
    TaskoTask* t = &taskList[id];
    t->active = false;
    if (t->timer) xTimerDelete(t->timer, 0);
    if (t->handle) vTaskDelete(t->handle);
    if (taskoDebug) TaskoLog("Removed task");
}

static inline void TaskoClearAll() {
    for (uint8_t i = 0; i < taskCount; i++) TaskoRemove(i);
    taskCount = 0;
    if (taskoDebug) TaskoLog("Cleared all tasks");
}

static inline void TaskoPause(int id) {
    if (id < 0 || id >= taskCount) return;
    TaskoTask* t = &taskList[id];
    t->active = false;
    if (t->timer) xTimerStop(t->timer, 0);
    if (taskoDebug) TaskoLog("Paused task");
}

static inline void TaskoResume(int id) {
    if (id < 0 || id >= taskCount) return;
    TaskoTask* t = &taskList[id];
    t->active = true;
    if (t->timer) xTimerStart(t->timer, 0);
    if (taskoDebug) TaskoLog("Resumed task");
}

#ifdef __cplusplus
}
#endif

#endif // TASKO_H
