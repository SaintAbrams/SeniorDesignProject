/*
 * PROJECT: Marine Crane AI - RFID Auth Node
 * HARDWARE: ESP32-C3 Mini
 * SENSORS: RC522 RFID Module (SPI)
 */

#include <esp_now.h>
#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>

// ESP32-C3 SPI Pins
#define RST_PIN   3     
#define SS_PIN    7     
MFRC522 mfrc522(SS_PIN, RST_PIN);  

// TODO: Replace with the UID of your personal Master Tag
String AUTHORIZED_UID = "A1 B2 C3 D4"; 

// TODO: Replace with Hub MAC Address
uint8_t hubAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 

bool system_unlocked = false;
unsigned long last_transmit_time = 0;

typedef struct struct_message {
    uint8_t node_type;    
    float value_1;        
    bool status_flag;     
} struct_message;
struct_message myData;

void setup() {
    Serial.begin(115200);
    SPI.begin(6, 4, 5, 7); // SCK, MISO, MOSI, SS for C3 hardware SPI
    mfrc522.PCD_Init(); 

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) return;
    
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, hubAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);

    myData.node_type = 4; // ID: RFID Station
    Serial.println("RFID Node Online. Waiting for card tap...");
}

void loop() {
    // Read Card if present
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        String tagUID = "";
        for (byte i = 0; i < mfrc522.uid.size; i++) {
            tagUID += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
            tagUID += String(mfrc522.uid.uidByte[i], HEX);
        }
        tagUID.trim();
        tagUID.toUpperCase();
        
        Serial.println("Card Detected: " + tagUID);

        if (tagUID == AUTHORIZED_UID) {
            system_unlocked = !system_unlocked; // Toggle lock state on tap
            Serial.println(system_unlocked ? "SYSTEM UNLOCKED" : "SYSTEM LOCKED");
            delay(1000); // Debounce to prevent rapid toggling
        }
    }

    // Ping the Hub once a second with the auth status
    if (millis() - last_transmit_time >= 1000) {
        myData.value_1 = 0; // Unused for RFID
        myData.status_flag = system_unlocked;
        
        esp_now_send(hubAddress, (uint8_t *) &myData, sizeof(myData));
        last_transmit_time = millis();
    }
}
