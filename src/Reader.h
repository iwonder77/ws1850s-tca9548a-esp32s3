#pragma once

#include <Arduino.h>
#include <MFRC522Debug.h>
#include <MFRC522DriverI2C.h>
#include <MFRC522v2.h>
#include <Wire.h>

#include "Config.h"
#include "MuxController.h"

enum TagState { TAG_ABSENT, TAG_DETECTED, TAG_PRESENT, TAG_REMOVED };

// Reader class owns channel switching on mux for now
struct MuxGuard {
  MuxGuard(uint8_t ch) { MuxController::selectChannel(ch); }
  ~MuxGuard() { MuxController::disableChannel(); }
};

class Reader {
public:
  Reader(const char *name, uint8_t channel)
      : name_(name), channel_(channel), driver_(config::WS1850S_I2C_ADDR, Wire),
        reader_(driver_) {};

  void init();
  void update();
  void printStatus() const;

  TagState getTagState() const { return tag_state; };
  uint8_t getChannel() const { return channel_; }
  bool getReaderStatus() const { return reader_ok; }

private:
  const char *name_;
  uint8_t channel_;
  MFRC522DriverI2C driver_;
  MFRC522 reader_;

  bool reader_ok = false;

  TagState tag_state = TAG_ABSENT;
  uint32_t last_seen_time = 0;
  uint32_t first_seen_time = 0;

  uint8_t consecutive_fails = 0;
  uint8_t last_UID[10]{};
  uint8_t last_UID_length = 0;

  void clearTagData();
  void readTagData();
  uint8_t calculateChecksum(const uint8_t *data, uint8_t length);
  bool compareUID(uint8_t *uid1, uint8_t len1, uint8_t *uid2, uint8_t len2);
};
