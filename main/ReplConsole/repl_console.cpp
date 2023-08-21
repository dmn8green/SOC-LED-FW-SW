#include "repl_console.h"

#include "cmd_tasks.h"
#include "cmd_wifi.h"

// Path: main/configuration_interface/configuration_interface.cpp

// ifconfig (no arg prints all interfaces with info)
// ifconfig eth/wifi dhcp on/off
// ifconfig eth/wifi ipadress netmask gateway
// ifconfig eth/wifi macaddress
// ifconfig eth/wifi status
// ifconfig eth/wifi up/down
//
// join-wifi SSID password
//
// chp: Chargepoint config
// dev: Device specific config (number of leds)
// test: ledpattern/network/eth/wifi/brightness...
// 

static esp_console_repl_t *repl = nullptr;

void repl_configure(uint16_t txPin, uint16_t rxPin, uint16_t channel, uint32_t baudRate)
{
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();

    repl_config.prompt = "mn8-tools>";

    //esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    esp_console_dev_uart_config_t uart_config;
    uart_config.baud_rate = baudRate;
    uart_config.tx_gpio_num = txPin;
    uart_config.rx_gpio_num = rxPin;
    uart_config.channel = channel;
    
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));

    register_tasks();
    register_wifi();
}

void repl_start(void)
{
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
