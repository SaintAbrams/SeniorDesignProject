/*
 * PROJECT: Marine Crane AI - Weather Node (REVISION 2)
 * HARDWARE: ESP32-C3 Mini
 * SENSORS: BME280 (I2C). Anemometer removed pending hardware.
 */

#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// TODO: Replace with Hub MAC Address
uint8_t hubAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 

Adafruit_BME280 bme; 
unsigned long last_transmit_time = 0;
const int TRANSMIT_INTERVAL = 2000; 

typedef struct struct_message {
    uint8_t node_type;    
    float value_1;        
    bool status_flag;     
} struct_message;

struct_message myData;

void setup() {
    Serial.begin(115200);

    if (!bme.begin(0x76)) { 
        Serial.println("BME280 not found");
    }

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) return;
    
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, hubAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);

    myData.node_type = 1; // ID: Weather Station
}

void loop() {
    if (millis() - last_transmit_time >= TRANSMIT_INTERVAL) {
        
        // Read Temp/Humidity for local serial debugging if desired
        float temp = bme.readTemperature();
        
        // Since the anemometer is missing, force safe values for the Hub
        myData.value_1 = 0.0; // Wind speed = 0
        myData.status_flag = false; // weather_red = false (Safe)
        
        esp_now_send(hubAddress, (uint8_t *) &myData, sizeof(myData));
        Serial.printf("Weather Node Active. Sending safe defaults. Temp: %.1f C\n", temp);

        last_transmit_time = millis();
    }
}
