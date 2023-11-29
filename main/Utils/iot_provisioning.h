//*****************************************************************************
/**
 * @file iot.h
 * @author pat laplante (plaplante@appliedlogix.com)
 * @brief iot helper methods, to provision/unprovision a device, definition
 * @version 0.1
 * @date 2023-10-18
 * 
 * @copyright Copyright MN8 (c) 2023
 */
//*****************************************************************************

#pragma once

//*****************************************************************************
/**
 * @brief Provision this device as an AWS iot thing
 * 
 * This function is registered as a REPL command. 
 * It takes the following arguments:
 *  - url: url to the provisioning server (optional)
 *  - username: username for the provisioning server
 *  - password: password for the provisioning server
 * 
 * The function will send a POST request to the provisioning server with the
 * following JSON payload:
 * {
 *  "thingId": "mac_address of this device"
 * }
 * 
 * The mac address is obtained from the esp-idf API esp_efuse_mac_get_default()
 * This is the same mac address returned when calling esptool.py chip_id
 * 
 * @param url
 * @param username
 * @param password
 * @return true if successful
 */
bool provision_device(const char* url, const char* username, const char* password);

//*****************************************************************************
/**
 * @brief Unprovision this device as an AWS iot thing
 * 
 * This function is registered as a REPL command. 
 * It takes the following arguments:
 *  - url: url to the provisioning server (optional)
 *  - username: username for the provisioning server
 *  - password: password for the provisioning server
 * 
 * The function will send a POST request to the unprovisioning server with the
 * following JSON payload:
 * {
 *  "thingId": "mac_address of this device"
 * }
 * 
 * The mac address is obtained from the esp-idf API esp_efuse_mac_get_default()
 * This is the same mac address returned when calling esptool.py chip_id
 * 
 * @param argc 
 * @param argv 
 * @return true if successful 
 */
bool unprovision_device(const char* url, const char* username, const char* password);
