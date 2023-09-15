#include "LedTaskSpi.h"

#include "esp_log.h"
#include "esp_check.h"
#include "led_strip_encoder.h"
#include "esp_rom_gpio.h"
#include "soc/spi_periph.h"

#include <memory.h>
#include <math.h>

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define EXAMPLE_LED_NUMBERS 32
#define EXAMPLE_CHASE_SPEED_MS 10

static const char *TAG = "LED";

#define COLOR_RED 0xFF0000
#define COLOR_GREEN 0x00FF00
#define COLOR_BLUE 0x0000FF
#define COLOR_WHITE 0x808080
#define COLOR_BLACK 0x000000
#define COLOR_ORANGE 0xFF7000
#define COLOR_CYAN 0x00FF80
#define COLOR_YELLOW 0xFFFF00

#define COLOR_YELLOW_HSV 60, 100, 100
#define COLOR_BLUE_HSV 240, 100, 100
#define COLOR_WHITE_HSV 0, 0, 100

float max(float a, float b, float c) {
   return ((a > b)? (a > c ? a : c) : (b > c ? b : c));
}
float min(float a, float b, float c) {
   return ((a < b)? (a < c ? a : c) : (b < c ? b : c));
}

//static const uint8_t mZero = 0b11000000; // 300:900 ns
//static const uint8_t mOne  = 0b11111100; // 900:300 ns
static const uint8_t mZero = 0b11000000; // 300:900 ns
static const uint8_t mOne  = 0b11111100; // 900:300 ns
static const uint8_t mIdle = 0b11111111; // >=80000ns (send 67x)

void RGBtoHSV(float fR, float fG, float fB, float& fH, float& fS, float& fV) {
  float fCMax = max(fR, fG, fB);
  float fCMin = min(fR, fG, fB);
  float fDelta = fCMax - fCMin;
  
  if(fDelta > 0) {
    if(fCMax == fR) {
      fH = 60 * (fmod(((fG - fB) / fDelta), 6));
    } else if(fCMax == fG) {
      fH = 60 * (((fB - fR) / fDelta) + 2);
    } else if(fCMax == fB) {
      fH = 60 * (((fR - fG) / fDelta) + 4);
    }
    
    if(fCMax > 0) {
      fS = fDelta / fCMax;
    } else {
      fS = 0;
    }
    
    fV = fCMax;
  } else {
    fH = 0;
    fS = 0;
    fV = fCMax;
  }
  
  if(fH < 0) {
    fH = 360 + fH;
  }
}


void hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
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

void rgv2hsv(uint32_t rgb, uint32_t * hsv_color) {
    uint32_t r = (rgb >> 16) & 0xFF;
    uint32_t g = (rgb >> 8) & 0xFF;
    uint32_t b = rgb & 0xFF;
    
    float h, s, v;
    RGBtoHSV(r, g, b, h, s, v);
    hsv_color[0] = h;
    hsv_color[1] = s;
    hsv_color[2] = v;

}

uint32_t hsv2rgb(uint32_t hsv) {
    uint32_t h = (hsv >> 16) & 0xFF;
    uint32_t s = (hsv >> 8) & 0xFF;
    uint32_t v = hsv & 0xFF;

    uint32_t r, g, b;
    hsv2rgb(h, s, v, &r, &g, &b);

    return (r << 16) | (g << 8) | b;
}

int refresh_rates[101];

void StaticColorMode::refresh(uint8_t* led_pixels, int led_count, int start_pixel) {
    for (int pixel_idx = start_pixel; pixel_idx < led_count; pixel_idx++)
    {
        led_pixels[pixel_idx * 3 + 0] = (color >> 8) & 0xFF;
        led_pixels[pixel_idx * 3 + 1] = (color >> 16) & 0xFF;
        led_pixels[pixel_idx * 3 + 2] = color & 0xFF;
    }
}

void ChargingColorMode::refresh(uint8_t* led_pixels, int led_count, int start_pixel) {
    uint32_t charged_led_count = (charge_percent * led_count) / 100;

    for (int pixel_idx = start_pixel; pixel_idx < led_count; pixel_idx++)
    {
        uint32_t color = pixel_idx > charged_led_count ? color_end : color_start;

        led_pixels[pixel_idx * 3 + 0] = (color >> 8) & 0xFF;
        led_pixels[pixel_idx * 3 + 1] = (color >> 16) & 0xFF;
        led_pixels[pixel_idx * 3 + 2] = color & 0xFF;
    }
}

