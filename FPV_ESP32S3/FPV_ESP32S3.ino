/*
 * PROJECT: Marine Crane AI - FPV Headset (Final Release)
 * HARDWARE: ESP32-S3 Sense (XIAO) + OV3660 + 0.42" OLED
 * AI MODEL: saintabrams-project-1
 * ROLE: Captures video, runs FOMO inference, updates HUD, sends commands.
 */

#include <saintabrams-project-1_inferencing.h> // Edge Impulse Model
#include "esp_camera.h"
#include <esp_now.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- XIAO ESP32-S3 Sense Camera Pins ---
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39
#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

// --- Micro-OLED Hardware Profile ---
#define SCREEN_WIDTH 72
#define SCREEN_HEIGHT 40
#define OLED_RESET -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// TODO: Replace with the MAC Address of the MAIN CRANE S3
uint8_t craneHubAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 

// --- ESP-NOW Data Structures ---
struct TelemetryPacket {
    float wind_knots;
    float load_lbs;
    int   cage_id;
    bool  is_locked; 
} currentTelemetry;

struct CommandPacket {
    uint8_t command_id; // 0=None, 1=Hoist, 2=Lower, 3=SlewL, 4=SlewR, 5=Halt
} outgoingCommand;

SemaphoreHandle_t stateMutex;

// --- Global Camera Frame Buffer for AI ---
camera_fb_t * fb = NULL;

// --- Edge Impulse Camera Callback ---
// Converts the raw RGB565 camera buffer into the RGB888 format FOMO expects
int ei_camera_get_data(size_t offset, size_t length, float *out_ptr) {
    size_t pixel_ix = offset * 2; // 2 bytes per pixel in RGB565
    size_t bytes_left = length;
    size_t out_ptr_ix = 0;

    while (bytes_left != 0) {
        // Unpack RGB565 to RGB888
        uint16_t pixel = (fb->buf[pixel_ix] << 8) | fb->buf[pixel_ix + 1];
        uint8_t r = ((pixel >> 11) & 0x1F) << 3;
        uint8_t g = ((pixel >> 5)  & 0x3F) << 2;
        uint8_t b = (pixel         & 0x1F) << 3;
        
        // Pack into Edge Impulse 24-bit float format
        out_ptr[out_ptr_ix] = (r << 16) + (g << 8) + b;

        out_ptr_ix++;
        pixel_ix += 2;
        bytes_left--;
    }
    return 0;
}

// --- Core 0: ESP-NOW Receive Callback ---
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    if (len == sizeof(TelemetryPacket)) {
        if (xSemaphoreTake(stateMutex, portMAX_DELAY)) {
            memcpy(&currentTelemetry, incomingData, sizeof(currentTelemetry));
            xSemaphoreGive(stateMutex);
        }
    }
}

// --- Core 1: HUD Drawing Function ---
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

    // INTERLOCK OVERRIDE
    if (locked) {
        display.setTextSize(2); 
        display.setCursor(12, 12); 
        display.print("STOP");
        display.display();
        return; 
    }

    // TOP ROW: Environment & Load
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.printf("W:%.0f L:%.0f", wind, load);

    // MIDDLE ROW: AI Command
    display.setTextSize(2);
    display.setCursor(0, 14);
    if (gesture == "thumbs_up") display.print("HOIST");
    else if (gesture == "thumbs_down") display.print("LOWER");
    else if (gesture == "index_left") display.print("SLEW L");
    else if (gesture == "index_right") display.print("SLEW R");
    else if (gesture == "fist") display.print("HALT");
    else display.print("READY");

    // BOTTOM ROW: Oyster Cage Auth
    display.setTextSize(1);
    display.setCursor(0, 32);
    if (cage == 0) display.print("CAGE: NONE");
    else if (cage == 999) display.print("CAGE: UNKWN"); 
    else display.printf("CAGE: #%d", cage);

    display.display();
}

void setup() {
    Serial.begin(115200);
    
    // 1. Initialize OLED HUD
    Wire.begin(5, 6); 
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
        Serial.println("OLED init failed");
    }
    display.clearDisplay(); display.display();

    // 2. Initialize Telemetry Mutex
    stateMutex = xSemaphoreCreateMutex();
    currentTelemetry = {0.0, 0.0, 0, false};

    // 3. Initialize ESP-NOW
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) return;
    esp_now_register_recv_cb(OnDataRecv);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, craneHubAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);

    // 4. Initialize OV3660 Camera
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    
    // Critical AI parameters: Capture in raw RGB565 to match FOMO ingestion
    config.pixel_format = PIXFORMAT_RGB565; 
    config.frame_size = FRAMESIZE_240X240; 
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }
}

void loop() {
    // 1. Capture Camera Frame
    fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }

    // 2. Wrap Image Data for Edge Impulse
    signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &ei_camera_get_data;

    // 3. Run FOMO Inference
    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);
    
    // Free the camera memory immediately after inference to prevent memory leaks
    esp_camera_fb_return(fb);

    if (err != EI_IMPULSE_OK) {
        Serial.printf("ERR: Failed to run classifier (%d)\n", err);
        return;
    }

    // 4. Parse the FOMO Bounding Boxes
    String active_gesture = "READY";
    float highest_confidence = 0.0;

    for (size_t ix = 0; ix < result.bounding_boxes_count; ix++) {
        auto bb = result.bounding_boxes[ix];
        // Only accept commands with > 75% AI confidence
        if (bb.value > 0.75 && bb.value > highest_confidence) {
            highest_confidence = bb.value;
            active_gesture = bb.label; 
        }
    }

    // 5. Map the Gesture to a Network Command
    if (active_gesture == "thumbs_up") outgoingCommand.command_id = 1;
    else if (active_gesture == "thumbs_down") outgoingCommand.command_id = 2;
    else if (active_gesture == "index_left") outgoingCommand.command_id = 3;
    else if (active_gesture == "index_right") outgoingCommand.command_id = 4;
    else if (active_gesture == "fist") outgoingCommand.command_id = 5;
    else outgoingCommand.command_id = 0; 

    // 6. Transmit Command to Crane
    esp_now_send(craneHubAddress, (uint8_t *) &outgoingCommand, sizeof(outgoingCommand));

    // 7. Update OLED Display Optics
    updateHUD(active_gesture);
}
