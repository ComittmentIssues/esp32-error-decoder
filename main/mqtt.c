/**
 * @file mqtt.c
*/

/* Includes */
#include "mqtt.h"
#include "assert.h"

/* Private Defines */

/**
 * @brief Max length of a JSON string in number of charachters
*/
#define JSON_LENGTH 2048

/* Private Variables */

/**
 * @brief mqtt topic esp subscribes to in order to get JSON payload.
*/
const char * topic = "blink";

/**
 * @brief holds mqtt message handle instance to transfer data between threads.
 */
static mqtt_MessageHandle_t* CommsSlot;

/* Private Function Prototypes */
/**
 * @brief MQTT event handler loop
 * @param[in] handler_args Loop arguments
 * @param[in] base event base
 * @param[in] event_id event id
 * @param[in] event_data event data
 * 
 * @return None
*/
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

/* Module Functions*/

void mqtt_init( const char* url, mqtt_MessageHandle_t* messageHandle)
{
    assert(messageHandle != NULL);

    esp_mqtt_client_config_t mqttConfig = {
        .uri = url,
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqttConfig);
    
    CommsSlot = messageHandle;
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{  
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch (event_id)
    {
        case MQTT_EVENT_CONNECTED:
            /* JSON payload sent under topic: blink*/
            esp_mqtt_client_subscribe(client, topic, 2); 
            break;
        case MQTT_EVENT_DISCONNECTED:
            /* code */
            break;
        case MQTT_EVENT_SUBSCRIBED:
           
            break;
        case MQTT_EVENT_DATA:
        {
            memcpy(CommsSlot->rxBuffer, event->data, event->data_len);
            CommsSlot->data_len = event->data_len;
            break;
        }
        case MQTT_EVENT_ERROR:
            /* code */
            break;
        default:
            break;
    }
}