void ChargingColorPulseMode::reset(uint32_t color_start, uint32_t h, uint32_t s, uint32_t v, uint32_t min_value, uint32_t rate) {
    this->static_color_mode.set_color(color_start);
    this->pulsing_color_mode.reset(h, s, v, min_value, rate);
    charge_percent = charge_percent == 100 ? 0 : charge_percent + 1;
    this->set_rate(rate);
}

void ChargingColorPulseMode::refresh(uint8_t* led_pixels, int led_count, int start_pixel) {
    uint32_t charged_led_count = (charge_percent * led_count) / 100;
    this->static_color_mode.refresh(led_pixels, charged_led_count, 0);
    this->pulsing_color_mode.refresh(led_pixels, led_count, charged_led_count);

    
    if ((++cheat) % 100 == 0) {
        charge_percent = charge_percent == 100 ? 0 : charge_percent + 1;    
    }
}

void TestChargingColorMode::refresh(uint8_t* led_pixels, int led_count, int start_pixel) {
    this->ChargingColorMode::refresh(led_pixels, led_count);
    charge_percent = charge_percent == 100 ? 0 : charge_percent + 1;
}

void PulsingColorMode::reset(uint32_t h, uint32_t s, uint32_t v, uint32_t min_value, uint32_t rate) {
    this->hsv_color[0] = h;
    this->hsv_color[1] = s;
    this->hsv_color[2] = v;
    this->max_value = 100;//v;
    this->min_value = 0;//min_value;

    ESP_LOGI(TAG, "HSV: %ld, %ld, %ld", this->hsv_color[0], this->hsv_color[1], this->hsv_color[2]);

    this->pulse_count = this->min_value;
    this->increasing = true;
    this->set_rate(rate);
}

void PulsingColorMode::refresh(uint8_t* led_pixels, int led_count, int start_pixel) {
    #define AT_MIN_VALUE (this->pulse_count == this->min_value)
    #define AT_MAX_VALUE (this->pulse_count == this->max_value)

    uint32_t red, green, blue;
    uint32_t hue, saturation, value;

    hue = this->hsv_color[0];//139;//(hsv_color >> 16) & 0xFF;
    saturation = this->hsv_color[1];//(hsv_color >> 8) & 0xFF;
    value = this->pulse_count;
    // ESP_LOGI(TAG, "HSV: %ld, %ld, %ld", hue, saturation, value);

    // If we are at the min or max value, then we need to change direction.
    uint32_t range = this->max_value - this->min_value;
    uint32_t pulse_rate_idx = 100.0*(this->pulse_count-this->min_value)/range;
    this->set_rate(refresh_rates[pulse_rate_idx] + 10);
    this->pulse_count = this->increasing ? this->pulse_count + 1 : this->pulse_count - 1;
    this->increasing = AT_MIN_VALUE ? true : AT_MAX_VALUE ? false : this->increasing;
    //ESP_LOGI(TAG, "%ld: Pulse count: %ld", this->led_number, this->pulse_count);


    hsv2rgb(hue, saturation, value, &red, &green, &blue);

    for (int pixel_idx = start_pixel; pixel_idx < led_count; pixel_idx++) {
        // Build RGB pixels
        led_pixels[pixel_idx * 3 + 0] = green;
        led_pixels[pixel_idx * 3 + 1] = red;
        led_pixels[pixel_idx * 3 + 2] = blue;
    }
}


static portMUX_TYPE my_spinlock = portMUX_INITIALIZER_UNLOCKED;
#define NUMBITS (EXAMPLE_LED_NUMBERS * 24 + 67)


void LedTaskSpi::setPixel(size_t index, uint8_t g, uint8_t r, uint8_t b) {
    index *= 24;
    uint8_t value;

    if (bits == nullptr) {
        bits = (uint8_t *)malloc(NUMBITS);
        memset(bits, mIdle, NUMBITS);
    }

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

esp_err_t LedTaskSpi::write_led_value_to_strip(void)
{
    esp_err_t ret = ESP_OK;

    for (int i=0; i<EXAMPLE_LED_NUMBERS * 3; i+=3) {
        setPixel(i/3, led_pixels[i+0], led_pixels[i+1], led_pixels[i+2]);
    }
    //setPixel(EXAMPLE_LED_NUMBERS, led_pixels[0], led_pixels[1], led_pixels[2]);

    spi_transaction_t trans;
    memset(&trans, 0, sizeof(spi_transaction_t));
    trans.flags     = 0;
    trans.tx_buffer = bits;
    trans.length    = NUMBITS * 8; // Number of bits, ot bytes!

    esp_err_t err = spi_device_queue_trans(spiHandle, &trans, 0);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error sending SPI: %d", err);
    }// else {
    //     ESP_LOGI(TAG, "Sent SPI");
    // }
    vTaskDelay(10 / portTICK_PERIOD_MS);

    return ret;
}

