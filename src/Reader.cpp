#include "Reader.h"
#include "Config.h"

// NOTE: must call Wire.begin() in setup() before we initialize reader!
void Reader::init() {
  // a guard is used to ensure channel is disabled when this guy goes out of
  // scope (see struct in Reader.h)
  MuxGuard guard(channel_);

  Serial.print("Testing ");
  Serial.print(name_);
  Serial.print(" Reader I2C Communication on Channel ");
  Serial.print(channel_);
  Serial.print(": ");

  Wire.beginTransmission(config::WS1850S_I2C_ADDR);
  if (Wire.endTransmission() != 0) {
    Serial.println("FAILED - I2C Communication ERROR");
    reader_ok = false;
    return;
  }

  Serial.println(" SUCCESS");
  reader_ok = true;

  // initialize hardware instance
  reader_.PCD_Init();
}

void Reader::update() {
  MuxGuard guard(channel_);

  if (!reader_ok) {
    return;
  }

  uint32_t now = millis();
  bool tag_detected = false;

  // --- TAG DETECTION ---
  if (reader_.PICC_IsNewCardPresent() && reader_.PICC_ReadCardSerial()) {
    tag_detected = true;

    // check if this is the same tag or a different one
    bool is_same_tag = (last_UID_length == reader_.uid.size) &&
                       compareUID(last_UID, last_UID_length,
                                  reader_.uid.uidByte, reader_.uid.size);

    // update UID
    memcpy(last_UID, reader_.uid.uidByte, reader_.uid.size);
    last_UID_length = reader_.uid.size;

    // update timing and error
    last_seen_time = now;
    consecutive_fails = 0;

    // transition tagState now that a tag has been detected
    switch (tag_state) {
    case TAG_ABSENT:
      tag_state = TAG_DETECTED;
      first_seen_time = now; // start debounce timer
      Serial.print(name_);
      Serial.println(": New tag detected!");
      break;
    case TAG_DETECTED:
      // check debounce timer in case we have a false positive detection
      // this debounce check ensures tag is actually present
      if (now - first_seen_time > config::TAG_DEBOUNCE_TIME) {
        tag_state = TAG_PRESENT;
        Serial.print(name_);
        Serial.println(": Tag confirmed present, reading data");
        // once tag is confirmed present, now we read its data
        readTagData();
      }
      break;
    case TAG_PRESENT:
      // check if this is the same tag we saw on previous turn
      if (!is_same_tag) {
        // different tag detected
        tag_state = TAG_DETECTED;
        first_seen_time = now;
        Serial.print(name_);
        Serial.println(": Different tag detected, clearing previous data");
        // clear previous tag data before we move on to reading this one
        clearTagData();
      }
      // otherwise same tag is still present - no action needed
      break;
    case TAG_REMOVED:
      tag_state = TAG_DETECTED;
      first_seen_time = now;
      Serial.print(name_);
      Serial.println(": Tag returned!");
      break;
    default:
      break;
    }
  }

  // --- ABSENCE DETECTION ---
  if (!tag_detected && tag_state != TAG_ABSENT) {
    consecutive_fails++;

    switch (tag_state) {
    case TAG_DETECTED:
      // small timeout for tags that were just detected or for false positives
      if (consecutive_fails > 2) {
        tag_state = TAG_ABSENT;
        Serial.print(name_);
        Serial.println(": Tag detection failed");
        clearTagData();
      }
      break;
    case TAG_PRESENT:
      // more lenient/longer timeout for already established tags
      if (consecutive_fails >= config::TAG_PRESENCE_THRESHOLD ||
          now - last_seen_time > config::TAG_ABSENCE_TIMEOUT) {
        tag_state = TAG_REMOVED;
        Serial.print(name_);
        Serial.println(": Tag removed");
        clearTagData();
      }
      break;
    case TAG_REMOVED:
      // confirm removal
      if (now - last_seen_time > config::TAG_ABSENCE_TIMEOUT) {
        tag_state = TAG_ABSENT;
        Serial.print(name_);
        Serial.println(": Tag removal confirmed");
      }
      break;
    default:
      break;
    }
  }
}

void Reader::printStatus() const {
  switch (tag_state) {
  case TAG_ABSENT:
    Serial.println("No card");
    break;
  case TAG_DETECTED:
    Serial.println("Detecting...");
    break;
  case TAG_PRESENT:
    Serial.println("Tag present");
    break;
  case TAG_REMOVED:
    Serial.println("Card removed (confirming...)");
    break;
  }
}

void Reader::clearTagData() {
  last_UID_length = 0;
  memset(last_UID, 0, sizeof(last_UID));
}

void Reader::readTagData() {
  if (!reader_ok || tag_state != TAG_PRESENT)
    return;

  Serial.print(name_);
  Serial.println(": Reading tag data...");

  byte buffer[18];
  byte bufferSize = sizeof(buffer);

  if (reader_.MIFARE_Read(config::TAG_START_PAGE, buffer, &bufferSize) !=
      MFRC522::StatusCode::STATUS_OK) {
    Serial.print(name_);
    Serial.println(": Failed to read card data");
    // Don't clear tag data - we know tag is present, just couldn't read it
    return;
  }

  /*****************
   * DON'T HALT - let tag remain active for continuous detection
   * reader.PICC_HaltA();
   ******************/
  reader_.PCD_StopCrypto1();
}

// ========== UTILITY FUNCTIONS ==========
uint8_t Reader::calculateChecksum(const uint8_t *data, uint8_t length) {
  uint8_t sum = 0;
  for (uint8_t i = 0; i < length; i++) {
    sum ^= data[i];
  }
  return sum;
}

bool Reader::compareUID(byte *uid1, uint8_t len1, byte *uid2, uint8_t len2) {
  return (len1 == len2) && memcmp(uid1, uid2, len1) == 0;
}
