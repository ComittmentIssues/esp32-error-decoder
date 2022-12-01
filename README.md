# Error Decoder

(See the README.md file in the upper level 'examples' directory for more information about examples.)

This is a simple ESP32 project to blink an LED based off of an unsigned 4 bit error code.
The project is built using FreeRTOS and the ESP-IDF tool chain. Data is sent to the ESP32 using MQTT over WIFI. The project also contains a python script under the ***transmission*** folder to verify the application by sending JSON strings via the mosquitto broker.

For more information follow the [docs page](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html#start-a-new-project)

## Hardware Setup.

The application uses Pin 5 as the digital IO pin to toggle an LED. This can be modified by changing the value of 
```
#define BLINK_GPIO x
```

## ESP IDF
The project was built using the ESP-IDF toolchain in VS Code.

ESP-IDF projects are built using CMake. The project build configuration is contained in `CMakeLists.txt`
files that provide set of directives and instructions describing the project's source files and targets
(executable, library, or both). 

Below is short explanation of remaining files in the project folder.

```
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   └── main.c
└── README.md                  This is the file you are currently reading
```
Additionally, the sample project contains Makefile and component.mk files, used for the legacy Make based build system. 
They are not used or needed when building with CMake and idf.py.

## MQTT
The device recieves data using the MQTT protocol (see [MQTT foundation](https://mqtt.org/) for more information). In its current configuration, the device uses the Ecipse Project online MQTT broker to exchange information. The ESP32 subscribes to the topic "blink", which is used to deliver a JSON payload. The device can interpret a json string of variable length. However, the key "error" must be transmitted with an unsigned 4-bit integer value as follows:

```
{"error":4}
```

The mosquitto MQTT broker is used to faciliate communications between the PC and the ESP32. If you wish to use another broker, you will need to update the broker URL in both transmission/mqtt_transmit.py, and main/mqtt.c

mqtt_transmit.py: ```mqtt_broker = "mqtt.xxx.yy"```
mqtt.c: ```const char * mqttBrokerUrl = "mqtt://mqtt.eclipseprojects.io";```

## WIFI

The application configures the esp32 for station mode to connect to the internet. To change the network settings, The wifi ssid and password are stored in the source file wifi.c as follows.

```
#define WIFI_SSID       "SSID_NAME"
#define WIFI_PASSWORD   "Password"
```

These values need to be updated to the wifi details of the network you wish to use.
