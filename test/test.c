/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <pico/stdlib.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "unity_config.h"
#include <unity.h>

SemaphoreHandle_t semaphore;

TaskHandle_t firstThread;
TaskHandle_t secondThread;

TaskFunction_t primary;
TaskFunction_t secondary;

configRUN_TIME_COUNTER_TYPE first_stats, second_stats, elapsed_stats;
TickType_t elapsed_ticks;

void setUp(void) {
}

void tearDown(void) {
}

void priorityInversionBinary() {
    xSemaphoreTake(semaphore, portMAX_DELAY);

    semaphore = xSemaphoreCreateBinary();

    xTaskCreate(primary, "First", configMINIMAL_STACK_SIZE, "FirstThread", tskIDLE_PRIORITY+(1), &firstThread);
    vTaskDelay(100);

    xTaskCreate(secondary, "Second", configMINIMAL_STACK_SIZE, "SecondThread", tskIDLE_PRIORITY+(2), &secondThread);

    busy_wait_us(1000);

    xSemaphoreGive(semaphore);

    vSemaphoreDelete(semaphore);
}

void priorityInversionMutex() {
    xSemaphoreTake(semaphore, portMAX_DELAY);

    semaphore = xSemaphoreCreateMutex();

    xTaskCreate(primary, "First", configMINIMAL_STACK_SIZE, "FirstThread", tskIDLE_PRIORITY+(1), &firstThread);
    vTaskDelay(100);

    xTaskCreate(secondary, "Second", configMINIMAL_STACK_SIZE, "SecondThread", tskIDLE_PRIORITY+(2), &secondThread);

    busy_wait_us(1000);

    xSemaphoreGive(semaphore);

    vSemaphoreDelete(semaphore);
}

void busy_busy(void *args) {
    char *name = (char *)args;
    printf("busy_busy %s\n", name);
    for(int i = 0; ; i++) {
    }
}

void busy_yield(void *args) {
    char *name = (char *)args;
    printf("busy_yield %s\n", name);
    for(int i = 0; ; i++) {
        taskYIELD();
    }
}

void testSamePriorityBothBusyBusy() {
     run_analyzer(busy_busy, tskIDLE_PRIORITY+(3), 0, &first_stats,
                 busy_busy, tskIDLE_PRIORITY+(3), 0, &second_stats,
                 &elapsed_stats, &elapsed_ticks);
    TEST_ASSERT(2000000 < first_stats);
    TEST_ASSERT(2000000 < second_stats);
}

void testSamePriorityBothBusyYield() {
    run_analyzer(busy_yield, tskIDLE_PRIORITY+(3), 0, &first_stats,
                 busy_yield, tskIDLE_PRIORITY+(3), 0, &second_stats,
                 &elapsed_stats, &elapsed_ticks);
    TEST_ASSERT(2000000 < first_stats);
    TEST_ASSERT(2000000 < second_stats);

}

void testSamePriorityBusyBusy_BusyYield() {
     run_analyzer(busy_busy, tskIDLE_PRIORITY+(3), 0, &first_stats,
                 busy_yield, tskIDLE_PRIORITY+(3), 0, &second_stats,
                 &elapsed_stats, &elapsed_ticks);
    TEST_ASSERT(2000000 < first_stats);
    TEST_ASSERT(2000000 < second_stats);

}

void testDifferentPriorityBothBusyBusy() {
     run_analyzer(busy_busy, tskIDLE_PRIORITY+(3), 1, &first_stats,
                 busy_busy, tskIDLE_PRIORITY+(3), 0, &second_stats,
                 &elapsed_stats, &elapsed_ticks);
    TEST_ASSERT(2000000 < first_stats);
    TEST_ASSERT(2000000 < second_stats);

}

void testDifferentPriorityBothBusyYield() {
     run_analyzer(busy_yield, tskIDLE_PRIORITY+(3), 1, &first_stats,
                 busy_yield, tskIDLE_PRIORITY+(3), 0, &second_stats,
                 &elapsed_stats, &elapsed_ticks);
    TEST_ASSERT(2000000 < first_stats);
    TEST_ASSERT(2000000 < second_stats);

}

void measure_runtime(void (func0)(void), UBaseType_t priority0,
                     void (func1)(void), UBaseType_t priority1,
                     configRUN_TIME_COUNTER_TYPE runtime0, configRUN_TIME_COUNTER_TYPEruntime1)
{
    TaskHandle_t handle0, handle1;
    xTaskCreate(simple_task, "Thread0", configMINIMAL_STACK_SIZE, func0, priority0, &handle0);
    xTaskCreate(delayed_task, "Thread1", configMINIMAL_STACK_SIZE, func1, priority1, &handle1);

    vTaskDelay(2000);

    TaskStatus_t status;
    vTaskGetInfo(handle0, &status, pdFALSE, eInvalid);
    *runtime0 = status.ulRunTimeCounter;
    vTaskGetInfo(handle1, &status, pdFALSE, eInvalid);
    *runtime1 = status.ulRunTimeCounter;

    vTaskDelete(handle0);
    vTaskDelete(handle1);
}

void tests() {
    UNITY_BEGIN();
    RUN_TEST(testSamePriorityBothBusyBusy);
    RUN_TEST(testSamePriorityBothBusyYield);
    RUN_TEST(testSamePriorityBusyBusy_BusyYield);
    RUN_TEST(testDifferentPriorityBothBusyBusy);
    RUN_TEST(testDifferentPriorityBothBusyYield);
    UNITY_END();
    vTaskDelay(1000);
}

int main( void )
{
    stdio_init_all();
    TaskHandle_t main_handle;
    xTaskCreate(tests, "Tests", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+10, &main_handle);
    vTaskStartScheduler();
    return 0;
}
