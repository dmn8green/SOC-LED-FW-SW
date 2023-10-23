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
#include "esp_intr_alloc.h"
#include "freertos/FreeRTOS.h"

#define CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
#include "freertos/task.h"

#include "sdkconfig.h"

static int do_info_command(int argc, char **argv)
{
    printf("Version is:\n    Major: %d\n    Minor: %d\n    Patch: %d\n", 1, 0, 0);
    
    return 0;
}

void register_info_command(void)
{
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers" 
    const esp_console_cmd_t cmd = {
        .command = "info",
        .help = "Print information about the system",
        .hint = NULL,
        .func = &do_info_command,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}
