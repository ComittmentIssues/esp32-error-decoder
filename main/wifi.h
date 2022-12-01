/**
 * @file wifi.h
 * Wifi Module for ESP32
 */

#ifndef _WIFI_H
#define _WIFI_H

/* Includes */
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_wifi_types.h"

/**
 * @brief SSID of desired network
*/
#define WIFI_SSID       "XXXXX"

/**
 * @brief network password
 */
#define WIFI_PASSWORD   "XXXXX"

/**
 * @brief Setup ESP32 wifi module in station mode
 * @param None
 * @return None
*/
void wifi_init_connection(void);


#endif /* _WIfI_H */