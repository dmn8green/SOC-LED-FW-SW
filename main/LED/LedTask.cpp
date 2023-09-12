#include "LedTask.h"

#include "esp_log.h"
#include "esp_check.h"
#include "led_strip_encoder.h"

#include <memory.h>

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define EXAMPLE_LED_NUMBERS 32
#define EXAMPLE_CHASE_SPEED_MS 10

static const char *TAG = "LED";

#define COLOR_RED 0xFF0000
#define COLOR_GREEN 0x00FF00
#define COLOR_BLUE 0x0000FF
#define COLOR_WHITE 0xFFFFFF
#define COLOR_BLACK 0x000000
#define COLOR_ORANGE 0xFFA500

void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i)
    {
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

esp_err_t LedTask::write_led_value_to_strip(void)
{
    esp_err_t ret = ESP_OK;

    ESP_GOTO_ON_ERROR(
        rmt_transmit(led_chan, led_encoder, led_pixels, EXAMPLE_LED_NUMBERS * 3, &tx_config),
        err,
        TAG,
        "%d: Failed to transmit LED data", this->led_number);

    ESP_GOTO_ON_ERROR(
        rmt_tx_wait_all_done(this->led_chan, portMAX_DELAY),
        err,
        TAG,
        "%d: Failed to wait for LED data to be transmitted", this->led_number);

err:
    return ret;
}

esp_err_t LedTask::show_static_color(uint32_t color)
{
    esp_err_t ret = ESP_OK;
    ESP_LOGI(TAG, "%d: Showing static color %06lx", this->led_number, color);
    for (int pixel_idx = 0; pixel_idx < EXAMPLE_LED_NUMBERS; pixel_idx += 3)
    {
        this->led_pixels[pixel_idx * 3 + 0] = (color >> 16) & 0xFF;
        this->led_pixels[pixel_idx * 3 + 1] = (color >> 8) & 0xFF;
        this->led_pixels[pixel_idx * 3 + 2] = color & 0xFF;
    }

    ESP_GOTO_ON_ERROR(write_led_value_to_strip(), err, TAG, "%d: Failed to write LED data to strip", this->led_number);

err:
    return ret;
}

esp_err_t LedTask::show_charging_color(uint32_t color_start, uint32_t color_end, uint32_t charge_percent)
{
    esp_err_t ret = ESP_OK;
    ESP_LOGI(TAG, "%d: Showing charging color %ld", this->led_number, charge_percent);
    uint32_t charged_led_count = (charge_percent * EXAMPLE_LED_NUMBERS) / 100;
    ESP_LOGI(TAG, "%d: Charged LED count %ld", this->led_number, charged_led_count);

    for (int pixel_idx = 0; pixel_idx < EXAMPLE_LED_NUMBERS; pixel_idx += 3)
    {
        uint32_t color = charged_led_count > pixel_idx ? color_end : color_start;

        this->led_pixels[pixel_idx * 3 + 0] = (color >> 16) & 0xFF;
        this->led_pixels[pixel_idx * 3 + 1] = (color >> 8) & 0xFF;
        this->led_pixels[pixel_idx * 3 + 2] = color & 0xFF;
    }

    ESP_GOTO_ON_ERROR(write_led_value_to_strip(), err, TAG, "%d: Failed to write LED data to strip", this->led_number);

err:
    return ret;
}

void LedTask::vTaskCodeLed()
{
    LedTask *task_info = this;
    TickType_t queue_timeout = portMAX_DELAY;

    led_state_info_t updated_state;
    bool needs_refreshing = true;
    esp_err_t ret = ESP_OK;

    while (1)
    {
        if (xQueueReceive(task_info->state_update_queue, &updated_state, queue_timeout / portTICK_PERIOD_MS) == pdTRUE)
        {
            ESP_LOGI(TAG, "%d: Switching to state %d", this->led_number, updated_state.state);
            if (state_info.state != updated_state.state || state_info.charge_percent != updated_state.charge_percent)
            {
                state_info = updated_state;
                needs_refreshing = true;
            }
        }

        if (needs_refreshing)
        {
            switch (state_info.state)
            {
            case e_station_available:
                ESP_LOGI(TAG, "%d: Station is available", this->led_number);
                ret = task_info->show_static_color(COLOR_GREEN);

                queue_timeout = portMAX_DELAY;
                needs_refreshing = false;
                break;
            case e_station_unreachable:
                ESP_LOGI(TAG, "%d: Station is unreachable", this->led_number);
                ret = task_info->show_static_color(COLOR_RED);
                queue_timeout = portMAX_DELAY;
                needs_refreshing = false;
                break;
            case e_station_status_unknown:
                ESP_LOGI(TAG, "%d: Station status is unknown", this->led_number);
                ret = task_info->show_static_color(COLOR_RED);
                queue_timeout = portMAX_DELAY;
                needs_refreshing = false;
                break;
            case e_station_in_use:
                ESP_LOGI(TAG, "%d: Station is in use", this->led_number);
                ret = task_info->show_charging_color(COLOR_BLUE, COLOR_WHITE, state_info.charge_percent);
                queue_timeout = portMAX_DELAY;
                needs_refreshing = false;
                break;
            case e_station_reserved:
                ESP_LOGI(TAG, "%d: Station is reserved", this->led_number);
                ret = task_info->show_static_color(COLOR_ORANGE);
                queue_timeout = portMAX_DELAY;
                needs_refreshing = false;
                break;
            case e_station_test_charging:
                ESP_LOGI(TAG, "%d: Station is test charging for debugging purposes %d", this->led_number, state_info.charge_percent);
                state_info.charge_percent = state_info.charge_percent == 100 ? 0 : state_info.charge_percent + 1;
                ret = task_info->show_charging_color(COLOR_RED, COLOR_GREEN, state_info.charge_percent);

                // Keep going one second at a time, increasing the charge percent
                queue_timeout = 10 * 1000;
                needs_refreshing = true;
                break;
            default:
                ESP_LOGE(TAG, "%d: Unknown state %d", this->led_number, state_info.state);
                break;
            }

            // TODO: FIGURE OUT WHAT TO DO IF WE CAN'T CHANGE THE LED STATE
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "%d: Failed to show state %d", this->led_number, state_info.state);
            }
        }

        /* #region for reference until we have the pulsing implemented. */
        // if (pattern == 0 && pattern_updated) {
        //     ESP_LOGI(TAG, "Turning off strip %d", task_info->led_number);
        //     memset(task_info->led_pixels, 0, EXAMPLE_LED_NUMBERS*3);
        //     ESP_ERROR_CHECK(rmt_transmit(task_info->led_chan, task_info->led_encoder, task_info->led_pixels, EXAMPLE_LED_NUMBERS*3, &task_info->tx_config));
        //     ESP_ERROR_CHECK(rmt_tx_wait_all_done(task_info->led_chan, portMAX_DELAY));
        //     vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
        //     pattern_updated = false;
        //     continue;
        // }

        // if (pattern == 1 && pattern_updated) {
        //     // All white
        //     ESP_LOGI(TAG, "Turning on strip %d", task_info->led_number);
        //     memset(task_info->led_pixels, 255, EXAMPLE_LED_NUMBERS*3);
        //     ESP_ERROR_CHECK(rmt_transmit(task_info->led_chan, task_info->led_encoder, task_info->led_pixels, EXAMPLE_LED_NUMBERS*3, &task_info->tx_config));
        //     ESP_ERROR_CHECK(rmt_tx_wait_all_done(task_info->led_chan, portMAX_DELAY));
        //     pattern_updated = false;
        //     continue;
        // }

        // if (pattern == 2) {
        //     for (int i = 0; i < 3; i++) {
        //         for (int j = i; j < EXAMPLE_LED_NUMBERS; j += 3) {
        //             // Build RGB pixels
        //             hue = j * 360 / EXAMPLE_LED_NUMBERS + start_rgb;
        //             led_strip_hsv2rgb(hue, 100, 100, &red, &green, &blue);
        //             task_info->led_pixels[j * 3 + 0] = green;
        //             task_info->led_pixels[j * 3 + 1] = blue;
        //             task_info->led_pixels[j * 3 + 2] = red;
        //         }

        //     // Flush RGB values to LEDs
        //         ESP_ERROR_CHECK(rmt_transmit(task_info->led_chan, task_info->led_encoder, task_info->led_pixels, EXAMPLE_LED_NUMBERS*3, &task_info->tx_config));
        //         ESP_ERROR_CHECK(rmt_tx_wait_all_done(task_info->led_chan, portMAX_DELAY));
        //         vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));

        //         memset(task_info->led_pixels, 0, EXAMPLE_LED_NUMBERS*3);
        //         ESP_ERROR_CHECK(rmt_transmit(task_info->led_chan, task_info->led_encoder, task_info->led_pixels, EXAMPLE_LED_NUMBERS*3, &task_info->tx_config));
        //         ESP_ERROR_CHECK(rmt_tx_wait_all_done(task_info->led_chan, portMAX_DELAY));
        //         vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
        //     }
        //     start_rgb += 60;
        //     queue_timeout = 10;
        // }

        //     if (pattern == 3) {
        //         // do a breathe pattern
        //         if (breathe_counter == 255) { increasing = false; }
        //         if (breathe_counter == 1) { increasing = true; }
        //         if (increasing) { breathe_counter++; }
        //         else { breathe_counter--; }

        //         memset(task_info->led_pixels, breathe_counter, EXAMPLE_LED_NUMBERS*3);
        //         ESP_ERROR_CHECK(rmt_transmit(task_info->led_chan, task_info->led_encoder, task_info->led_pixels, EXAMPLE_LED_NUMBERS*3, &task_info->tx_config));
        //         ESP_ERROR_CHECK(rmt_tx_wait_all_done(task_info->led_chan, portMAX_DELAY));
        //         pattern_updated = false;
        //         // vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
        //         continue;
        //     }

        //     if (pattern == 4) {
        //         hue = 180;

        //         if (breathe_counter == 100) { increasing = false; }
        //         if (breathe_counter == 1) { increasing = true; }
        //         if (increasing) { breathe_counter++; }
        //         else { breathe_counter--; }

        //         led_strip_hsv2rgb(hue, breathe_counter, 100, &red, &green, &blue);

        //         for (int j = 0; j < EXAMPLE_LED_NUMBERS; j += 3) {
        //             // Build RGB pixels
        //             task_info->led_pixels[j * 3 + 0] = green;
        //             task_info->led_pixels[j * 3 + 1] = blue;
        //             task_info->led_pixels[j * 3 + 2] = red;
        //         }

        //         // memset(task_info->led_pixels, breathe_counter, EXAMPLE_LED_NUMBERS*3);
        //         ESP_ERROR_CHECK(rmt_transmit(task_info->led_chan, task_info->led_encoder, task_info->led_pixels, EXAMPLE_LED_NUMBERS*3, &task_info->tx_config));
        //         ESP_ERROR_CHECK(rmt_tx_wait_all_done(task_info->led_chan, portMAX_DELAY));
        //         pattern_updated = false;
        //         queue_timeout = 10;

        //         continue;
        //     }
        //     if (pattern == 5) {
        //         hue = 144;

        //         if (breathe_counter == 95) { increasing = false; }
        //         if (breathe_counter == 10) { increasing = true; }
        //         if (increasing) { breathe_counter++; }
        //         else { breathe_counter--; }

        //         led_strip_hsv2rgb(hue, 95, breathe_counter, &red, &green, &blue);

        //         for (int j = 0; j < EXAMPLE_LED_NUMBERS; j++) {
        //             // Build RGB pixels
        //             task_info->led_pixels[j * 3 + 0] = green;
        //             task_info->led_pixels[j * 3 + 1] = blue;
        //             task_info->led_pixels[j * 3 + 2] = red;
        //         }

        //         ESP_ERROR_CHECK(rmt_transmit(task_info->led_chan, task_info->led_encoder, task_info->led_pixels, EXAMPLE_LED_NUMBERS*3, &task_info->tx_config));
        //         ESP_ERROR_CHECK(rmt_tx_wait_all_done(task_info->led_chan, portMAX_DELAY));
        //         pattern_updated = false;
        //         queue_timeout = 10;

        //         continue;
        //     }
        /* #endregion */
    }
}

