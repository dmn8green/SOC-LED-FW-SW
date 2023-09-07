#include "LedTask.h"

#include "esp_log.h"
#include "led_strip_encoder.h"

#include <memory.h>

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define EXAMPLE_LED_NUMBERS         32
#define EXAMPLE_CHASE_SPEED_MS      10

static const char* TAG = "LED";

void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

void LedTask::svTaskCodeLed( void * pvParameters )
{
    LedTask* task_info = (LedTask*)pvParameters;
    task_info->vTaskCodeLed();
}

void LedTask::vTaskCodeLed()
{
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint16_t hue = 0;
    uint16_t start_rgb = 0;
	int pattern=0;
    int incoming_pattern = 0;
    bool pattern_updated=false;
    int breathe_counter = 0;
    bool increasing = true;

    LedTask* task_info = this;

    while(1) {
        if (xQueueReceive(task_info->queue, &incoming_pattern,  10 / portTICK_PERIOD_MS) == pdTRUE)
		{
            ESP_LOGI(TAG, "Apply pattern %d", incoming_pattern);
            if (incoming_pattern != pattern) {
                pattern = incoming_pattern;
                pattern_updated = true;
            }
		}
        
        if (pattern == 0 && pattern_updated) {
            ESP_LOGI(TAG, "Turning off strip %d", task_info->led_number);
            memset(task_info->led_pixels, 0, EXAMPLE_LED_NUMBERS*3);
            ESP_ERROR_CHECK(rmt_transmit(task_info->led_chan, task_info->led_encoder, task_info->led_pixels, EXAMPLE_LED_NUMBERS*3, &task_info->tx_config));
            ESP_ERROR_CHECK(rmt_tx_wait_all_done(task_info->led_chan, portMAX_DELAY));
            vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
            pattern_updated = false;
            continue;
        }

        if (pattern == 1 && pattern_updated) {
            // All white
            ESP_LOGI(TAG, "Turning on strip %d", task_info->led_number);
            memset(task_info->led_pixels, 255, EXAMPLE_LED_NUMBERS*3);
            ESP_ERROR_CHECK(rmt_transmit(task_info->led_chan, task_info->led_encoder, task_info->led_pixels, EXAMPLE_LED_NUMBERS*3, &task_info->tx_config));
            ESP_ERROR_CHECK(rmt_tx_wait_all_done(task_info->led_chan, portMAX_DELAY));
            pattern_updated = false;
            continue;
        }

        if (pattern == 2) {
            for (int i = 0; i < 3; i++) {
                for (int j = i; j < EXAMPLE_LED_NUMBERS; j += 3) {
                    // Build RGB pixels
                    hue = j * 360 / EXAMPLE_LED_NUMBERS + start_rgb;
                    led_strip_hsv2rgb(hue, 100, 100, &red, &green, &blue);
                    task_info->led_pixels[j * 3 + 0] = green;
                    task_info->led_pixels[j * 3 + 1] = blue;
                    task_info->led_pixels[j * 3 + 2] = red;
                }
            
            // Flush RGB values to LEDs
                ESP_ERROR_CHECK(rmt_transmit(task_info->led_chan, task_info->led_encoder, task_info->led_pixels, EXAMPLE_LED_NUMBERS*3, &task_info->tx_config));
                ESP_ERROR_CHECK(rmt_tx_wait_all_done(task_info->led_chan, portMAX_DELAY));
                vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
                
                memset(task_info->led_pixels, 0, EXAMPLE_LED_NUMBERS*3);
                ESP_ERROR_CHECK(rmt_transmit(task_info->led_chan, task_info->led_encoder, task_info->led_pixels, EXAMPLE_LED_NUMBERS*3, &task_info->tx_config));
                ESP_ERROR_CHECK(rmt_tx_wait_all_done(task_info->led_chan, portMAX_DELAY));
                vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
            }
            start_rgb += 60;
        }

        if (pattern == 3) {
            // do a breathe pattern
            if (breathe_counter == 255) { increasing = false; }
            if (breathe_counter == 1) { increasing = true; }
            if (increasing) { breathe_counter++; }
            else { breathe_counter--; }

            memset(task_info->led_pixels, breathe_counter, EXAMPLE_LED_NUMBERS*3);
            ESP_ERROR_CHECK(rmt_transmit(task_info->led_chan, task_info->led_encoder, task_info->led_pixels, EXAMPLE_LED_NUMBERS*3, &task_info->tx_config));
            ESP_ERROR_CHECK(rmt_tx_wait_all_done(task_info->led_chan, portMAX_DELAY));
            pattern_updated = false;
            // vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
            continue;
        }

        if (pattern == 4) {
            hue = 180;

            if (breathe_counter == 100) { increasing = false; }
            if (breathe_counter == 1) { increasing = true; }
            if (increasing) { breathe_counter++; }
            else { breathe_counter--; }


            led_strip_hsv2rgb(hue, breathe_counter, 100, &red, &green, &blue);

            for (int j = 0; j < EXAMPLE_LED_NUMBERS; j += 3) {
                // Build RGB pixels
                task_info->led_pixels[j * 3 + 0] = green;
                task_info->led_pixels[j * 3 + 1] = blue;
                task_info->led_pixels[j * 3 + 2] = red;
            }

            // memset(task_info->led_pixels, breathe_counter, EXAMPLE_LED_NUMBERS*3);
            ESP_ERROR_CHECK(rmt_transmit(task_info->led_chan, task_info->led_encoder, task_info->led_pixels, EXAMPLE_LED_NUMBERS*3, &task_info->tx_config));
            ESP_ERROR_CHECK(rmt_tx_wait_all_done(task_info->led_chan, portMAX_DELAY));
            pattern_updated = false;

            continue;
        }
        if (pattern == 5) {
            hue = 144;

            if (breathe_counter == 95) { increasing = false; }
            if (breathe_counter == 10) { increasing = true; }
            if (increasing) { breathe_counter++; }
            else { breathe_counter--; }


            led_strip_hsv2rgb(hue, 95, breathe_counter, &red, &green, &blue);

            for (int j = 0; j < EXAMPLE_LED_NUMBERS; j++) {
                // Build RGB pixels
                task_info->led_pixels[j * 3 + 0] = green;
                task_info->led_pixels[j * 3 + 1] = blue;
                task_info->led_pixels[j * 3 + 2] = red;
            }

            ESP_ERROR_CHECK(rmt_transmit(task_info->led_chan, task_info->led_encoder, task_info->led_pixels, EXAMPLE_LED_NUMBERS*3, &task_info->tx_config));
            ESP_ERROR_CHECK(rmt_tx_wait_all_done(task_info->led_chan, portMAX_DELAY));
            pattern_updated = false;

            continue;
        }
    }
}

