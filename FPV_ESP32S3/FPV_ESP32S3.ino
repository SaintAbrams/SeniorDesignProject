/*
 * PROJECT: Marine Crane AI - FPV Headset (Vision & UI)
 * HARDWARE: ESP32-S3 Sense + 0.42" OLED
 * ROLE: Runs Neural Network, updates HUD, sends commands to Crane Hub.
 */

#include <esp_now.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Hardware Profile ---
#define SCREEN_WIDTH 72
#define SCREEN_HEIGHT 40
#define OLED_RESET -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// TODO: Replace with the MAC Address of the MAIN CRANE S3
uint8_t craneHubAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 

// --- Data Structures ---
// Data coming FROM the Crane Hub TO the Headset
struct TelemetryPacket {
    float wind_knots;
    float load_lbs;
    int   cage_id;
    bool  is_locked; // True if any safety E-Stop is active
} currentTelemetry;

// Data going FROM the Headset TO the Crane Hub
struct CommandPacket {
    uint8_t command_id; // 0=None, 1=Hoist, 2=Lower, 3=SlewL, 4=SlewR, 5=Halt
} outgoingCommand;

SemaphoreHandle_t stateMutex;

// --- ESP-NOW Receive Callback ---
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    if (len == sizeof(TelemetryPacket)) {
        if (xSemaphoreTake(stateMutex, portMAX_DELAY)) {
            memcpy(&currentTelemetry, incomingData, sizeof(currentTelemetry));
            xSemaphoreGive(stateMutex);
        }
    }
}

// --- HUD Drawing Function ---
void updateHUD(String gesture) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    float wind = 0.0; float load = 0.0; int cage = 0; bool locked = false;
    
    if (xSemaphoreTake(stateMutex, portMAX_DELAY)) {
        wind = currentTelemetry.wind_knots;
        load = currentTelemetry.load_lbs;
        cage = currentTelemetry.cage_id;
        locked = currentTelemetry.is_locked;
        xSemaphoreGive(stateMutex);
    }

    if (locked) {
        display.setTextSize(2); 
        display.setCursor(12, 12); 
        display.print("STOP");
        display.display();
        return; 
    }

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.printf("W:%.0f L:%.0f", wind, load);

    display.setTextSize(2);
    display.setCursor(0, 14);
    if (gesture == "thumbs_up") display.print("HOIST");
    else if (gesture == "thumbs_down") display.print("LOWER");
    else if (gesture == "index_left") display.print("SLEW L");
    else if (gesture == "index_right") display.print("SLEW R");
    else if (gesture == "fist") display.print("HALT");
    else display.print("READY");

    display.setTextSize(1);
    display.setCursor(0, 32);
    if (cage == 0) display.print("CAGE: NONE");
    else if (cage == 999) display.print("CAGE: UNKWN"); 
    else display.printf("CAGE: #%d", cage);

    display.display();
}

void setup() {
    Serial.begin(115200);
    
    Wire.begin(5, 6); 
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
        Serial.println("OLED init failed");
    }
    display.clearDisplay(); display.display();

    stateMutex = xSemaphoreCreateMutex();

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) return;
    esp_now_register_recv_cb(OnDataRecv);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, craneHubAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);

    // Default startup state
    currentTelemetry = {0.0, 0.0, 0, false};
}

void loop() {
    // 1. Run AI Vision (Placeholder)
    String gesture = "thumbs_up"; 

    // 2. Map Gesture to Command ID
    if (gesture == "thumbs_up") outgoingCommand.command_id = 1;
    else if (gesture == "thumbs_down") outgoingCommand.command_id = 2;
    else if (gesture == "index_left") outgoingCommand.command_id = 3;
    else if (gesture == "index_right") outgoingCommand.command_id = 4;
    else if (gesture == "fist") outgoingCommand.command_id = 5;
    else outgoingCommand.command_id = 0;

    // 3. Send Command to Crane
    esp_now_send(craneHubAddress, (uint8_t *) &outgoingCommand, sizeof(outgoingCommand));

    // 4. Update HUD
    updateHUD(gesture);
    
    delay(50);
}
