#pragma once

#include "Singleton.h"

#include <functional>

#include "esp_partition.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_ota_ops.h"

#define UPDATE_SIZE_UNKNOWN 0xFFFFFFFF
#define ENCRYPTED_BLOCK_SIZE 16

class FwUpdater : public Singleton<FwUpdater>
{
public:
    FwUpdater(token) {};
    ~FwUpdater(void) = default;

public:
    typedef std::function<void(uint8_t)> fn_progress_callback_t;

    inline FwUpdater &register_progress_callback(fn_progress_callback_t fn) { progress_callback = fn; return *this; } 

    //****************************************************************************************
    /**
     * @brief  Begin the update process
     *
     * Call this to check the space needed for the update
     *
     * @param  size: size of the update
     *
     * @retval ESP_OK if the write was successful, ESP_FAIL if not
     */
    esp_err_t begin(uint32_t size = UPDATE_SIZE_UNKNOWN);

    //****************************************************************************************
    /**
     * @brief  Write data to the update
     *
     * Writes a buffer to the flash and increments the address, returns the amount written
     *
     * @param  data: pointer to the data to write
     * @param  len: length of the data to write
     *
     * @retval ESP_OK if the write was successful, ESP_FAIL if not
     */
    esp_err_t write(uint8_t *data, size_t len);

    //****************************************************************************************
    /**
     * @brief  End the update process
     *
     * If all bytes are written this call will write the config to eboot and return true
     * If there is already an update running but is not finished and !evenIfRemaining
     * or there is an error this will clear everything and return false
     * the last error is available through getError()
     * evenIfRemaining is helpfull when you update without knowing the final size first
     *
     * @param  evenIfRemaining: if true, the update will be ended even if there are remaining bytes
     *
     * @retval ESP_OK if the write was successful, ESP_FAIL if not
     */
    esp_err_t end(bool evenIfRemaining = false);

    //****************************************************************************************
    /**
     * @brief  Abort the update process
     *
     * Aborts the running update
     *
     * @retval none
     */
    void abort();

private:
    const esp_partition_t * update_partition = nullptr;
    uint32_t update_size = 0;
    uint32_t bytes_written = 0;
    esp_ota_handle_t update_handle = 0;
    fn_progress_callback_t progress_callback = nullptr;

    // String _target_md5;
    // MD5Builder _md5;
};
