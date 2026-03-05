/**
 * Project: Prototyping for Gene Machine in Salon exhibit
 * File: ws1850s-tca9548A-esp32s3.ino
 * Description: Scan NTAG213 tags with two M5Stack RFID2 readers connected to channels
 *              on the TCA9548A I2C mux
 *
 * Author: Isai Sanchez
 * Date: 3-5-26
 * MCU Board: Waveshare ESP32-S3-ETH
 * Hardware:
 *  - RFID2 Module: M5Stack RFID2 (WS1850S, I2C @ 0x28)
 *  - I2C Multiplexer: TCA9548A
 * Libraries:
 *  - Wire.h
 *  - MFRC522v2.h (https://github.com/OSSLibraries/Arduino_MFRC522v2/tree/master)
 *  - MFRC522DriverI2C.h
 *  - MFRC522Debug.h
 * Notes:
 *  - ESP32-S3 GPIO matrix allows I2C on almost any pin via Wire.begin(SDA_PIN, SCL_PIN)
 *  - WS1850S version check bypassed — known quirk with this module vs pure MFRC522
 *
 * (c) Thanksgiving Point Exhibits Electronics Team — 2025
 */

#include <Wire.h>

#include "src/Config.h"
#include "src/Reader.h"

Reader readers[] = { { "ch_0", 0 }, { "ch_1", 1 } };
uint32_t last_poll_time = 0;

// ----- BUTTON INTERRUPT STUFF -----
const uint8_t BUTTON_PIN = 48;
const uint8_t LED_PIN = 47;
volatile bool pressed = false;
volatile unsigned long last_press_time = 0;

// ===== ISR ======
void IRAM_ATTR buttonISR() {
  unsigned long now = millis();
  if (now - last_press_time > config::BUTTON_DEBOUNCE_MS) {
    pressed = !pressed;
    last_press_time = now;
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }  // wait for USB CDC Serial on ESP32-S3

  Wire.begin(config::I2C_SDA, config::I2C_SCL);
  Wire.setClock(400000);  // RFID2 supports 400 kHz fast mode; reduces read latency
  delay(100);

  for (int i = 0; i < config::NUM_READERS; i++) {
    readers[i].init();
  }

  Serial.println("=== RFID Scanner Ready ===");
  Serial.print("I2C SDA: GPIO");
  Serial.println(config::I2C_SDA);
  Serial.print("I2C SCL: GPIO");
  Serial.println(config::I2C_SCL);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(BUTTON_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

  Serial.println("Press button to scan for a tag.");
}

void loop() {
  uint32_t now = millis();

  if (now - last_poll_time >= config::POLL_INTERVAL_MS) {
    for (int i = 0; i < config::NUM_READERS; i++) {
      readers[i].update();
    }
    last_poll_time = now;
  } else {
    digitalWrite(LED_PIN, LOW);
  }
  delay(10);
}
