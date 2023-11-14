#include "rev.h"

#include "Utils/FuseMacAddress.h"

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
    printf(
        "Version is: %d.%d.%d\n", 
        VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH
    );

    // print fuse mac address
    uint8_t mac[6];
    get_fuse_mac_address(mac);

    printf("Fuse MAC address is: %02x:%02x:%02x:%02x:%02x:%02x\n", 
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
    );

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
