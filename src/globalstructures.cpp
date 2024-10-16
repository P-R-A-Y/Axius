#include "Arduino.h"
#include "globalstructures.h"

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

  delete[] packet;
  packet = new uint8_t[packetSize];

  memcpy(&packet[0], &beaconPacketTemplate[0], 38);                              // front part
  memcpy(&packet[10], mac, 6);                                                   //set mac
  memcpy(&packet[16], mac, 6);                                                   //set mac
  packet[37] = nameSize;                                                         // set name size
  if (cd1s) {
    packet[32] = 0xe8; 
    packet[33] = 0x03; 
  } else {
    packet[32] = 0x64; 
    packet[33] = 0x00; 
  }
  if (wpa2) packet[34] = 0x31;                                                   //set wpa enabled
  else      packet[34] = 0x21;                                                   //set wpa disabled
  memcpy(&packet[((hashTagIndex > -1 && hashTagIndexBackwards) ? 42 : 38)], &name[0], nameSize - (hashTagIndex == -1 ? 0 : 4));        // set name
  if (hashTagIndex > -1) {
    if (hashTagIndexBackwards) {
      packet[38 + 0] = substrings[hashTagIndex];
      packet[38 + 1] = substrings[hashTagIndex + 1];
      packet[38 + 2] = '#';
      packet[38 + 3] = ' ';
    } else {
      packet[38 + nameSize - 4] = ' ';
      packet[38 + nameSize - 3] = '#';
      packet[38 + nameSize - 2] = substrings[hashTagIndex];
      packet[38 + nameSize - 1] = substrings[hashTagIndex + 1];
    }
  }
  memcpy(&packet[38 + nameSize], &beaconPacketTemplate[70], wpa2 ? 39 : 13);     // set back part
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


bool blinkNow(uint16_t timeScale) {
  return (millis() % timeScale) > (timeScale / 2);
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