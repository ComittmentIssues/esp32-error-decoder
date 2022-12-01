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
#define BLINK_GPIO 5
#define JSMN_TOKENS 128
#define ERROR_CODE_MAX ((uint8_t)0xF)

/* Typedefs */
typedef enum Comms_StatusEnum {
    COMMS_OK = 0,
    COMMS_ERROR
} Comms_Status_t;

/* Task Handles */
TaskHandle_t blinkTaskHandle = NULL;
TaskHandle_t testTaskHandle = NULL;
TaskHandle_t commsTaskHandle = NULL;

/* Global Variables */
static uint8_t error_code = 0;
const char * mqttBrokerUrl = "mqtt://mqtt.eclipseprojects.io";
mqtt_MessageHandle_t payloadHandle;

/* Function Prototypes */
void update_errorCode(uint8_t value);

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

/**
 * @brief: test the blink error code parsing function
 * 
 * @note: sends an error code every 2 seconds.
 *        Error Code starts at 0 and increments until 
 *        16, then wraps around to 0. Only numbers > 0
 *        will trigger the blink routine.
 */

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

/* Commms Service Module Functions */

/**
 * @brief: simple key value pair comparison from 
 *         https://github.com/zserge/jsmn/blob/master/example/simple.c
 * 
*/
static int jsoneq(const char *json, jsmntok_t *tok, const char *s) 
{
  if ((int)strlen(s) == tok->end - tok->start && strncmp(json + tok->start, s, tok->end - tok->start) == 0) 
    {
        return 0;
    }
  return -1;
}

Comms_Status_t Comms_ProcessPayload(jsmn_parser* parser, jsmntok_t* tokens, char * jsonString, uint32_t len)
{
    jsmn_init(parser);
    memset(tokens, 0x00, sizeof(jsmntok_t)*JSMN_TOKENS);

    int8_t r = jsmn_parse(parser,
                          jsonString,
                          len,
                          tokens,
                          JSMN_TOKENS);
    if (r < 0)
    {
        return COMMS_ERROR;
    }
    
    for(uint8_t i = 0; i < (uint8_t)r; i++)
    {
        if(jsoneq(jsonString, &tokens[i], "error") == 0)
        {
            uint32_t token_len = tokens[i+1].end - tokens[i+1].start;
            char val[token_len];
            sprintf(val, "%.*s", token_len, jsonString+ tokens[i+1].start);
            update_errorCode(atoi((char*)val));
        }
    }

    return 0;
}

void update_errorCode(uint8_t value)
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
    jsmn_parser parser;
    jsmntok_t tokens[JSMN_TOKENS];

    memset(&payloadHandle, 0, sizeof(payloadHandle));

    nvs_flash_init();

    printf("Initialising WIFI Communications.\n");
    wifi_init_connection();
    vTaskDelay(2000/portTICK_PERIOD_MS);

    printf("Initialising MQTT Message Service.");
    mqtt_init(mqttBrokerUrl, &payloadHandle);
    printf("Intiialisng JSMN Parser. \n");

    while(1)
    { 
        if(payloadHandle.data_len > 0)
        {
            Comms_ProcessPayload(&parser, tokens, payloadHandle.rxBuffer, payloadHandle.data_len);
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
    printf("create Communication Task\n");
    xTaskCreatePinnedToCore(Comms_Task, "Comms Task", 8192, NULL, 10, &commsTaskHandle, 1);
    
    // Create Test Task
    //printf("create Test Task\n");
    //xTaskCreatePinnedToCore(Test_Task, "Test Task", 4096, NULL, 10, &testTaskHandle, 1);
   
}

