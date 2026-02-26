/*
 * ----------------------------------------------
 * PROJECT: rfid-scan-on-demand
 * Description: Scan NTAG213 tags on Enter keypress via Serial Monitor
 *
 * Board: Waveshare ESP32-S3-ETH
 * RFID Module: M5Stack RFID2 (WS1850S, I2C @ 0x28)
 * Libraries:
 *   - Wire.h
 *   - MFRC522v2.h
 *   - MFRC522DriverI2C.h
 *   - MFRC522Debug.h
 * Notes:
 *   - ESP32-S3 GPIO matrix allows I2C on almost any pin via Wire.begin(SDA, SCL)
 *   - WS1850S version check bypassed — known quirk with this module vs pure MFRC522
 * ----------------------------------------------
 */

#include <Wire.h>
#include <MFRC522v2.h>
#include <MFRC522DriverI2C.h>
#include <MFRC522Debug.h>

// ----- PIN DEFINITIONS -----
const uint8_t I2C_SDA = 16;
const uint8_t I2C_SCL = 17;
const uint8_t BUTTON_PIN = 1;

// ----- INTERRUPT VARs -----
volatile bool pressed = false;
volatile unsigned long lastPressTime = 0;

// ----- CONSTANTS -----
const uint8_t TAG_START_PAGE = 4;       // NTAG213 user data begins at page 4
const uint8_t RFID2_I2C_ADDR = 0x28;    // M5Stack RFID2 fixed address
const uint32_t SCAN_TIMEOUT_MS = 5000;  // how long to poll for a tag after Enter
const uint32_t BUTTON_DEBOUNCE_MS = 250;

// ----- RFID SETUP -----
MFRC522DriverI2C driver{ RFID2_I2C_ADDR, Wire };
MFRC522 reader{ driver };

// ===== UTILITY =====
uint8_t calculateChecksum(const uint8_t *data, uint8_t length) {
  uint8_t result = 0;
  for (uint8_t i = 0; i < length; i++) {
    result ^= data[i];
  }
  return result;
}

// Drain any leftover bytes in the Serial RX buffer.
// Important because Serial Monitor sends \r\n (two chars) on Enter,
// which would otherwise trigger two back-to-back scan attempts.
void flushSerial() {
  while (Serial.available()) {
    Serial.read();
  }
}

// ===== ISR ======
void IRAM_ATTR buttonISR() {
  unsigned long now = millis();
  if (now - lastPressTime > BUTTON_DEBOUNCE_MS) {
    pressed = true;
    lastPressTime = now;
  }
}

// ===== RFID READ =====
void readStructFromTag(uint8_t startPage) {
  // MIFARE_Read returns 16 bytes (4 pages) + 2 CRC bytes → buffer needs 18 bytes minimum.
  // Our struct is 6 bytes, so pages 4 and 5 are sufficient. Reading 16 is fine; we only use what we need.
  byte buffer[18];
  byte bufferSize = sizeof(buffer);

  MFRC522::StatusCode status = reader.MIFARE_Read(startPage, buffer, &bufferSize);

  if (status != MFRC522::StatusCode::STATUS_OK) {
    Serial.print("  Read failed. Status: ");
    Serial.println(MFRC522Debug::GetStatusCodeName(status));
    return;
  }

  MFRC522Debug::PICC_DumpToSerial(reader, Serial, &(reader.uid));
}


// ===== SCAN ONCE =====
// Polls for a tag for up to SCAN_TIMEOUT_MS milliseconds.
// Reads and halts cleanly on detection, or reports timeout.

void scanOnce() {
  Serial.println("\nPolling for tag...");

  unsigned long start = millis();

  while (millis() - start < SCAN_TIMEOUT_MS) {
    if (reader.PICC_IsNewCardPresent() && reader.PICC_ReadCardSerial()) {

      // PICC_DumpToSerial() in readStructFromTag() method prints uid
      // // Print UID for reference — useful during debugging
      // Serial.print("     UID: ");
      // for (byte i = 0; i < reader.uid.size; i++) {
      //   if (reader.uid.uidByte[i] < 0x10) Serial.print("0");
      //   Serial.print(reader.uid.uidByte[i], HEX);
      //   Serial.print(" ");
      // }
      // Serial.println();

      readStructFromTag(TAG_START_PAGE);

      // Always halt cleanly to reset the tag's state machine
      reader.PICC_HaltA();
      reader.PCD_StopCrypto1();

      return;  // done — wait for next Enter
    }
    delay(50);
  }

  Serial.println("  No tag detected within timeout.");
}


// ===== SETUP & LOOP =====

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }  // wait for USB CDC Serial on ESP32-S3

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);  // RFID2 supports 400 kHz fast mode; reduces read latency

  reader.PCD_Init();
  delay(100);

  Serial.println("=== RFID Scanner Ready ===");
  Serial.print("I2C SDA: GPIO");
  Serial.println(I2C_SDA);
  Serial.print("I2C SCL: GPIO");
  Serial.println(I2C_SCL);

  pinMode(BUTTON_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

  Serial.println("Press button to scan for a tag.");
}

void loop() {
  if (pressed) {
    Serial.println("\nButton pressed!");
    delay(10);
    pressed = false;
    scanOnce();
    Serial.println("\nReady. Press button to scan again.");
  }
  delay(10);
}
