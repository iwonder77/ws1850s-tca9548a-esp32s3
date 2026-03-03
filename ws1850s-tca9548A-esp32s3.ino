/*
 * ----------------------------------------------
 * PROJECT: rfid-scan-on-demand
 * Description: Scan NTAG213 tags on Enter keypress via Serial Monitor
 *
 * Board: Waveshare ESP32-S3-ETH
 * RFID Module: M5Stack RFID2 (WS1850S, I2C @ 0x28)
 * Libraries:
 *   - Wire.h
 *   - MFRC522v2.h (https://github.com/OSSLibraries/Arduino_MFRC522v2/tree/master)
 *   - MFRC522DriverI2C.h
 *   - MFRC522Debug.h
 * Notes:
 *   - ESP32-S3 GPIO matrix allows I2C on almost any pin via Wire.begin(SDA, SCL)
 *   - WS1850S version check bypassed — known quirk with this module vs pure MFRC522
 * ----------------------------------------------
 */

#include <Wire.h>

#include "src/Config.h"
#include "src/Reader.h"

Reader readers[] = { { "ch_0", 0 }, { "ch_1", 1 } };
uint32_t last_poll_time = 0;

// ----- BUTTON INTERRUPT STUFF -----
const uint8_t BUTTON_PIN = 48;
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

  pinMode(BUTTON_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

  Serial.println("Press button to scan for a tag.");
}

void loop() {
  uint32_t now = millis();
  if (pressed) {
    if (now - last_poll_time >= config::POLL_INTERVAL_MS) {
      for (int i = 0; i < config::NUM_READERS; i++) {
        readers[i].update();
      }
      last_poll_time = now;
    }
  }
  delay(10);
}
