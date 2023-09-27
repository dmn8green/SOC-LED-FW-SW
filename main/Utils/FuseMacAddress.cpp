//*****************************************************************************
/**
 * @file FuseMacAddress.cpp   
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief Utility functions to get the mac address from the ESP32 fuse module.
 * @version 0.1
 * @date 2023-09-04
 * 
 * @copyright Copyright MN8 (c) 2023
 * 
 * This code returns the eFuse mac address, the same mac address that is
 * returned when using `esptools.py` to read the chip id.
 * 
 * This MAC address is unique.  It is not the same as the MAC address that
 * is returned by the `esp_wifi_get_mac` function for each interfaces.
 * 
 */
//*****************************************************************************
#include "FuseMacAddress.h"

#include "esp_err.h"
#include "esp_rom_efuse.h"
#include "esp_mac.h"
#include "esp_efuse.h"
#include "esp_efuse_table.h"

//*****************************************************************************
/**
 * @brief Get the fuse mac address object
 * 
 * @param mac_address Must be a buffer with at least 6 bytes.
 * @return esp_err_t  ESP_OK if successful.
 */
esp_err_t get_fuse_mac_address(uint8_t* mac_address) {
    uint64_t _chipmacid = 0LL;
    esp_efuse_mac_get_default((uint8_t*) (&_chipmacid));
    ESP_LOGI(__func__, "ESP32 Chip ID = %04X",(uint16_t)(_chipmacid>>32));
    ESP_LOGI(__func__, "ESP32 Chip ID = %04X",(uint16_t)(_chipmacid>>16));
    ESP_LOGI(__func__, "ESP32 Chip ID = %04X",(uint16_t)(_chipmacid));

    mac_address[0] = (_chipmacid >> 40) & 0xFF;
    mac_address[1] = (_chipmacid >> 32) & 0xFF;
    mac_address[2] = (_chipmacid >> 24) & 0xFF;
    mac_address[3] = (_chipmacid >> 16) & 0xFF;
    mac_address[4] = (_chipmacid >> 8) & 0xFF;
    mac_address[5] = (_chipmacid >> 0) & 0xFF;
    return ESP_OK;
}

//*****************************************************************************
/**
 * @brief Get the fuse mac address string object
 * 
 * @param mac_address_string        to hold the mac address string (no colons)
 *                                  must be at least 13 bytes long.
 * @return esp_err_t                ESP_OK if successful.
 */
esp_err_t get_fuse_mac_address_string(char* mac_address_string) {
    uint8_t mac_address[6];
    esp_err_t err = get_fuse_mac_address(mac_address);
    if (err != ESP_OK) {
        return err;
    }

    sprintf(mac_address_string, "%02X%02X%02X%02X%02X%02X",
        mac_address[0], mac_address[1], mac_address[2],
        mac_address[3], mac_address[4], mac_address[5]);

    return ESP_OK;
}
