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
    bool pendingRemove; // for deferred removal
    bool used;
} TaskoTask;

static TaskoTask taskList[TASKO_MAX_TASKS];
static uint8_t taskCount = 0;
static bool taskoDebug = false;

// Enable debug logging
static inline void TaskoEnableDebug(bool enable) { taskoDebug = enable; }
static inline void TaskoLog(const char* msg) { if (taskoDebug) Serial.println(msg); }

// One-time task wrapper
static void TaskoOneTimeWrapper(void* param) {
    int idx = (int)(intptr_t)param;
    if (taskList[idx].onStart) taskList[idx].onStart(idx);
    if (taskList[idx].callback) taskList[idx].callback(taskList[idx].arg);
    if (taskList[idx].onStop) taskList[idx].onStop(idx);
    taskList[idx].active = false;
    vTaskDelete(NULL); // never return
}

// Timer callback wrapper
static void TaskoTimerCallback(TimerHandle_t xTimer) {
    int idx = (int)(intptr_t)pvTimerGetTimerID(xTimer);
    if (!taskList[idx].active) return;

    if (taskList[idx].pendingRemove) {
        xTimerDelete(taskList[idx].timer, 0);
        taskList[idx].timer = NULL;
        return;
    }

    if (taskList[idx].onStart) taskList[idx].onStart(idx);
    if (taskList[idx].callback) taskList[idx].callback(taskList[idx].arg);
    if (taskList[idx].onStop) taskList[idx].onStop(idx);
}

// Add task
static int TaskoAdd(TaskoCallback func, void* arg, uint32_t intervalMs, bool repeat,
                    uint8_t priority, uint8_t core,
                    TaskoHook startHook, TaskoHook stopHook) {
    if (taskCount >= TASKO_MAX_TASKS) return -1;

    int id = -1;
    for (int i = 0; i < TASKO_MAX_TASKS; i++) {
        if (!taskList[i].used) {
            id = i;
            break;
        }
    }
    if (id == -1) return -1; // no free slot
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
        xTimerStart(t->timer, 0);
        t->handle = NULL;
        if (taskoDebug) TaskoLog("Added repeating task");
    } else {
        TaskHandle_t tmpHandle = NULL;
        xTaskCreatePinnedToCore(TaskoOneTimeWrapper, "TaskoOnce",
                                4096, (void*)(intptr_t)id,
                                priority, &tmpHandle, core);
        t->handle = tmpHandle;
        if (taskoDebug) TaskoLog("Added one-time task");
    }

    return id;
}

// Remove task safely (If it’s running, wait and remove it later)
static void TaskoRemove(int id) {
    if (id < 0 || id >= TASKO_MAX_TASKS || !taskList[id].used) return;
    TaskoTask* t = &taskList[id];
    t->active = false;

    // If the task is still running, wait before removing it.
    if (t->handle == xTaskGetCurrentTaskHandle()) {
        t->pendingRemove = true;
        if (taskoDebug) TaskoLog("Wait to remove running task");
        return;
    }

    if (t->timer) {
        xTimerDelete(t->timer, 0);
        t->timer = NULL;
    }
    if (t->handle) {
        vTaskDelete(t->handle);
        t->handle = NULL;
    }
    t->used = false;
    if (taskoDebug) TaskoLog("Removed task immediately");
}

static void TaskoClearAll() {
    for (int i = 0; i < TASKO_MAX_TASKS; i++) {
    if (taskList[i].used) TaskoRemove(i);
    }
    taskCount = 0;
    if (taskoDebug) TaskoLog("Cleared all tasks");
}

static void TaskoPause(int id) {
    if (id < 0 || id >= taskCount) return;
    TaskoTask* t = &taskList[id];
    t->active = false;
    if (t->timer) xTimerStop(t->timer, 0);
    if (taskoDebug) TaskoLog("Paused task");
}

static void TaskoResume(int id) {
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
