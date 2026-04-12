/*
 * PROJECT: Marine Crane AI - Load Cell Node
 * HARDWARE: ESP32-C3 Mini
 * SENSORS: HX711 Amplifier + Strain Gauge
 */

#include <esp_now.h>
#include <WiFi.h>
#include <HX711.h>

// HX711 Circuit Wiring
const int LOADCELL_DOUT_PIN = 4;
const int LOADCELL_SCK_PIN = 5;

// Safety & Calibration
const float OVERLOAD_THRESHOLD_LBS = 800.0; // Adjust to your physical limits
const float CALIBRATION_FACTOR = 2280.f;    // Find this via trial with known weight

// TODO: Replace with Hub MAC Address
uint8_t hubAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 

HX711 scale;
unsigned long last_transmit_time = 0;

typedef struct struct_message {
    uint8_t node_type;    
    float value_1;        
    bool status_flag;     
} struct_message;
struct_message myData;

void setup() {
    Serial.begin(115200);

    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    scale.set_scale(CALIBRATION_FACTOR);
    scale.tare(); // Reset scale to 0 on boot

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) return;
    
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, hubAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);

    // CRITICAL: Set to 2 for Alpha, 3 for Beta
    myData.node_type = 3; 
}

void loop() {
    if (millis() - last_transmit_time >= 500) { // Transmit 2x a second
        if (scale.is_ready()) {
            float current_weight = scale.get_units(3); // Average of 3 readings
            
            bool is_overloaded = (current_weight >= OVERLOAD_THRESHOLD_LBS);

            myData.value_1 = current_weight;
            myData.status_flag = is_overloaded;
            
            esp_now_send(hubAddress, (uint8_t *) &myData, sizeof(myData));
            Serial.printf("Weight: %.1f lbs | Overload: %s\n", current_weight, is_overloaded ? "YES" : "NO");
        }
        last_transmit_time = millis();
    }
}
