/*
 * PROJECT: Marine Crane AI - Master Crane Hub
 * HARDWARE: ESP32-S3 (Standard) + Motor Relays
 * ROLE: Central telemetry aggregator, Safety Interlocks, Motor Controller
 */

#include <esp_now.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// --- Relay Pins (Adjust to your actual wiring) ---
const int PIN_RELAY_HOIST = 4;
const int PIN_RELAY_LOWER = 5;
const int PIN_RELAY_SLEW_L = 6;
const int PIN_RELAY_SLEW_R = 7;

// TODO: Replace with the MAC Address of the FPV HEADSET
uint8_t headsetAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 

// --- Global State Matrix ---
struct CraneState {
    float wind_speed_knots;
    bool  weather_red;
    float tension_alpha_lbs;
    bool  overload_flag;
    int   active_cage_id;
    bool  proximity_alert; 
} currentState;

SemaphoreHandle_t stateMutex;

// --- Data Structures ---
// The format sent by all C3 remote nodes
typedef struct struct_message {
    uint8_t node_type;    
    float value_1;        
    bool status_flag;     
} struct_message;

// The format received from the FPV Headset
struct CommandPacket {
    uint8_t command_id; 
} incomingCommand;

// The format sent TO the FPV Headset for the OLED
struct TelemetryPacket {
    float wind_knots;
    float load_lbs;
    int   cage_id;
    bool  is_locked; 
} outgoingTelemetry;

// --- Core 0: ESP-NOW Multi-Receiver ---
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingDataBytes, int len) {
    if (xSemaphoreTake(stateMutex, portMAX_DELAY)) {
        
        // Is this a command from the Headset?
        if (len == sizeof(CommandPacket)) {
            memcpy(&incomingCommand, incomingDataBytes, sizeof(incomingCommand));
        }
        // Is this telemetry from a C3 Node?
        else if (len == sizeof(struct_message)) {
            struct_message c3Data;
            memcpy(&c3Data, incomingDataBytes, sizeof(c3Data));
            
            switch(c3Data.node_type) {
                case 1: // Weather
                    currentState.wind_speed_knots = c3Data.value_1;
                    currentState.weather_red = c3Data.status_flag;
                    break;
                case 2: // Load Alpha
                    currentState.tension_alpha_lbs = c3Data.value_1;
                    currentState.overload_flag = c3Data.status_flag;
                    break;
                case 3: // Load Beta
                    if (c3Data.status_flag) currentState.overload_flag = true;
                    break;
                case 4: // RFID / Cage
                    currentState.active_cage_id = (int)c3Data.value_1;
                    break;
                case 5: // Proximity
                    currentState.proximity_alert = c3Data.status_flag;
                    break;
            }
        }
        xSemaphoreGive(stateMutex);
    }
}

// --- Motor Control Helper ---
void haltAllMotors() {
    digitalWrite(PIN_RELAY_HOIST, LOW);
    digitalWrite(PIN_RELAY_LOWER, LOW);
    digitalWrite(PIN_RELAY_SLEW_L, LOW);
    digitalWrite(PIN_RELAY_SLEW_R, LOW);
}

void setup() {
    Serial.begin(115200);

    // Initialize Motor Pins
    pinMode(PIN_RELAY_HOIST, OUTPUT);
    pinMode(PIN_RELAY_LOWER, OUTPUT);
    pinMode(PIN_RELAY_SLEW_L, OUTPUT);
    pinMode(PIN_RELAY_SLEW_R, OUTPUT);
    haltAllMotors();

    stateMutex = xSemaphoreCreateMutex();

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) return;
    esp_now_register_recv_cb(OnDataRecv);

    // Add Headset as a peer to send data back
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, headsetAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);

    // Default Safe State
    currentState = {0.0, false, 0.0, false, 0, false};
    incomingCommand.command_id = 0; 
}

void loop() {
    bool safety_lockout = false;
    uint8_t active_cmd = 0;

    // 1. Evaluate State Matrix & Incoming Commands
    if (xSemaphoreTake(stateMutex, portMAX_DELAY)) {
        if (currentState.weather_red || currentState.overload_flag || currentState.proximity_alert || incomingCommand.command_id == 5) {
            safety_lockout = true;
        }
        active_cmd = incomingCommand.command_id;
        
        // Pack data for the Headset
        outgoingTelemetry.wind_knots = currentState.wind_speed_knots;
        outgoingTelemetry.load_lbs = currentState.tension_alpha_lbs;
        outgoingTelemetry.cage_id = currentState.active_cage_id;
        outgoingTelemetry.is_locked = safety_lockout;
        
        xSemaphoreGive(stateMutex);
    }

    // 2. Transmit UI Data to Headset (Every 100ms)
    esp_now_send(headsetAddress, (uint8_t *) &outgoingTelemetry, sizeof(outgoingTelemetry));

    // 3. Execute Motor Relays
    if (safety_lockout) {
        haltAllMotors();
        Serial.println("CRANE LOCKED: Safety Interlock Active");
    } else {
        // Break before Make (ensure opposing relays aren't both ON)
        haltAllMotors(); 
        
        switch (active_cmd) {
            case 1: digitalWrite(PIN_RELAY_HOIST, HIGH); break;
            case 2: digitalWrite(PIN_RELAY_LOWER, HIGH); break;
            case 3: digitalWrite(PIN_RELAY_SLEW_L, HIGH); break;
            case 4: digitalWrite(PIN_RELAY_SLEW_R, HIGH); break;
        }
    }

    delay(100); 
}
