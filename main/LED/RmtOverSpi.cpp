//******************************************************************************
/**
 * @file RmtOverSpi.cpp
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief RmtOverSpi class implementation
 * @version 0.1
 * @date 2023-09-19
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//******************************************************************************

#include "RmtOverSpi.h"

#include "esp_log.h"
#include "esp_check.h"

#include <memory.h>

static const char* TAG = "RmtOverSpi";

static const uint8_t mZero = 0b11000000; // 300:900 ns
static const uint8_t mOne  = 0b11111100; // 900:300 ns
static const uint8_t mIdle = 0b11111111; // >=80000ns (send 67x)

//******************************************************************************
esp_err_t RmtOverSpi::setup(spi_host_device_t spi_host, int gpio_num, int led_count) {
    esp_err_t ret = ESP_OK;
    
    this->led_count = led_count;

    //! @note Why +67? That was part of the sample I found on stack overflow.  That's why.
    this->num_bits = led_count * 24 + 67;  // Need some idle time after (+67)
    this->bits = (uint8_t*)malloc(this->num_bits);
    if (this->bits == nullptr) {
        ESP_LOGE(TAG, "Unable to allocate memory for bits");
        return ESP_ERR_NO_MEM;
    }

    for (int i = 0; i < this->num_bits; i++) {
        this->bits[i] = mIdle;
    }

    spi_bus_config_t buscfg = {
            .mosi_io_num     = gpio_num,
            .miso_io_num     = GPIO_NUM_NC,
            .sclk_io_num     = GPIO_NUM_NC,
            .quadwp_io_num   = GPIO_NUM_NC,
            .quadhd_io_num   = GPIO_NUM_NC,
            .data4_io_num    = GPIO_NUM_NC,
            .data5_io_num    = GPIO_NUM_NC,
            .data6_io_num    = GPIO_NUM_NC,
            .data7_io_num    = GPIO_NUM_NC,
            .max_transfer_sz = (int) this->num_bits, // 0 = Default
            .flags           = 0, // SPICOMMON_BUSFLAG_*
            .intr_flags      = 0 };

    // The clock speed is 6.664 MHz.  This is 150 ns per bit.
    // This is in range with the LED spec.
    const spi_device_interface_config_t devcfg = {
            .command_bits     = 0,
            .address_bits     = 0,
            .dummy_bits       = 0,
            .mode             = 1,
            .duty_cycle_pos   = 0,
            .cs_ena_pretrans  = 0,
            .cs_ena_posttrans = 16,
            .clock_speed_hz   = 6'664'000,
            .input_delay_ns   = 0,
            .spics_io_num     = GPIO_NUM_NC,
            .flags            = SPI_DEVICE_NO_DUMMY,
            .queue_size       = 1,
            .pre_cb           = nullptr,
            .post_cb          = nullptr };

    ESP_GOTO_ON_ERROR(
        spi_bus_initialize(spi_host, &buscfg, SPI_DMA_CH_AUTO),
        err_exit,
        TAG,
        "spi_bus_initialize failed"
    );

    ESP_GOTO_ON_ERROR(
        spi_bus_add_device(spi_host, &devcfg, &spi_handle),
        err_exit,
        TAG,
        "Failed to spi_bus_add_device"
    );

    // We have to invert the signal so that it is high when idle.
    esp_rom_gpio_connect_out_signal(gpio_num, spi_periph_signal[spi_host].spid_out, true, false);
    esp_rom_gpio_connect_in_signal(gpio_num, spi_periph_signal[spi_host].spid_in, true);

err_exit:
    return ret;
}

//******************************************************************************
esp_err_t RmtOverSpi::write_led_value_to_strip(uint8_t* pixels) {
    esp_err_t ret = ESP_OK;

    for (int i=0; i<this->led_count * 3; i+=3) {
        set_pixels(i/3, pixels[i+0], pixels[i+1], pixels[i+2]);
    }

    spi_transaction_t trans;
    memset(&trans, 0, sizeof(spi_transaction_t));
    trans.flags     = 0;
    trans.tx_buffer = bits;
    trans.length    = this->num_bits * 8; // Number of bits, ot bytes!

    esp_err_t err = spi_device_queue_trans(spi_handle, &trans, 0);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error sending SPI: %d", err);
    }// else {
    //     ESP_LOGI(TAG, "Sent SPI");
    // }
    vTaskDelay(10 / portTICK_PERIOD_MS);

    return ret;
}

//******************************************************************************
void RmtOverSpi::set_pixels(size_t index, uint8_t g, uint8_t r, uint8_t b) {
    index *= 24;
    uint8_t value;

    value = g;
    bits[index++] = value & 0x80 ? mOne : mZero;
    bits[index++] = value & 0x40 ? mOne : mZero;
    bits[index++] = value & 0x20 ? mOne : mZero;
    bits[index++] = value & 0x10 ? mOne : mZero;
    bits[index++] = value & 0x08 ? mOne : mZero;
    bits[index++] = value & 0x04 ? mOne : mZero;
    bits[index++] = value & 0x02 ? mOne : mZero;
    bits[index++] = value & 0x01 ? mOne : mZero;
    value = r;
    bits[index++] = value & 0x80 ? mOne : mZero;
    bits[index++] = value & 0x40 ? mOne : mZero;
    bits[index++] = value & 0x20 ? mOne : mZero;
    bits[index++] = value & 0x10 ? mOne : mZero;
    bits[index++] = value & 0x08 ? mOne : mZero;
    bits[index++] = value & 0x04 ? mOne : mZero;
    bits[index++] = value & 0x02 ? mOne : mZero;
    bits[index++] = value & 0x01 ? mOne : mZero;
    value = b;
    bits[index++] = value & 0x80 ? mOne : mZero;
    bits[index++] = value & 0x40 ? mOne : mZero;
    bits[index++] = value & 0x20 ? mOne : mZero;
    bits[index++] = value & 0x10 ? mOne : mZero;
    bits[index++] = value & 0x08 ? mOne : mZero;
    bits[index++] = value & 0x04 ? mOne : mZero;
    bits[index++] = value & 0x02 ? mOne : mZero;
    bits[index++] = value & 0x01 ? mOne : mZero;
}
