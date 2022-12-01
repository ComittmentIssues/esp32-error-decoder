
#include <stdio.h>
#include "wifi.h"

/* Function Prototypes */
static void wifi_event_handler(void* arg, 
                               esp_event_base_t event_base, 
                               int32_t event_id, 
                               void* event_data);

/* Module Functions */
void wifi_init_connection(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t wifiInitConfig = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifiInitConfig);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);

    wifi_config_t wifiConfig= { 
        .sta = { .ssid = WIFI_SSID, .password = WIFI_PASSWORD}
        };
    
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifiConfig);

    esp_wifi_start();

    esp_wifi_connect();

}

/**
 * Handler keeps track of Wifi state to monitor
 * whether wifi has been connected or disconnected
*/
static void wifi_event_handler(void* arg, 
                               esp_event_base_t event_base, 
                               int32_t event_id, 
                               void* event_data)
{
    switch(event_id)
    {
        case WIFI_EVENT_STA_START:
        {
            uint8_t macAddress[6] = {0};
            esp_wifi_get_mac(WIFI_IF_STA, macAddress);
            printf("Wifi Started. MAC: %x:%x:%x:%x:%x:%x \n", 
                   macAddress[0],
                   macAddress[1],
                   macAddress[2],
                   macAddress[3],
                   macAddress[4],
                   macAddress[5]);
            printf("Connecting...\n");
            break;
        }
        case WIFI_EVENT_STA_CONNECTED:
            printf("Connected!\n");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            printf("Disconnected!\n");
            esp_wifi_connect();
            break;
        
        case IP_EVENT_STA_GOT_IP:
            printf("got IP\n");
            break;
        default:
            printf("Unhandled event_id %d\n", event_id);
            break;
    }
}