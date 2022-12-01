/**
 * @file mqtt.h
*/
#ifndef _MQTT_H
#define _MQTT_H

/* Includes */
#include "mqtt_client.h"
#include <stdio.h>
#include <string.h>

/* Private Typedef */

/**
 * @brief: MQTT message struct to pass incoming data between threads
*/
typedef struct mqtt_MessageHandleStruct {
    char rxBuffer[1024];    /**< data buffer*/
    uint32_t data_len;      /**< data length*/
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

