/*
 * PROJECT: Marine Crane AI - Proximity Node
 * HARDWARE: ESP32-C3 Mini
 * SENSORS: Waterproof Ultrasonic (JSN-SR04T)
 */

#include <esp_now.h>
#include <WiFi.h>

const int TRIG_PIN = 4;
const int ECHO_PIN = 5;

// Safety Limit (Distance to trigger halt)
const float COLLISION_THRESHOLD_INCHES = 36.0; 

// TODO: Replace with Hub MAC Address
uint8_t hubAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 

unsigned long last_transmit_time = 0;

typedef struct struct_message {
    uint8_t node_type;    
    float value_1;        
    bool status_flag;     
} struct_message;
struct_message myData;

void setup() {
    Serial.begin(115200);
    
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) return;
    
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, hubAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);

    myData.node_type = 5; // ID: Proximity Station
}

void loop() {
    if (millis() - last_transmit_time >= 100) { // Fast update rate for collision
        
        digitalWrite(TRIG_PIN, LOW);
        delayMicroseconds(2);
        digitalWrite(TRIG_PIN, HIGH);
        delayMicroseconds(10);
        digitalWrite(TRIG_PIN, LOW);
        
        long duration = pulseIn(ECHO_PIN, HIGH);
        // Convert to inches (speed of sound)
        float distance_inches = (duration * 0.0133) / 2; 

        bool collision_imminent = false;
        // Ignore wild 0 readings common with ultrasonic noise
        if (distance_inches > 0 && distance_inches <= COLLISION_THRESHOLD_INCHES) {
            collision_imminent = true;
        }

        myData.value_1 = distance_inches;
        myData.status_flag = collision_imminent;
        
        esp_now_send(hubAddress, (uint8_t *) &myData, sizeof(myData));
        
        last_transmit_time = millis();
    }
}
