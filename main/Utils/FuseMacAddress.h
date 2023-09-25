//*****************************************************************************
/**
 * @file FuseMacAddress.h
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief Utility functions to get the mac address from the ESP32 fuse module.
 * @version 0.1
 * @date 2023-09-04
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//*****************************************************************************
#pragma once

#include "esp_err.h"

esp_err_t get_fuse_mac_address(uint8_t* mac_address);
esp_err_t get_fuse_mac_address_string(char* mac_address_string);
