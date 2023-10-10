
#include "Utils/NoCopy.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"


class FreeRTOSTask : public NoCopy {
public:
    FreeRTOSTask(uint32_t stackSize, uint8_t priority, uint8_t coreNum);
    virtual ~FreeRTOSTask(void) = default;
    
public:
    esp_err_t start(void);
    esp_err_t resume(void);
    esp_err_t suspend(void);

protected:
    virtual void taskFunction(void) = 0;
    static void sTaskFunction( void * pvParameters ) { ((FreeRTOSTask*)pvParameters)->taskFunction(); }

protected:
    virtual const char* task_name(void) = 0;
    virtual esp_err_t on_start() { return ESP_OK; }
    virtual esp_err_t on_resume(void) { return ESP_OK; }
    virtual esp_err_t on_suspend(void) { return ESP_OK; }

private:
    TaskHandle_t task_handle;
    uint32_t stack_size;
    uint8_t priority;
    uint8_t core_num;
};
