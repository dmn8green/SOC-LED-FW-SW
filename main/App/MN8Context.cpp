#include "MN8Context.h"
#include "Utils/FuseMacAddress.h"

#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_mac.h"
#include "esp_vfs_fat.h"
#include "esp_check.h"

esp_err_t MN8Context::setup(void) {
    // get mac
    get_fuse_mac_address_string(this->mac_address);
    return ESP_OK;
}