//******************************************************************************
/**
 * @file RmtOverSpi.h
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief RmtOverSpi class definitions
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************


#pragma once

#include "esp_err.h"
#include "esp_rom_gpio.h"
#include "driver/spi_master.h"
#include "soc/spi_periph.h"
#include "hal/gpio_types.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdint.h>

//******************************************************************************
/**
 * @brief RmtOverSpi class
 * 
 * This class is responsible for writing LED values to the LED strip using the
 * SPI peripheral instead of the ESP32 RMT driver.
 * 
 * The ESP32 RMT driver is known to flicker when wifi is enabled.
 */
class RmtOverSpi {
public:
    esp_err_t setup(spi_host_device_t spi_host, int gpio_num, int led_count);
    esp_err_t write_led_value_to_strip(uint8_t* pixels);

private:
    void set_pixels(size_t index, uint8_t g, uint8_t r, uint8_t b);

private:
    uint32_t led_count = 0;
    uint32_t num_bits = 0;
    uint8_t *bits = nullptr;
    spi_device_handle_t spi_handle;
};