void LedTaskSpi::vTaskCodeLed()
{
    LedTaskSpi *task_info = this;
    TickType_t queue_timeout = portMAX_DELAY;

    // Start with a state changed. On boot, the state is set to e_station_booting_up
    // in the setup phase. This will cause the LED to be set to yellow.
    bool state_changed = true;

    led_state_info_t updated_state;
    esp_err_t ret = ESP_OK;

    do
    {
        if (state_changed)
        {
            switch (state_info.state)
            {
            case e_station_available:
                ESP_LOGI(TAG, "%d: Station is available", this->led_number);
                this->static_color_mode.set_color(COLOR_GREEN);
                this->color_mode = &this->static_color_mode;
                break;
            case e_station_waiting_for_power:
                ESP_LOGI(TAG, "%d: Station is waiting for power", this->led_number);
                this->static_color_mode.set_color(COLOR_CYAN);
                this->color_mode = &this->static_color_mode;
                break;
            case e_station_charging:
                ESP_LOGI(TAG, "%d: Charging", this->led_number);
                this->charging_color_mode.set_color_start(COLOR_BLUE);
                this->charging_color_mode.set_color_end(COLOR_WHITE);
                this->charging_color_mode.set_charge_percent(state_info.charge_percent);
                this->color_mode = &this->charging_color_mode;
                break;
            case e_station_charging_pulse:
                ESP_LOGI(TAG, "%d: Charging pulse", this->led_number);
                this->charging_color_pulse_mode.reset(COLOR_BLUE, COLOR_WHITE_HSV, 30, 10);
                this->charging_color_pulse_mode.set_charge_percent(state_info.charge_percent);
                this->color_mode = &this->charging_color_pulse_mode;
                break;
            case e_station_charging_complete:
                ESP_LOGI(TAG, "%d: Charging complete", this->led_number);
                this->static_color_mode.set_color(COLOR_BLUE);
                this->color_mode = &this->static_color_mode;
                break;
            case e_station_out_of_service:
                ESP_LOGI(TAG, "%d: Station is out of service", this->led_number);
                this->static_color_mode.set_color(COLOR_RED);
                this->color_mode = &this->static_color_mode;
                break;
            case e_station_disable:
                ESP_LOGI(TAG, "%d: Station is disabled", this->led_number);
                this->static_color_mode.set_color(COLOR_RED);
                this->color_mode = &this->static_color_mode;
                break;
            case e_station_booting_up:
                //ESP_LOGI(TAG, "%d: Station is booting up", this->led_number);
                this->pulsing_color_mode.reset(COLOR_YELLOW_HSV, 20, 20);
                this->color_mode = &this->pulsing_color_mode;
                break;
            case e_station_offline:
                ESP_LOGI(TAG, "%d: Station is offline", this->led_number);
                this->static_color_mode.set_color(COLOR_WHITE);
                this->color_mode = &this->static_color_mode;
                break;
            case e_station_reserved:
                ESP_LOGI(TAG, "%d: Station is reserved", this->led_number);
                this->static_color_mode.set_color(COLOR_ORANGE);
                this->color_mode = &this->static_color_mode;
                break;
            case e_station_test_charging:
                ESP_LOGI(TAG, "%d: Station is test charging for debugging purposes %d", this->led_number, state_info.charge_percent);
                this->test_charging_color_mode.set_color_start(COLOR_BLUE);
                this->test_charging_color_mode.set_color_end(COLOR_WHITE);
                this->test_charging_color_mode.set_charge_percent(state_info.charge_percent);
                this->test_charging_color_mode.set_rate(1 * 1000); // 10 seconds.
                this->color_mode = &this->test_charging_color_mode;
                break;
            default:
                ESP_LOGE(TAG, "%d: Unknown state %d", this->led_number, state_info.state);
                break;
            }

            //this->color_mode->reset();
            state_changed = false;

            // TODO: FIGURE OUT WHAT TO DO IF WE CAN'T CHANGE THE LED STATE
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "%d: Failed to show state %d", this->led_number, state_info.state);
            }
        }

        if (this->color_mode != NULL) {
            this->color_mode->refresh(this->led_pixels, EXAMPLE_LED_NUMBERS);
            this->write_led_value_to_strip();
            queue_timeout = this->color_mode->get_rate() / portTICK_PERIOD_MS;
            // ESP_LOGI(TAG, "%d: Queue timeout: %ld", this->led_number, queue_timeout);
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

        if (xQueueReceive(task_info->state_update_queue, &updated_state, queue_timeout) == pdTRUE)
        {
            ESP_LOGI(TAG, "%d: Switching to state %d", this->led_number, updated_state.state);
            if (state_info.state != updated_state.state || state_info.charge_percent != updated_state.charge_percent)
            {
                state_info = updated_state;
                state_changed = true;
            } else {
                state_changed = false;
            }
        }
    } while(true);
}

