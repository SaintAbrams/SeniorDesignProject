/*
 * PROJECT: Marine Crane AI - RFID Oyster Cage Tracker
 * HARDWARE: ESP32-C3 Mini + RC522 RFID Module (SPI)
 * FUNCTION: Translates Tag UIDs to Cage IDs and sends to Hub
 */

#include <esp_now.h>
#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN   3     
#define SS_PIN    7     
MFRC522 mfrc522(SS_PIN, RST_PIN);  

// TODO: Replace with Hub MAC Address
uint8_t hubAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 

// --- The Oyster Cage Database ---
// Map your physical RFID tags to your farm's cage numbers here
int identifyCage(String uid) {
    if (uid == "A1 B2 C3 D4") return 101;
    if (uid == "DE AD BE EF") return 102;
    if (uid == "11 22 33 44") return 103;
    return 999; // 999 = Unknown/Unregistered Cage
}

int current_cage_id = 0; // 0 means no cage on hook
unsigned long last_scan_time = 0;
const unsigned long CAGE_TIMEOUT_MS = 30000; // Clear cage ID after 30 seconds

typedef struct struct_message {
    uint8_t node_type;    
    float value_1;        
    bool status_flag;     
} struct_message;
struct_message myData;

void setup() {
    Serial.begin(115200);
    SPI.begin(6, 4, 5, 7); 
    mfrc522.PCD_Init(); 

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) return;
    
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, hubAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);

    myData.node_type = 4; // ID: RFID Station
    Serial.println("Oyster Tracker Online. Waiting for cages...");
}

void loop() {
    // 1. Handle New Tag Scans
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        String tagUID = "";
        for (byte i = 0; i < mfrc522.uid.size; i++) {
            tagUID += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
            tagUID += String(mfrc522.uid.uidByte[i], HEX);
        }
        tagUID.trim();
        tagUID.toUpperCase();
        
        // Translate UID to Farm Cage Number
        current_cage_id = identifyCage(tagUID);
        last_scan_time = millis();
        
        Serial.printf("Cage Scanned: #%d (UID: %s)\n", current_cage_id, tagUID.c_str());
        
        // Instantly transmit the new cage ID
        myData.value_1 = (float)current_cage_id;
        myData.status_flag = true; // Flag indicates active scan
        esp_now_send(hubAddress, (uint8_t *) &myData, sizeof(myData));
        
        delay(1000); // Anti-spam debounce
    }

    // 2. Auto-Clear the active cage if nothing is scanned for 30 seconds
    // (Assuming you finished lifting it and swung it onto the deck)
    if (current_cage_id != 0 && (millis() - last_scan_time > CAGE_TIMEOUT_MS)) {
        current_cage_id = 0;
        Serial.println("Cage timeout. Hook empty.");
        
        myData.value_1 = 0.0;
        myData.status_flag = false;
        esp_now_send(hubAddress, (uint8_t *) &myData, sizeof(myData));
    }
}
