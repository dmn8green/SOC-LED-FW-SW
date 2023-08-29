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

static int reboot(int argc, char **argv)
{
    esp_restart();
    return 0;
}

void register_reboot(void)
{
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers" 
    const esp_console_cmd_t cmd = {
        .command = "reboot",
        .help = "Reboot board",
        .hint = NULL,
        .func = &reboot,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}