esp_err_t LedTaskSpi::setup(int led_number, int gpio_pin, spi_host_device_t spinum)
{
    esp_err_t ret = ESP_OK;
    
refresh_rates[0]=20;
refresh_rates[1]=20;
refresh_rates[2]=20;
refresh_rates[3]=20;
refresh_rates[4]=20;
refresh_rates[5]=20;
refresh_rates[6]=20;
refresh_rates[7]=20;
refresh_rates[8]=20;
refresh_rates[9]=20;
refresh_rates[10]=20;
refresh_rates[11]=20;
refresh_rates[12]=19;
refresh_rates[13]=19;
refresh_rates[14]=19;
refresh_rates[15]=19;
refresh_rates[16]=19;
refresh_rates[17]=19;
refresh_rates[18]=19;
refresh_rates[19]=18;
refresh_rates[20]=18;
refresh_rates[21]=18;
refresh_rates[22]=17;
refresh_rates[23]=17;
refresh_rates[24]=16;
refresh_rates[25]=16;
refresh_rates[26]=15;
refresh_rates[27]=15;
refresh_rates[28]=14;
refresh_rates[29]=14;
refresh_rates[30]=13;
refresh_rates[31]=12;
refresh_rates[32]=11;
refresh_rates[33]=10;
refresh_rates[34]=10;
refresh_rates[35]=9;
refresh_rates[36]=8;
refresh_rates[37]=7;
refresh_rates[38]=6;
refresh_rates[39]=5;
refresh_rates[40]=5;
refresh_rates[41]=4;
refresh_rates[42]=3;
refresh_rates[43]=2;
refresh_rates[44]=2;
refresh_rates[45]=1;
refresh_rates[46]=1;
refresh_rates[47]=0;
refresh_rates[48]=0;
refresh_rates[49]=0;
refresh_rates[50]=0;
refresh_rates[51]=0;
refresh_rates[52]=0;
refresh_rates[53]=0;
refresh_rates[54]=1;
refresh_rates[55]=1;
refresh_rates[56]=2;
refresh_rates[57]=2;
refresh_rates[58]=3;
refresh_rates[59]=4;
refresh_rates[60]=5;
refresh_rates[61]=5;
refresh_rates[62]=6;
refresh_rates[63]=7;
refresh_rates[64]=8;
refresh_rates[65]=9;
refresh_rates[66]=10;
refresh_rates[67]=10;
refresh_rates[68]=11;
refresh_rates[69]=12;
refresh_rates[70]=13;
refresh_rates[71]=14;
refresh_rates[72]=14;
refresh_rates[73]=15;
refresh_rates[74]=15;
refresh_rates[75]=16;
refresh_rates[76]=16;
refresh_rates[77]=17;
refresh_rates[78]=17;
refresh_rates[79]=18;
refresh_rates[80]=18;
refresh_rates[81]=18;
refresh_rates[82]=19;
refresh_rates[83]=19;
refresh_rates[84]=19;
refresh_rates[85]=19;
refresh_rates[86]=19;
refresh_rates[87]=19;
refresh_rates[88]=19;
refresh_rates[89]=20;
refresh_rates[90]=20;
refresh_rates[91]=20;
refresh_rates[92]=20;
refresh_rates[93]=20;
refresh_rates[94]=20;
refresh_rates[95]=20;
refresh_rates[96]=20;
refresh_rates[97]=20;
refresh_rates[98]=20;
refresh_rates[99]=20;
refresh_rates[100]=20;

    this->led_number = led_number;
    this->gpio_pin = gpio_pin;
    this->led_pixels = (uint8_t *)malloc(EXAMPLE_LED_NUMBERS * 3);

    spi_bus_config_t buscfg = {
            .mosi_io_num     = gpio_pin,
            .miso_io_num     = GPIO_NUM_NC,
            .sclk_io_num     = GPIO_NUM_NC,
            .quadwp_io_num   = GPIO_NUM_NC,
            .quadhd_io_num   = GPIO_NUM_NC,
            .data4_io_num    = GPIO_NUM_NC,
            .data5_io_num    = GPIO_NUM_NC,
            .data6_io_num    = GPIO_NUM_NC,
            .data7_io_num    = GPIO_NUM_NC,
            .max_transfer_sz = NUMBITS, // 0 = Default
            .flags           = 0, // SPICOMMON_BUSFLAG_*
            .intr_flags      = 0 };

    ESP_ERROR_CHECK(spi_bus_initialize(spinum, &buscfg, SPI_DMA_CH_AUTO));

    const spi_device_interface_config_t devcfg = {
            .command_bits     = 0,
            .address_bits     = 0,
            .dummy_bits       = 0,
            .mode             = 1,
            .duty_cycle_pos   = 0,
            .cs_ena_pretrans  = 0,
            .cs_ena_posttrans = 16,
            .clock_speed_hz   = 4000000, 
            .input_delay_ns   = 0,
            .spics_io_num     = GPIO_NUM_NC,
            .flags            = SPI_DEVICE_NO_DUMMY,
            .queue_size       = 1,
            .pre_cb           = nullptr,
            .post_cb          = nullptr };

    ESP_ERROR_CHECK(spi_bus_add_device(spinum, &devcfg, &spiHandle));

        esp_rom_gpio_connect_out_signal(gpio_pin, spi_periph_signal[spinum].spid_out, true, false);
        esp_rom_gpio_connect_in_signal(gpio_pin, spi_periph_signal[spinum].spid_in, true);
        
    this->state_update_queue = xQueueCreate(10, sizeof(led_state_info_t));
    this->state_info.state = e_station_booting_up;

    return ret;
}

