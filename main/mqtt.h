/**
 * MQTT module
 * Author: Jamie Jacobson
*/

#ifndef _MQTT_H
#define _MQTT_H

/* Includes */
#include "mqtt_client.h"
#include <stdio.h>
#include <string.h>

/* Private Typedef */
typedef struct mqtt_MessageHandleStruct {
    char rxBuffer[1024];
    uint32_t data_len;
} mqtt_MessageHandle_t;

/* Public Functions */
/**
 * @brief initialises and configures ESP32 for Wifi in station mode 
 * @param[in] url url of mqtt broker
 * @param[in] buffer Buffer to store message
 * @return none
*/
void mqtt_init( const char* url, mqtt_MessageHandle_t* messageHandle);

#endif // !_MQTT_H

