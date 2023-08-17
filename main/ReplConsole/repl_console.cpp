#include "repl_console.h"

#include "cmd_tasks.h"
#include "cmd_wifi.h"

// Path: main/configuration_interface/configuration_interface.cpp

// eth: Physical lan config
// wifi: Wifi config
// chp: Chargepoint config
// dev: Device specific config (number of leds)
// test: ledpattern/network/eth/wifi/brightness...

static esp_console_repl_t *repl = nullptr;

void repl_configure(uint16_t txPin, uint16_t rxPin, uint32_t baudRate)
{
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();

    repl_config.prompt = "mn8-tools>";

    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));

    register_tasks();
    register_wifi();
}

void repl_start(void)
{
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}