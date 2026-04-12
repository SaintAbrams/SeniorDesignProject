# Main Crane Hub Build Guide

## Bill of Materials (BOM)
* ESP32-S3 (Standard)
* 4-Channel Optoisolated Relay Board
* 12V to 5V Step-Down Buck Converter

## 3D Printing Parameters
* **Material:** ASA or PETG (Must be UV and heat resistant)
* **Notes:** Print with a TPU gasket for IP65 waterproofing.

## Assembly Instructions
1. **Power:** Wire the buck converter to the main crane 12V bus.
2. **Relays:** Wire ESP32 pins 4, 5, 6, and 7 to the relay signal inputs.