esp_err_t LedTaskSpi::start(void)
{
    char name[16] = {0};
    snprintf(name, 16, "LED%d", this->led_number);
    xTaskCreatePinnedToCore(
        LedTaskSpi::svTaskCodeLed, // Function to implement the task
        name,                   // Name of the task
        5000,                   // Stack size in words
        this,                   // Task input parameter
        10,                     // Priority
        &this->task_handle,     // Task handle.
        1);                     // Core 1

    return ESP_OK;
}

esp_err_t LedTaskSpi::resume(void)
{
    vTaskResume(this->task_handle);
    return ESP_OK;
}

esp_err_t LedTaskSpi::suspend(void)
{
    vTaskSuspend(this->task_handle);
    return ESP_OK;
}

esp_err_t LedTaskSpi::set_state(const char *new_state, int charge_percent)
{
    #define MAP_TO_ENUM(x) if (strcmp(new_state, #x) == 0) \
        {\
            state_info.state = e_station_##x; \
            goto mapped; \
        }
    
    led_state_info_t state_info;
    state_info.state = e_station_status_unknown;

    MAP_TO_ENUM(available);
    MAP_TO_ENUM(waiting_for_power);
    MAP_TO_ENUM(charging);
    MAP_TO_ENUM(charging_pulse);
    MAP_TO_ENUM(charging_complete);
    MAP_TO_ENUM(out_of_service);
    MAP_TO_ENUM(disable);
    MAP_TO_ENUM(booting_up);
    MAP_TO_ENUM(offline);
    MAP_TO_ENUM(reserved);
    MAP_TO_ENUM(test_charging);
    
    if (state_info.state == e_station_status_unknown) {
        ESP_LOGE(TAG, "%d: Unknown state %s", this->led_number, new_state);
        return ESP_FAIL;
    }

mapped:
    state_info.charge_percent = charge_percent;
    xQueueSend(this->state_update_queue, &state_info, portMAX_DELAY);
    return ESP_OK;
}