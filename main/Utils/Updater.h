#pragma once

#include "Singleton.h"

#include <functional>
#include "esp_partition.h"

#define UPDATE_ERROR_OK (0)
#define UPDATE_ERROR_WRITE (1)
#define UPDATE_ERROR_ERASE (2)
#define UPDATE_ERROR_READ (3)
#define UPDATE_ERROR_SPACE (4)
#define UPDATE_ERROR_SIZE (5)
#define UPDATE_ERROR_STREAM (6)
#define UPDATE_ERROR_MD5 (7)
#define UPDATE_ERROR_MAGIC_BYTE (8)
#define UPDATE_ERROR_ACTIVATE (9)
#define UPDATE_ERROR_NO_PARTITION (10)
#define UPDATE_ERROR_BAD_ARGUMENT (11)
#define UPDATE_ERROR_ABORT (12)

#define UPDATE_SIZE_UNKNOWN 0xFFFFFFFF

#define U_FLASH 0
#define U_SPIFFS 100
#define U_AUTH 200

#define ENCRYPTED_BLOCK_SIZE 16

class FwUpdater : public Singleton<FwUpdater>
{
public:
    FwUpdater(token);
    ~FwUpdater(void) = default;

public:
    typedef std::function<void(size_t, size_t)> fn_progress_callback_t;

    FwUpdater &onProgress(fn_progress_callback_t fn);

    //****************************************************************************************
    /**
     * @brief  Begin the update process
     *
     * Call this to check the space needed for the update
     *
     * @param  size: size of the update
     * @param  command: U_FLASH, U_SPIFFS, U_AUTH
     * @param  ledPin: pin to control the led
     * @param  ledOn: led on state
     * @param  label: label of the partition to update
     *
     * @retval true if the update can be started, false if not enough space
     */
    bool begin(size_t size = UPDATE_SIZE_UNKNOWN, int command = U_FLASH, const char *label = NULL);

    //****************************************************************************************
    /**
     * @brief  Write data to the update
     *
     * Writes a buffer to the flash and increments the address, returns the amount written
     *
     * @param  size: size of the update
     * @param  command: U_FLASH, U_SPIFFS, U_AUTH
     * @param  ledPin: pin to control the led
     * @param  ledOn: led on state
     * @param  label: label of the partition to update
     *
     * @retval the amount written
     */
    size_t write(uint8_t *data, size_t len);

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
     * @retval true if the update was successful, false if not
     */
    bool end(bool evenIfRemaining = false);

    //****************************************************************************************
    /**
     * @brief  Abort the update process
     *
     * Aborts the running update
     *
     * @retval none
     */
    void abort();

    const char *errorString();

    // /*
    //   sets the expected MD5 for the firmware (hexString)
    // */
    // bool setMD5(const char *expected_md5);

    // /*
    //   returns the MD5 String of the successfully ended firmware
    // */
    // String md5String(void) { return _md5.toString(); }

    // /*
    //   populated the result with the md5 bytes of the successfully ended firmware
    // */
    // void md5(uint8_t *result) { return _md5.getBytes(result); }

    // Helpers
    inline uint8_t getError() { return _error; }
    inline void clearError() { _error = UPDATE_ERROR_OK; }
    inline bool hasError() { return _error != UPDATE_ERROR_OK; }
    inline bool isRunning() { return _size > 0; }
    inline bool isFinished() { return _progress == _size; }
    inline size_t size() { return _size; }
    inline size_t progress() { return _progress; }
    inline size_t remaining() { return _size - _progress; }

    /*
      check if there is a firmware on the other OTA partition that you can bootinto
    */
    //****************************************************************************************
    /**
     * @brief  Check if there is a firmware on the other OTA partition that you can boot into
     *
     * @retval true if there is a firmware on the other OTA partition that you can boot into, false if not
     */
    bool canRollBack(void);

    //****************************************************************************************
    /**
     * @brief  Set the other OTA partition as bootable (reboot to enable)
     *
     * @retval true if the other OTA partition was set as bootable, false if not
     */
    bool rollBack();

private:
    void _reset();
    void _abort(uint8_t err);
    bool _writeBuffer();
    bool _verifyHeader(uint8_t data);
    bool _verifyEnd();
    bool _enablePartition(const esp_partition_t *partition);

    uint8_t _error;
    uint8_t *_buffer;
    uint8_t *_skipBuffer;
    size_t _bufferLen;
    size_t _size;
    fn_progress_callback_t _progress_callback;
    uint32_t _progress;
    uint32_t _paroffset;
    uint32_t _command;
    const esp_partition_t *_partition;

    // String _target_md5;
    // MD5Builder _md5;
};
