// config.h

#pragma once

// === Pin-definities ===
#define SDA_PIN       5
#define SCL_PIN       6
#define FSR_PIN       3
#define RESET_PIN     4
#define NEOPIXEL_PIN  44
#define NUMPIXELS     16

// === I²C-adres 7-segment display ===
#define I2C_ADDR_DISPLAY   0x70

// === FSR-thresholds ===
static const int FSR_FULL_THRESHOLD    = 2500;  // vol glas ≥ 2500
static const int FSR_NOGLASS_THRESHOLD = 1800;  // geen glas ≤ 1800
static const int FSR_EMPTY_THRESHOLD   = 2000;  // leeg glas ≥ 2000


