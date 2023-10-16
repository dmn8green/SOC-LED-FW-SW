#include "FreeRTOSTask.h"

#include <memory.h>

FreeRTOSTask::FreeRTOSTask(uint32_t stackSize=5000, uint8_t priority=11, uint8_t coreNum=0) :
    task_handle(nullptr),
    stack_size(stackSize),
    priority(priority),
    core_num(coreNum)
{
}

esp_err_t FreeRTOSTask::start(void) {
    if (this->task_handle != nullptr) {
        return ESP_FAIL;
    }

    xTaskCreatePinnedToCore(
        FreeRTOSTask::sTaskFunction, this->task_name(), this->stack_size, this,
        this->priority, &this->task_handle, this->core_num);
    return ESP_OK;
}

esp_err_t FreeRTOSTask::resume(void)
{
    vTaskResume(this->task_handle);
    on_resume();
    return ESP_OK;
}

esp_err_t FreeRTOSTask::suspend(void)
{
    vTaskSuspend(this->task_handle);
    on_suspend();
    return ESP_OK;
}