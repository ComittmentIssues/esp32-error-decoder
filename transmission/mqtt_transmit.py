"""
mqtt_transmit.py

Program used to test the ESP32 error decoding module. continuosly publishes mqtt
messages under the topic "blink". Messages are JSON strings of variable payload that
contain the following key value pair: "error": x which is used to set the error code
on the ESP32.
"""

import json
import time
import paho.mqtt.client as mqtt
from random import randint

mqtt_broker = "mqtt.eclipseprojects.io"
topic = "blink"
packet_no = 0
esp_dict = {"error":0, "ignore": "randstring","count": packet_no}

def main():
    client = mqtt.Client("Error Module")
    client.connect(mqtt_broker)

    while True:
        chance = randint(0, 100)
        if(chance < 25):
            esp_dict["error"] = randint(0,15)
        else: 
            esp_dict["error"] = 0
        json_str = json.dumps(esp_dict)
        print(json_str)
        client.publish(topic, json_str.replace(" ",""))

        esp_dict["count"] += 1
        time.sleep(0.5)


if __name__ == '__main__':
    main()
