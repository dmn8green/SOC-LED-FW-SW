#include "Updater.h"

#include "spi_flash_mmap.h"
#include "spi_flash_mmap.h"
#include "esp_ota_ops.h"
#include "esp_image_format.h"
#include "esp_log.h"

#include <memory.h>
#include <esp_timer.h>

static const char *TAG = "Updater";

//****************************************************************************************
esp_err_t FwUpdater::begin(uint32_t size)
{
    esp_err_t err = 0;

    if (this->update_partition != nullptr) {
        ESP_LOGI(TAG, "Update already running");
        return false;
    }

    if (size == 0) {
        ESP_LOGE(TAG, "Size is 0 no updates possible");
        return false;
    }

    this->update_size = 0;
    this->update_handle = 0;
    this->bytes_written = 0;

    this->update_partition = esp_ota_get_next_update_partition(NULL);
    if (this->update_partition == nullptr) {
        ESP_LOGE(TAG, "Partition could not be found");
        return false;
    }
    ESP_LOGI(TAG,"Update firmware command");

    err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &this->update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        esp_ota_abort(this->update_handle);
    }

    if (size == UPDATE_SIZE_UNKNOWN) {
        size = this->update_partition->size;
    } else if (size > this->update_partition->size) {
        ESP_LOGE(TAG, "too large %lu > %lu", size, this->update_partition->size);
        return ESP_ERR_INVALID_SIZE;
    }
    this->update_size = size;
    ESP_LOGI(TAG, "Updater beginned, size: %lu handle %ld", size, this->update_handle);
    return ESP_OK;
}

//****************************************************************************************
esp_err_t FwUpdater::write(uint8_t *data, size_t len)
{
    esp_err_t ret = ESP_OK;
    uint8_t percentWritten = 0;
    uint32_t usecTimeCount = 0;
    uint32_t prev_usecTimeCount = 0;

    ESP_LOGI(TAG, "Byte written %lu, update size %lu", this->bytes_written, this->update_size);
    if (len > this->update_size - this->bytes_written) {
        ESP_LOGE(TAG, "too large %d > %lu", len, this->update_size - this->bytes_written);
        return ESP_ERR_INVALID_SIZE;
    }

    prev_usecTimeCount = (uint32_t)(esp_timer_get_time() & 0x00000000ffffffff);

    ESP_LOGI(TAG, "Writing %d bytes handle %ld", len, this->update_handle);
    ret = esp_ota_write( update_handle, data, len);
    if (ret != ESP_OK) {
        esp_ota_abort(update_handle);
        ESP_LOGE(TAG, "esp_ota_write failed");
        goto err;
    }

    this->bytes_written += len;

    // percentWritten = (this->bytes_written * 99) / this->update_size;
    // usecTimeCount = (uint32_t)(esp_timer_get_time() & 0x00000000ffffffff);
    // ESP_LOGI(TAG, "esp_ota_write time %lu percent written %u",
    //                  usecTimeCount - prev_usecTimeCount, percentWritten);
    // this->progress_callback(percentWritten);

err:
    return ret;
}

//****************************************************************************************
esp_err_t FwUpdater::end(bool evenIfRemaining)
{
    esp_err_t ret = ESP_OK;

    if (this->update_partition == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    if (this->bytes_written != this->update_size) {
        if (!evenIfRemaining) {
            return ESP_ERR_INVALID_SIZE;
        }
    }

    // End the update
    ret = esp_ota_end(this->update_handle);
    if (ret != ESP_OK) {
        if (ret == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        } else {
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(ret));
        }
        goto err;
    }
    ESP_LOGI(TAG, "esp_ota_end succeeded");

    // Set the partition as bootable
    ret = esp_ota_set_boot_partition(this->update_partition);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(ret));
        goto err;
    }
    ESP_LOGI(TAG, "esp_ota_set_boot_partition succeeded");

err:
    // Clear the update
    this->update_partition = nullptr;
    this->update_size = 0;
    this->update_handle = 0;
    this->bytes_written = 0;

    return ret;
}


//****************************************************************************************
void FwUpdater::abort()
{
    this->update_partition = nullptr;
    if (this->update_handle) {
        esp_ota_abort(this->update_handle);
    }
    this->update_size = 0;
    this->update_handle = 0;
    this->bytes_written = 0;
}

