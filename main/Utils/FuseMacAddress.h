#pragma once

#include "esp_err.h"

esp_err_t get_fuse_mac_address(uint8_t* mac_address);
esp_err_t get_fuse_mac_address_string(char* mac_address_string);
