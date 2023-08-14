/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"


// #define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM      12
#define RMT_LED_STRIP_RESOLUTION_HZ    10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)

#define LED_FULL_BRIGHT   255

#define COLOR_HOLD_MS               3000
#define COLOR_OFF_MS               500

// With small (<5 ms) delays between updates, some LED strips won't get the update -- I think they may have
// very long reset requirements and we may be missing the reset and addressing imaginary LEDs beyond our
// bar.

// #define EXAMPLE_LED_NUMBERS         100
#define EXAMPLE_LED_NUMBERS         33   // New strip has 3 LEDs per controller
#define EXAMPLE_CHASE_SPEED_MS      15   // May need to be velocity, way different from 33 to 100 chips
#define PULSE_LEVEL_CHANGE_MS       10   // 255 levels, 2.5 sec transition   // doesn't work if too low!

static const char *TAG = "AL LED DEMO:";

static uint8_t led_strip_pixels[EXAMPLE_LED_NUMBERS * 3]; // dynamically used in tranisions (pulse, striping)
static uint8_t led_solid_pixels[EXAMPLE_LED_NUMBERS * 3]; // reference final desired state for pulsing, striping, etc.


void dump_mem (uint8_t *pMem, size_t memSize)
{

    for (int i = 0; i < memSize; i+=3)
    {
        ESP_LOGI(TAG, "LED: %d:  %d %d %d", i/3, pMem[i], pMem[i+1], pMem[i+2]);
    }
    ESP_LOGI(TAG, " ");
}
  
int
fill_mem_solid (uint8_t *pMem, size_t memSize, int color, int level)
{

    int i;

    int targetLevel = (level < 0  || level > LED_FULL_BRIGHT) ?  LED_FULL_BRIGHT : level;

    memset (pMem, 0, memSize);

    ESP_LOGI(TAG, "fill color %d for %d bytes (%d LEDs) at level %d", color, memSize, memSize/3, targetLevel);

    switch (color)
    {
        case 0:  // G
        case 1:  // R
        case 2:  // B
            for (i=0; i < memSize; i+=3)
            {
                *(pMem + i + color) = targetLevel;
            }
            break;

        case 3:  // Y = R+G
            for (i=0; i < memSize; i+=3)
            {
                *(pMem + i + 1) = targetLevel;
                *(pMem + i + 0) = targetLevel;
            }
            break;

        case 4:  // W
            memset (pMem, targetLevel, memSize);
            break;
    }

    // Make the first element lowest blue possible; weird latching issue on BTF WS2811 strip
    *pMem = 0x01;

    return 0;
}

