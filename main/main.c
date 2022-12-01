/**
 * @file main.c
 * 
 * @brief Error Code decoder project for ESP32 using FreeRTOS and ESP IDF
 * 
 * @par Program contains two main threads:
 *      1. A comms handling tasks which handles incoming JSON payloads.
 *      2. An Error Code handling task which decodes the error code and
 *         blinks an LED
 *      
 *      These tasks are run as seperate threads pinned to seperate cores.
 *      
 *      This application uses the esp_wifi API and MQTT message protocol
 *      to recieve data through the eclipse project online broker.
 * 
 *      3rd party applications used:
 *      jsmn.h/.c for JSON handling
 *      FreeRTOS for multi-threading
 *      ESP_IDF for HAL and driver level APIs
 *      
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "mqtt_client.h"
#include "wifi.h"
#include "mqtt.h"
#include "nvs_flash.h"
#include "jsmn.h"

/* Defines */

/**
 * @brief hardware GPIO pin number
*/
#define BLINK_GPIO 5

/**
 * @brief max number of JSON tokens handled by application
*/
#define JSMN_TOKENS 128

/**
 * @brief Max valid Error Code
*/
#define ERROR_CODE_MAX ((uint8_t)0xF)

/* Task Handles */

/**
 * @brief blink Task Handler
*/
TaskHandle_t blinkTaskHandle = NULL;

/**
 * @brief Test Task Handler
*/
TaskHandle_t testTaskHandle = NULL;

/**
 * @brief Comms Task Handler
*/
TaskHandle_t commsTaskHandle = NULL;

/* Global Variables */

/**
 * @brief stores the error code recieved by the comms Task [write only]
 *        to be used by the blink task [read only]
*/
static uint8_t error_code = 0;

/**
 * @brief: MQTT broker url location (change if necessary)
*/
const char * mqttBrokerUrl = "mqtt://mqtt.eclipseprojects.io";

/**
 * @brief JSON payload handler to pass messages from 
 *        MQTT event loop to comms Task
*/
mqtt_MessageHandle_t payloadHandle;

/* Function Prototypes */
/**
 * @brief updates error code and makes visible to 
 *        all tasks.
 * @param[in] value a 4-bit unsigned error code
 * 
 * @return None
*/
static void update_errorCode(uint8_t value);

/**
 * @brief Get the 4-bit led blink sequence and run routine
 *        based on the binary value
 * 
 * @param[in] errorCodeVal 4-bit unsigned integer value
 * 
 * @return None  
*/
static void decode_led(uint8_t errorCodeVal);

/**
 * @brief LED blink routine for a 0 value
*/
static void blink_low(void);

/**
 * @brief LED blink routine for a 1 value
*/
static void blink_high(void);

/**
 * @brief Initialise GPIO pin for digital output
 * to control LED
*/
static void configure_led(void);

/**
 * @brief: simple key value pair comparison from 
 *         https://github.com/zserge/jsmn/blob/master/example/simple.c
 *        
 * @param[in] json input json string
 * @param[in] tok  jsmn token
 * @param[in] s    string value to be compared
 * 
 * @return 0 if true else 1
*/
static int jsoneq(const char *json, jsmntok_t *tok, const char *s);

/* Task Function Prototypes*/
/**
 * @brief: test the blink error code parsing function
 * 
 * @note: sends an error code every 2 seconds.
 *        Error Code starts at 0 and increments until 
 *        16, then wraps around to 0. Only numbers > 0
 *        will trigger the blink routine.
 */
void Blink_Task(void* args);

/**
 * @brief Comms Task Handler
 * 
 * @param[in] args function arguments
*/
void Comms_Task (void* args);

/* LED Thread Functions */
static void blink_high(void)
{
    gpio_set_level(BLINK_GPIO, 1);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    gpio_set_level(BLINK_GPIO, 0);
}

static void blink_low(void)
{
    uint8_t state = 0;
    for (uint8_t i = 0; i < 4; i ++)
    {
        gpio_set_level(BLINK_GPIO, state);
        state = !state;
        vTaskDelay(250/portTICK_PERIOD_MS); // 4 Hz
    }
    gpio_set_level(BLINK_GPIO, 0);
}

static void decode_led(uint8_t errorCodeVal)
{
   uint8_t led_code[4] = {(errorCodeVal & 0b1000) >> 3,
                          (errorCodeVal & 0b100) >> 2,
                          (errorCodeVal & 0b10) >> 1,
                           errorCodeVal & 0b1,                          
                          };
    
    for(uint8_t i = 0; i < 4; i++)
    {
        if (led_code[i]) blink_high();
        else blink_low();
    }

}