esp_err_t LedTask::setup(int led_number, int gpio_pin)
{
    esp_err_t ret = ESP_OK;

    this->led_number = led_number;
    this->gpio_pin = gpio_pin;
    this->tx_config.loop_count = 0;
    this->led_pixels = (uint8_t *)malloc(EXAMPLE_LED_NUMBERS * 3);

    rmt_tx_channel_config_t tx_chan_config;
    tx_chan_config.clk_src = RMT_CLK_SRC_DEFAULT;
    tx_chan_config.gpio_num = (gpio_num_t)gpio_pin;
    tx_chan_config.mem_block_symbols = 64;
    tx_chan_config.resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ;
    tx_chan_config.trans_queue_depth = 4;
    tx_chan_config.flags.invert_out = true;

    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
    };

    ESP_GOTO_ON_ERROR(rmt_new_tx_channel(&tx_chan_config, &this->led_chan), err, TAG, "%d: Failed to create RMT TX channel", this->led_number);

    ESP_LOGI(TAG, "%d: Install led strip encoder", this->led_number);
    ESP_GOTO_ON_ERROR(rmt_new_led_strip_encoder(&encoder_config, &this->led_encoder), err, TAG, "%d: Failed to install led strip encoder", this->led_number);

    ESP_LOGI(TAG, "%d: Enable RMT TX channel", this->led_number);
    ESP_GOTO_ON_ERROR(rmt_enable(this->led_chan), err, TAG, "%d: Failed to enable RMT TX channel", this->led_number);

    this->queue = xQueueCreate(10, sizeof(int));
    this->state_update_queue = xQueueCreate(10, sizeof(led_state_info_t));

