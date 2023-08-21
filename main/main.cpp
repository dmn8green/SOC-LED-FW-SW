#include "ReplConsole/repl_console.h"
#include "pin_def.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "esp_vfs_fat.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "MN8 LB: ";

static void initialize_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

extern "C" void app_main(void)
{
    initialize_nvs();
    
    repl_configure(REPL_UART_TX_PIN, REPL_UART_RX_PIN, REPL_UART_CHANNEL, REPL_UART_BAUD_RATE);
    repl_start();

    while(1) {
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Hello world!");
    }
}