#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <unistd.h>

#include "esp_log.h"
#include "esp_console.h"
#include "esp_chip_info.h"
#include "esp_sleep.h"
#include "esp_flash.h"

#include "driver/rtc_io.h"
#include "driver/uart.h"
#include "argtable3/argtable3.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

static int ifconfig_cmd(int argc, char **argv)
{
    ESP_LOGI(__func__, "tasks_info");
    return 0;
}

void register_ifconfig(void)
{
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers" 
    const esp_console_cmd_t cmd = {
        .command = "ifconfig",
        .help = "Get information about network interfaces (wifi/ethernet)",
        .hint = NULL,
        .func = &ifconfig_cmd,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}