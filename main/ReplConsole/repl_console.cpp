#include "repl_console.h"

#include "cmd_tasks.h"
#include "cmd_wifi.h"
#include "cmd_ifconfig.h"
#include "cmd_reboot.h"
#include "cmd_ping.h"
#include "cmd_iot.h"
#include "cmd_led.h"
#include "cmd_chargepoint.h"
#include "cmd_factory_reset.h"
#include "cmd_info.h"

#include "App/MN8App.h"
#include "App/Configuration/ThingConfig.h"


static esp_console_repl_t *repl = nullptr;

void repl_configure(uint16_t txPin, uint16_t rxPin, uint16_t channel, uint32_t baudRate)
{
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();

    repl_config.prompt = "mn8-tools>";

    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    // esp_console_dev_uart_config_t uart_config;
    // uart_config.baud_rate = baudRate;
    // uart_config.tx_gpio_num = txPin;
    // uart_config.rx_gpio_num = rxPin;
    // uart_config.channel = channel;
    
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));

    MN8App& app = MN8App::instance();

    // System commands
    register_tasks();
    register_reboot();
    register_ping();
    register_info_command();
    register_factory_reset_command();

    // LED testing commands
    register_led();

    // App commands
    register_wifi();
    register_ifconfig();
    register_iot_command();

    ThingConfig thingConfig;
    thingConfig.load();
    if (thingConfig.is_configured()) {
        register_chargepoint_command();
    }

}

void repl_start(void)
{
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
