#pragma once

/**
 * @file ethernet_init.h
 * @author Espressif
 * @brief 
 * @version 0.1
 * @date 2023-08-29
 * 
 * Code lifted from the esp ethernet example, with some adjustment.
 * Our use case will only ever have one ethernet port connected to
 * the ESP32, so we can simplify the code a bit.
 * 
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "esp_eth_driver.h"

/**
 * @brief Initialize Ethernet driver based on Espressif IoT Development Framework Configuration
 *
 * @param[out] eth_handle_out Ethernet driver handles
 * @return
 *          - ESP_OK on success
 *          - ESP_ERR_INVALID_ARG when passed invalid pointers
 *          - ESP_ERR_NO_MEM when there is no memory to allocate for Ethernet driver handles array
 *          - ESP_FAIL on any other failure
 */
esp_err_t eth_init(esp_eth_handle_t **eth_handles_out);