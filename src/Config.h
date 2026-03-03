#pragma once

#include <Arduino.h>

namespace config {
// ----- PIN DEFINITIONS -----
const uint8_t I2C_SDA = 16;
const uint8_t I2C_SCL = 17;
//
// ----- CONSTANTS -----
const uint32_t BUTTON_DEBOUNCE_MS = 250;
const uint8_t CHANNEL_SWITCH_SETTLE_MS = 5;

// ----- RFID2 READER -----
const uint8_t NUM_READERS = 2;
const uint16_t POLL_INTERVAL_MS =
    50; // how often to poll all readers for tags (ms)
const uint8_t READER_INIT_SETTLE_MS = 10;
const uint32_t TAG_DEBOUNCE_TIME = 150;   // debounce time for tag detection
const uint32_t TAG_ABSENCE_TIMEOUT = 450; // time before considering tag removed
const uint8_t TAG_PRESENCE_THRESHOLD = 3; // num fails before marking absent
const uint8_t TAG_START_PAGE = 4; // page # to read data from in tag memory

// ----- I2C Addresses -----
static constexpr uint8_t MUX_ADDR = 0x70;
static constexpr uint8_t WS1850S_I2C_ADDR = 0x28;
} // namespace config
