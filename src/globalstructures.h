#include <Arduino.h>

#pragma once
struct ShortPacket {
  uint8_t receiveraddr[6];
  uint8_t sourceaddr[6];
  uint8_t channel;
};

#pragma once
const char substrings[111] {
  '0','1','2','3','4','5','6','7','8','9',
  'a','b','c','d','e','f','g','h','i','j',
  'k','l','m','n','o','p','q','r','s','t',
  'u','v','w','x','y','z','z','y','x','w',
  'v','u','t','s','r','q','p','o','n','m',
  'l','k','j','i','h','g','f','e','d','c',
  'b','a','9','8','7','6','5','4','3','2',
  '1','0','q','w','e','r','t','y','u','i',
  'o','p','a','s','d','f','g','h','j','k',
  'l','z','x','c','v','b','n','m','1','3',
  '2','4','3','5','4','6','5','7','6','8',
  '9'};

#pragma once
const uint8_t beaconPacketTemplate[109] = {
    /*  0 - 3  */ 0x80, 0x00, 0x00, 0x00,             // Type/Subtype: managment beacon frame
    /*  4 - 9  */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination: broadcast
    /* 10 - 15 */ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Source
    /* 16 - 21 */ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Source
    // Fixed parameters
    /* 22 - 23 */ 0x00, 0x00,                         // Fragment & sequence number (will be done by the SDK)
    /* 24 - 31 */ 0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00, // Timestamp
    /* 32 - 33 */ 0xe8, 0x03,                         // Interval: 0x64, 0x00 => every 100ms - 0xe8, 0x03 => every 1s
    /* 34 - 35 */ 0x31, 0x00,                         // capabilities Information
    // Tagged parameters
    // SSID parameters
    /* 36 - 37 */ 0x00, 0x20,                         // Tag: Set SSID length, Tag length: 32
    /* 38 - 69 */
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20,                           // SSID
    // Supported Rates
    /* 70 - 71 */ 0x01, 0x08,                         // Tag: Supported Rates, Tag length: 8 
    /* 72 */ 0x82,                    // 1(B)
    /* 73 */ 0x84,                    // 2(B)
    /* 74 */ 0x8b,                    // 5.5(B)
    /* 75 */ 0x96,                    // 11(B)
    /* 76 */ 0x24,                    // 18
    /* 77 */ 0x30,                    // 24
    /* 78 */ 0x48,                    // 36
    /* 79 */ 0x6c,                    // 54
    // Current Channel
    /* 80 - 81 */ 0x03, 0x01,         // Channel set, length
    /* 82 */      0x01,               // Current Channel 
    // RSN information
    /*  83 -  84 */ 0x30, 0x18,
    /*  85 -  86 */ 0x01, 0x00,
    /*  87 -  90 */ 0x00, 0x0f, 0xac, 0x02,
    /*  91 -  92 */ 0x02, 0x00,
    /*  93 - 100 */ 0x00, 0x0f, 0xac, 0x04, 0x00, 0x0f, 0xac, 0x04, /*Fix: changed 0x02(TKIP) to 0x04(CCMP) is default. WPA2 with TKIP not supported by many devices*/
    /* 101 - 102 */ 0x01, 0x00,
    /* 103 - 106 */ 0x00, 0x0f, 0xac, 0x02,
    /* 107 - 108 */ 0x00, 0x00
};

#pragma once
const uint8_t pseudoRealisticHeader[28] = {
  /*  0 - 1  */ 0xC0, 0x00,                          // type, subtype
  /*  2 - 3  */ 0x00, 0x00,                          // duration (SDK takes care of that)
  /*  4 - 9  */ 0xFA, 0xBA, 0xCA, 0xBA, 0x08, 0x01,  // reciever (target)
  /* 10 - 15 */ 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,  // source (ap)
  /* 16 - 21 */ 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,  // BSSID (ap)
  /* 22 - 23 */ 0x00, 0x00,                          // fragment & squence number
  /* 24 25 26 27 */ 0x00, 0x00, 0x00, 0x00,          // data size, packet type, packet id, device id
};

#pragma once
void stringToUint8Array(const String &str, uint8_t *array, size_t arraySize);

#pragma once
String uint8ArrayToString(const uint8_t *array, size_t arraySize);

#pragma once
struct Point {
  float x;
  float y;
};

#pragma once
struct Location {
  String name;
  Point point;
};

#pragma once
struct LootBlock {
  uint8_t money;
  Point point;
  float radius;
};

#pragma once
bool blinkNow(uint16_t timeScale);

#pragma once
class BeaconPacketConstructor {
  public:
    void setProperties(String name, bool wpa2, int8_t hashTagIndex, uint8_t* mac, bool cd1s);
    void setMac(uint8_t* mac);
    void setChannel(uint8_t c);
    uint8_t* getBytes();
    uint8_t getPacketSize();
    uint8_t getChannel();
  private:
    uint8_t* packet = new uint8_t[0];
    uint8_t nameSize, packetSize, channel;
};

#pragma once
class BoolPack {
public:
  BoolPack(uint8_t prototype) : data(prototype) {}
  BoolPack() : data(0) {}
  void setBool(uint8_t index, bool value) {
    if (index < 8) {
      if (value) {
        data |= (1 << 7-index);
      } else {
        data &= ~(1 << 7-index);
      }
    }
  }
  bool getBool(uint8_t index) const {
    if (index < 8) {
      return data & (1 << 7-index);
    }
    return false;
  }
  uint8_t getPackedData() const {
    return data;
  }
  void setPackedData(uint8_t packedData) {
    data = packedData;
  }
private:
  uint8_t data;
};