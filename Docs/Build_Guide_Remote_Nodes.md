# Remote ESP32-C3 Nodes Build Guide

## General Hardware Setup
Every node requires an ESP32-C3 and a reliable 12V-to-5V power delivery method.

## Node-Specific Instructions
### 1. Load Cell Nodes (Alpha & Beta)
* Requires HX711 Amplifier.
* Ensure strain gauges are mounted parallel to the sheer force.

### 2. RFID Node (Oyster Tracker)
* Requires MFRC522 SPI Module.

### 3. Proximity Node
* Mount the ultrasonic sensor facing away from the boom pivot.
