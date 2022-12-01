/*
 * Wifi Module for ESP32
 */

#ifndef _WIFI_H
#define _WIFI_H

/* Includes */
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_wifi_types.h"

#define WIFI_SSID       "Optus_43801A"
#define WIFI_PASSWORD   "yukesskeetKAXzh"

/**
 * @brief Setup ESP32 wifi module in station mode
 * @param None
 * @return None
*/
void wifi_init_connection(void);


#endif /* _WIfI_H */