void app_main(void)
{
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint16_t hue = 0;
    uint16_t start_rgb = 0;


    ESP_LOGI(TAG, "LED Chase with %d LEDs", EXAMPLE_LED_NUMBERS);
    ESP_LOGI(TAG, "Create RMT TX channel");
    rmt_channel_handle_t led_chan = NULL;
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = RMT_LED_STRIP_GPIO_NUM,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

    ESP_LOGI(TAG, "Install led strip encoder");
    rmt_encoder_handle_t led_encoder = NULL;
    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));

    ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(led_chan));

    ESP_LOGI(TAG, "Start LED rainbow chase");
    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no transfer loop
    };

    uint8_t pattern=0;

    // Loop through an ad-hoc collection of patterns.

    while (1)
    {

        // Solid colors
        if (pattern < 5)
        {

            fill_mem_solid (led_strip_pixels, sizeof (led_strip_pixels), pattern, -1);

            // only the last LED
            // memset(led_strip_pixels, 0, 32*3 );
            // Flush RGB values to LEDs
            ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
            ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));

            // dump_mem ( led_strip_pixels, sizeof(led_strip_pixels));

            vTaskDelay(pdMS_TO_TICKS(COLOR_HOLD_MS));

            memset(led_strip_pixels, 0, sizeof(led_strip_pixels));
            ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
            ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));

            vTaskDelay(pdMS_TO_TICKS(COLOR_OFF_MS));

        }

        // "Fill" and "Empty" solid color patterns
        else if (pattern < 10)
        {
            fill_mem_solid (led_solid_pixels, sizeof (led_solid_pixels), pattern%5, -1);

            memset(led_strip_pixels, 0, sizeof(led_strip_pixels));

            // Stripe it up!
            // One by one, add solid colors to transmitted array
            for (int led=0; led < EXAMPLE_LED_NUMBERS; led++)
            {
                int i = led * 3;
                led_strip_pixels[i] = led_solid_pixels[i];
                led_strip_pixels[i+1] = led_solid_pixels[i+1];
                led_strip_pixels[i+2] = led_solid_pixels[i+2];

                ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
                ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
                vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
            }

            vTaskDelay(pdMS_TO_TICKS(COLOR_HOLD_MS));

            // Take it down!
            for (int i=sizeof(led_strip_pixels)-3; i >= 0; i-=3)
            {
                // One by one, remove solid colors to transmitted array
                led_strip_pixels[i] = 0;
                led_strip_pixels[i+1] = 0;
                led_strip_pixels[i+2] = 0;

                ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
                ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
                vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
            }

        }

        // Pulse in/out of full color
        else if (pattern < 15)
        {
            // Target. Not displayed
            fill_mem_solid (led_solid_pixels, sizeof (led_solid_pixels), pattern%5, -1);

            for (int level=0; level < LED_FULL_BRIGHT; level++)
            {
                for (int i=0; i < sizeof (led_strip_pixels); i++)
                {
                    if (led_solid_pixels[i] != 0)
                    {
                        led_strip_pixels[i] = level;
                    }
                }
                ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels,
                                                        sizeof(led_strip_pixels), &tx_config));
                ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
                vTaskDelay(pdMS_TO_TICKS(PULSE_LEVEL_CHANGE_MS));
            }

            vTaskDelay(pdMS_TO_TICKS(COLOR_HOLD_MS));

            for (int level=LED_FULL_BRIGHT; level >= 0; level--)
            {
                for (int i=0; i < sizeof (led_strip_pixels); i++)
                {
                    if (led_solid_pixels[i] != 0)
                    {
                        led_strip_pixels[i] = level;
                    }
                }
                ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels,
                                                        sizeof(led_strip_pixels), &tx_config));
                ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
                vTaskDelay(pdMS_TO_TICKS(PULSE_LEVEL_CHANGE_MS));
            }
        }

        else if (pattern == 15)
        {
            // blue flash (x5)
            for (int i = 0; i < 10; i++)
            {

                if (i % 2 == 0)
                {
                    fill_mem_solid (led_strip_pixels, sizeof(led_strip_pixels), 2, -1);
                }
                else
                {
                    memset(led_strip_pixels, 0, sizeof(led_strip_pixels));
                }

                ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels,
                                                        sizeof(led_strip_pixels), &tx_config));
                ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));

                vTaskDelay(pdMS_TO_TICKS(300));
            }

        }

        // Blue SOC demo (20 to 80%) Chasing LED
        else if (pattern == 16)
        {
            for (int soc = 20; soc < 80; soc++)
            {

                size_t solidLeds = soc * EXAMPLE_LED_NUMBERS / 100;

                // Race a single blue LED from SOC to top
                for (int raceLed = solidLeds + 1; raceLed < EXAMPLE_LED_NUMBERS; raceLed++)
                {

                    // All blank
                    memset(led_strip_pixels, 0, sizeof(led_strip_pixels));

                    // Fill SOC level
                    fill_mem_solid (led_strip_pixels, solidLeds*3, 2, -1);  // Solid to SOC

                    // Single one lit
                    int i = (raceLed * 3) + 2;
                    led_strip_pixels[i] = LED_FULL_BRIGHT;

                    ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels,
                                                        sizeof(led_strip_pixels), &tx_config));
                    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));

                    vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS * 3));
                }
            }
        }

        // Blue SOC demo (20 to 80%) pulsing LED
        else if (pattern == 17)
        {
            for (int soc = 40; soc < 50; soc++)
            {

                size_t solidLeds = soc * EXAMPLE_LED_NUMBERS / 100;

                // All blank
                memset(led_strip_pixels, 0, sizeof(led_strip_pixels));

                // Pulse the LEDs above SOC level
                for (int level = 0; level < LED_FULL_BRIGHT; level += 1)
                {
                    fill_mem_solid (led_strip_pixels, solidLeds*3, 2, level);

                    ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels,
                                                        sizeof(led_strip_pixels), &tx_config));
                    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));

                    vTaskDelay(pdMS_TO_TICKS(PULSE_LEVEL_CHANGE_MS));
                }

                for (int level = LED_FULL_BRIGHT; level >= 0; level-=1)
                {
                    fill_mem_solid (led_strip_pixels, solidLeds*3, 2, level);

                    ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels,
                                                        sizeof(led_strip_pixels), &tx_config));
                    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
                    vTaskDelay(pdMS_TO_TICKS(PULSE_LEVEL_CHANGE_MS));
                }
            }
        }


        if (++pattern > 17)
                pattern=0;
    }


#if 0
    while (1) {
        for (int i = 0; i < 3; i++) {
            for (int j = i; j < EXAMPLE_LED_NUMBERS; j += 3) {
                // Build RGB pixels
                hue = j * 360 / EXAMPLE_LED_NUMBERS + start_rgb;
                led_strip_hsv2rgb(hue, 100, 100, &red, &green, &blue);
                led_strip_pixels[j * 3 + 0] = green;
                led_strip_pixels[j * 3 + 1] = blue;
                led_strip_pixels[j * 3 + 2] = red;

                led_strip_pixels[j * 3 + 0] = 0x00;
                led_strip_pixels[j * 3 + 1] = 0xFF;
                led_strip_pixels[j * 3 + 2] = 0x00;
            }
            // Flush RGB values to LEDs
            ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
            ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
            vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
#if 0
            memset(led_strip_pixels, 0, sizeof(led_strip_pixels));

            ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
            ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
            vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
#endif
        }
        start_rgb += 60;
    }
#endif
}
