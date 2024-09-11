#include "Arduino.h"
#include "globalstructures.h"

void BeaconPacketConstructor::setProperties(String name, bool wpa2, int8_t hashTagIndex, uint8_t* mac, bool cd1s) {
  name.replace("{hashindex}", String(hashTagIndex));
  if (name.indexOf("{nohash}") >= 0) {
    name.replace("{nohash}", "");
    hashTagIndex = -1;
  }
  if (hashTagIndex == -1) nameSize = min(name.length(), (uint) 32);
  else                    nameSize = min(name.length() + 4, (uint) 32);
  //Serial.println(name+" "+String(nameSize));
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
  memcpy(&packet[38], &name[0], nameSize - (hashTagIndex == -1 ? 0 : 4));                                   // set name
  if (hashTagIndex > -1) {
    packet[38 + nameSize - 4] = ' ';
    packet[38 + nameSize - 3] = '#';
    packet[38 + nameSize - 2] = substrings[hashTagIndex];
    packet[38 + nameSize - 1] = substrings[hashTagIndex + 1];
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

String uint8ArrayToString(const uint8_t *array, size_t arraySize) {
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






const uint8_t ESPPL_DS_NO     = 0;
const uint8_t ESPPL_DS_TO     = 1;
const uint8_t ESPPL_DS_FROM   = 2;
const uint8_t ESPPL_DS_TOFROM = 3;

const uint8_t ESPPL_MANAGEMENT = 0;
const uint8_t ESPPL_CONTROL    = 1;
const uint8_t ESPPL_DATA       = 2;

const uint8_t ESPPL_MANAGEMENT_ASSOCIATION_REQUEST    = 0;
const uint8_t ESPPL_MANAGEMENT_ASSOCIATION_RESPONSE   = 1;
const uint8_t ESPPL_MANAGEMENT_REASSOCIATION_REQUEST  = 2;
const uint8_t ESPPL_MANAGEMENT_REASSOCIATION_RESPONSE = 3;
const uint8_t ESPPL_MANAGEMENT_PROBE_REQUEST          = 4;
const uint8_t ESPPL_MANAGEMENT_PROBE_RESPONSE         = 5;
const uint8_t ESPPL_MANAGEMENT_TIMMING_ADVERTISEMENT  = 6;
const uint8_t ESPPL_MANAGEMENT_RESERVED1              = 7;
const uint8_t ESPPL_MANAGEMENT_BEACON                 = 8;
const uint8_t ESPPL_MANAGEMENT_ATIM                   = 9;
const uint8_t ESPPL_MANAGEMENT_DISASSOCIATION         = 10;
const uint8_t ESPPL_MANAGEMENT_AUTHENTICATION         = 11;
const uint8_t ESPPL_MANAGEMENT_DEAUTHENTICATION       = 12;
const uint8_t ESPPL_MANAGEMENT_ACTION                 = 13;
const uint8_t ESPPL_MANAGEMENT_ACTION_NO_ACK          = 14;
const uint8_t ESPPL_MANAGEMENT_RESERVED2              = 15;

const uint8_t ESPPL_CONTROL_RESERVED1                 = 0;
const uint8_t ESPPL_CONTROL_RESERVED2                 = 1;
const uint8_t ESPPL_CONTROL_RESERVED3                 = 2;
const uint8_t ESPPL_CONTROL_RESERVED4                 = 3;
const uint8_t ESPPL_CONTROL_RESERVED5                 = 4;
const uint8_t ESPPL_CONTROL_RESERVED6                 = 5;
const uint8_t ESPPL_CONTROL_RESERVED7                 = 6;
const uint8_t ESPPL_CONTROL_CONTROL_WRAPPER           = 7;
const uint8_t ESPPL_CONTROL_BLOCK_ACK_REQUEST         = 8;
const uint8_t ESPPL_CONTROL_BLOCK_ACK                 = 9;
const uint8_t ESPPL_CONTROL_PS_POLL                   = 10;
const uint8_t ESPPL_CONTROL_RTS                       = 11;
const uint8_t ESPPL_CONTROL_CTS                       = 12;
const uint8_t ESPPL_CONTROL_ACK                       = 13;
const uint8_t ESPPL_CONTROL_CF_END                    = 14;
const uint8_t ESPPL_CONTROL_CF_END_CF_ACK             = 15;

const uint8_t ESPPL_DATA_DATA                         = 0;
const uint8_t ESPPL_DATA_DATA_CF_ACK                  = 1;
const uint8_t ESPPL_DATA_DATA_CF_POLL                 = 2;
const uint8_t ESPPL_DATA_DATA_CF_ACK_CF_POLL          = 3;
const uint8_t ESPPL_DATA_NULL                         = 4;
const uint8_t ESPPL_DATA_CF_ACK                       = 5;
const uint8_t ESPPL_DATA_CF_POLL                      = 6;
const uint8_t ESPPL_DATA_CF_ACK_CF_POLL               = 7;
const uint8_t ESPPL_DATA_QOS_DATA                     = 8;
const uint8_t ESPPL_DATA_QOS_DATA_CF_ACK              = 9;
const uint8_t ESPPL_DATA_QOS_DATA_CF_ACK_CF_POLL      = 10;
const uint8_t ESPPL_DATA_QOS_NULL                     = 11;
const uint8_t ESPPL_DATA_RESERVED1                    = 12;
const uint8_t ESPPL_DATA_QOS_CF_POLL                  = 13;
const uint8_t ESPPL_DATA_QOS_CF_ACK_CF_POLL           = 14;