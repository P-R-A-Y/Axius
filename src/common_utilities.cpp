#include "Arduino.h"
#include "common_utilities.h"

void BeaconPacketConstructor::setProperties(String name, bool wpa2, int8_t hashTagIndex, uint8_t* mac, bool cd1s, bool hashTagIndexBackwards) {
  name.replace("{hashindex}", String(hashTagIndex));
  if (name.indexOf("{nohash}") >= 0) {
    name.replace("{nohash}", "");
    hashTagIndex = -1;
  }
  if (hashTagIndex == -1) nameSize = min(name.length(), (uint) 32);
  else                    nameSize = min(name.length() + 4, (uint) 32);
  packetSize = 109 - 32 + nameSize;
  if (!wpa2) packetSize -= 26;

  if (packet) {
    delete[] packet;
    packet = nullptr;
  }
  packet = new uint8_t[packetSize];

  memcpy_P(&packet[0], &beaconPacketTemplate[0], 38);                            // front part
  memcpy(&packet[10], mac, 6);                                                   // set mac
  memcpy(&packet[16], mac, 6);                                                   // set mac
  packet[37] = nameSize;                                                         // set name size
  if (cd1s) {
    packet[32] = 0xe8; 
    packet[33] = 0x03; 
  } else {
    packet[32] = 0x64; 
    packet[33] = 0x00; 
  }
  if (wpa2) packet[34] = 0x31;                                                   // set wpa enabled
  else      packet[34] = 0x21;                                                   // set wpa disabled
  memcpy(&packet[((hashTagIndex > -1 && hashTagIndexBackwards) ? 42 : 38)], &name[0], nameSize - (hashTagIndex == -1 ? 0 : 4));        // set name
  if (hashTagIndex > -1) {
    if (hashTagIndexBackwards) {
      packet[38 + 0] = pgm_read_byte(&substrings[hashTagIndex]);
      packet[38 + 1] = pgm_read_byte(&substrings[hashTagIndex + 1]);
      packet[38 + 2] = '#';
      packet[38 + 3] = ' ';
    } else {
      packet[38 + nameSize - 4] = ' ';
      packet[38 + nameSize - 3] = '#';
      packet[38 + nameSize - 2] = pgm_read_byte(&substrings[hashTagIndex]);
      packet[38 + nameSize - 1] = pgm_read_byte(&substrings[hashTagIndex + 1]);
    }
  }
  memcpy_P(&packet[38 + nameSize], &beaconPacketTemplate[70], wpa2 ? 39 : 13);   // set back part
  if (wpa2) packet[34] = 0x31;                                                   // set wpa2
  else packet[34] = 0x21;                                                        // set no wpa2
};

void BeaconPacketConstructor::setChannel(uint8_t c) {
  channel = c;
  packet[82 - 32 + nameSize] = channel;
};

void BeaconPacketConstructor::setMac(uint8_t* mac) {
  memcpy(&packet[10], mac, 6);
  memcpy(&packet[16], mac, 6);
}

uint8_t* BeaconPacketConstructor::getBytes() {
  return packet;
};

uint8_t BeaconPacketConstructor::getPacketSize() {
  return packetSize;
};

uint8_t BeaconPacketConstructor::getChannel() {
  return channel;
}

bool ismacgood(const uint8_t* arr) {
  return memcmp(arr, BAD_MAC, 6) != 0;
}

bool ismaclike(const uint8_t* cur, const uint8_t* tar) {
  return memcmp(cur, tar, 6) == 0;
}

bool blinkNow(uint16_t timeScale) {
  return (millis() % timeScale) > (timeScale / 2);
}

String strmac(uint8_t* mac) {
  String result = "";
  for (int i = 0; i < 6; i++) {
      if (mac[i] < 16) result += "0";
      result += String(mac[i], HEX);
      if (i < 5) result += ":";
  }
  return result;
}

String readProgMemString(const char* str) {
  size_t len = 0;
  char c;
  do {
      c = pgm_read_byte(str + len);
      len++;
  } while (c != '\0' && len < 256);

  char buffer[len];
  
  strncpy_P(buffer, str, len);
  
  return String(buffer);
}

void writeCharArrayToUint8Array(char* charArray, size_t charSize, uint8_t* uint8Array, size_t startAddress) {
  for (size_t i = 0; i < charSize; i++) {
    uint8Array[startAddress + i] = static_cast<uint8_t>(charArray[i]);
  }
}

void readUint8ArrayToCharArray(uint8_t* uint8Array, size_t startAddress, char* charArray, size_t charSize) {
  for (size_t i = 0; i < charSize; i++) {
    charArray[i] = static_cast<char>(uint8Array[startAddress + i]);
  }
}

String uint64_tToString(uint64_t ll) {
  String result = "";
  do {
    result = int(ll % 10) + result;
    ll /= 10;
  } while (ll != 0);
  return result;
}

bool comparePrefix(char* prefix1, char* prefix2) {
  for (uint8_t i = 0; i < 6; i++) {
    if (prefix1[i] != prefix2[i]) return false;
  }
  return true;
}

char* uint8ArrayToCharArray(uint8_t *array, size_t arraySize) {
  char* charstring = new char[arraySize+1];

  for (size_t i = 0; i < arraySize; i++) {
    charstring[i] = static_cast<char>(array[i]);
  }

  charstring[arraySize] = 0;

  return charstring;
}

String uint8ArrayToString(uint8_t *array, size_t arraySize) {
  String result = "";
  for (size_t i = 0; i < arraySize; i++) {
    if (array[i] != 0) {
      result += static_cast<char>(array[i]);
    } else {
      break;
    }
  }
  return result;
}

void stringToUint8Array(const String &str, uint8_t *array, size_t arraySize) {
  size_t len = str.length();
  if (len > arraySize) len = arraySize;
  for (size_t i = 0; i < len; i++) {
    array[i] = static_cast<uint8_t>(str[i]);
  }
  for (size_t i = len; i < arraySize; i++) {
    array[i] = 0;
  }
}

float normalize(float min, float max, float current) {
  if (current >= max)      return 1.0;
  else if (current <= min) return 0.0;
  else                     return (current - min) / (max - min);
}

float lerp(float min, float max, float curNorm) {
  return min * (1.0 - curNorm) + (max * curNorm);
}