static void configure_led(void)
{
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}


/* LED Thread Task */
void Blink_Task(void* args)
{
    printf("Blink Task Running...\n");
    (void)args; //free memory
    configure_led();

    while(1)
    {
        if(error_code)
        {
            printf("Error Code: %d\n", error_code);
            decode_led(error_code);
        }


        vTaskDelay(200/portTICK_RATE_MS);    
    }
}



#ifdef ENABLE_TEST

/* Test Thread Task */
void Test_Task(void* args)
{
    (void)args; //free memory
    printf("Test Task Running\n");
    static uint8_t errorCode = 0;
    while(1)
    {
        if (!error_code)
        {
            //TODO: replace with send_error_Code and make threadsafe
            printf("Sending Error Code %d\n", errorCode);
            error_code = errorCode;

            if (errorCode > 16)
            {
                errorCode = 0;
            }
            errorCode++;
        }
        
        vTaskDelay(1000/portTICK_RATE_MS);
    }
}
#endif

/* Commms Service Module Functions */

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) 
{
  if ((int)strlen(s) == tok->end - tok->start && strncmp(json + tok->start, s, tok->end - tok->start) == 0) 
    {
        return 0;
    }
  return -1;
}

void Comms_ProcessPayload(char * jsonString, uint32_t len)
{
    jsmn_parser parser;
    jsmntok_t tokens[JSMN_TOKENS];

    jsmn_init(&parser);
    memset(tokens, 0x00, sizeof(jsmntok_t)*JSMN_TOKENS);

    int8_t r = jsmn_parse(&parser,
                          jsonString,
                          len,
                          tokens,
                          JSMN_TOKENS);

    /* Invalid string recieved: exit function*/
    if (r < 0)
    {
        return;
    }
    
    for(uint8_t i = 0; i < (uint8_t)r-1; i++)
    {
        if(jsoneq(jsonString, &tokens[i], "error") == 0)
        {
            uint32_t token_len = tokens[i+1].end - tokens[i+1].start;
            char val[token_len];
            sprintf(val, "%.*s", token_len, jsonString+ tokens[i+1].start);
            update_errorCode(atoi((char*)val));
        }else if((jsoneq(jsonString, &tokens[i], "ignore") == 0)||
                 (jsoneq(jsonString, &tokens[i], "count") == 0))
        {
            uint32_t token_len = tokens[i+1].end - tokens[i+1].start;
            char val[token_len];
            sprintf(val, "%.*s", token_len, jsonString+ tokens[i+1].start);
            printf("%s\n", (char *)val);
        }
    }
}

static void update_errorCode(uint8_t value)
{
    if (value > ERROR_CODE_MAX)
    {
        printf("Warning: Error Code %d invalid. Error Codes must be 4 bits long.", value);
        return;
    }

    error_code = value & 0xF;
}

/* Comms Service Task */
void Comms_Task (void* args)
{
    memset(&payloadHandle, 0, sizeof(payloadHandle));

    nvs_flash_init();

    printf("Initialising WIFI Communications.\n");
    wifi_init_connection();
    vTaskDelay(2000/portTICK_PERIOD_MS);

    printf("Initialising MQTT Message Service.\n");
    mqtt_init(mqttBrokerUrl, &payloadHandle);

    while(1)
    { 
        if(payloadHandle.data_len > 0)
        {
            Comms_ProcessPayload(payloadHandle.rxBuffer, payloadHandle.data_len);
            memset(&payloadHandle, 0, sizeof(payloadHandle));
        }
        vTaskDelay(200/portTICK_PERIOD_MS);
    }
}

/* Main Function */
void app_main(void)
{
    // Create Blink Module Task
    printf("creating Blink Task\n");   
    xTaskCreate(Blink_Task, "Blink Task", 4096, NULL, 10, &blinkTaskHandle);

    // Create comms Task in seperate core
    printf("creating Communication Task\n");
    xTaskCreatePinnedToCore(Comms_Task, "Comms Task", 8192, NULL, 10, &commsTaskHandle, 1);
 
 #ifdef ENABLE_TEST   
    // Create Test Task  
    printf("creating Test Task\n");
    xTaskCreatePinnedToCore(Test_Task, "Test Task", 4096, NULL, 10, &testTaskHandle, 1);
#endif
}