esp_err_t LedTask::setup(int led_number, int gpio_pin) {
    this->led_number = led_number;
    this->gpio_pin = gpio_pin;
    this->tx_config.loop_count = 0;
    this->led_pixels = (uint8_t*)malloc(EXAMPLE_LED_NUMBERS * 3);

    rmt_tx_channel_config_t tx_chan_config;
    tx_chan_config.clk_src = RMT_CLK_SRC_DEFAULT;
    tx_chan_config.gpio_num = (gpio_num_t) gpio_pin;
    tx_chan_config.mem_block_symbols = 64;
    tx_chan_config.resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ;
    tx_chan_config.trans_queue_depth = 4;
    tx_chan_config.flags.invert_out = true;
    
        // .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        // .gpio_num = gpio_pin,
        // .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        // .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        // .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
        // .flags = {
        //     .invert_out = true, // invert output signal
        // },
        // };

    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &this->led_chan));

    ESP_LOGI(TAG, "Install led strip encoder");
    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
    };

    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &this->led_encoder));

    ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(this->led_chan));

    this->queue = xQueueCreate( 10, sizeof( int ) );

    return ESP_OK;
}

esp_err_t LedTask::start(void) {
    char name[16] = {0};
    snprintf(name, 16, "LED%d", this->led_number);
    xTaskCreatePinnedToCore(
        LedTask::svTaskCodeLed,     // Function to implement the task
        name,                       // Name of the task
        5000,                       // Stack size in words
        this,                       // Task input parameter
        2,                          // Priority
        &this->task_handle,         // Task handle.
        1);                         // Core 1

    return ESP_OK;
}

esp_err_t LedTask::resume(void) {
    vTaskResume(this->task_handle);
    return ESP_OK;
}

esp_err_t LedTask::suspend(void) {
    vTaskSuspend(this->task_handle);
    return ESP_OK;
}

esp_err_t LedTask::stop(void) {
    
    return ESP_OK;
}

esp_err_t LedTask::teardown(void) {
    return ESP_OK;
}

esp_err_t LedTask::set_pattern(int pattern) {
    xQueueSend(this->queue, &pattern, portMAX_DELAY);
    return ESP_OK;
}