err:
    return ret;
}

esp_err_t LedTask::start(void)
{
    char name[16] = {0};
    snprintf(name, 16, "LED%d", this->led_number);
    xTaskCreatePinnedToCore(
        LedTask::svTaskCodeLed, // Function to implement the task
        name,                   // Name of the task
        5000,                   // Stack size in words
        this,                   // Task input parameter
        2,                      // Priority
        &this->task_handle,     // Task handle.
        1);                     // Core 1

    return ESP_OK;
}

esp_err_t LedTask::resume(void)
{
    vTaskResume(this->task_handle);
    return ESP_OK;
}

esp_err_t LedTask::suspend(void)
{
    vTaskSuspend(this->task_handle);
    return ESP_OK;
}

esp_err_t LedTask::stop(void)
{

    return ESP_OK;
}

esp_err_t LedTask::teardown(void)
{
    return ESP_OK;
}

esp_err_t LedTask::set_pattern(int pattern)
{
    xQueueSend(this->queue, &pattern, portMAX_DELAY);
    return ESP_OK;
}

esp_err_t LedTask::set_state(const char *new_state, int charge_percent)
{
    led_state_info_t state_info;
         if (strcmp(new_state, "available") == 0)     { state_info.state = e_station_available; }
    else if (strcmp(new_state, "unreachable") == 0)   { state_info.state = e_station_unreachable; }
    else if (strcmp(new_state, "unknown") == 0)       { state_info.state = e_station_status_unknown; }
    else if (strcmp(new_state, "in-use") == 0)        { state_info.state = e_station_in_use; }
    else if (strcmp(new_state, "reserved") == 0)      { state_info.state = e_station_reserved; }
    else if (strcmp(new_state, "test_charging") == 0) { state_info.state = e_station_test_charging; }
    else {
        ESP_LOGE(TAG, "%d: Unknown state %s", this->led_number, new_state);
        return ESP_FAIL;
    }

    state_info.charge_percent = charge_percent;
    xQueueSend(this->state_update_queue, &state_info, portMAX_DELAY);
    return ESP_OK;
}