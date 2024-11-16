#define BEACONREQUEST 1
#define BEACONRESPONSE 2
#define PARAMETERSREQUEST 3
#define PARAMETERSUPDATE 4
#define PARAMETERCLICK 5
#define BEACONBROADCAST 6
#define CUSTOMPAYLOAD 7

#ifndef AXIUSPACKETS_H
#define AXIUSPACKETS_H

#include <map>
#include "globalstructures.h"

class AxiusPacket {
  public:
    virtual ~AxiusPacket() = default;
    virtual void setData(uint8_t* buffer) = 0;
    virtual void getData(uint8_t* targetBuffer) = 0;
    virtual uint8_t getSize() = 0;
    virtual uint8_t getType() = 0;
};

class BeaconRequestPacket : public AxiusPacket {
  public:
    uint8_t senderId;
    
    void setData(uint8_t* PacketBuffer) override {
      senderId = PacketBuffer[28];
    }

    void getData(uint8_t* targetBuffer) override {
      targetBuffer[28] = senderId;
    }

    uint8_t getSize() override {
      return 1;
    }

    uint8_t getType() override {
      return BEACONREQUEST;
    }
};

class BeaconResponsePacket : public AxiusPacket {
  public:
    uint8_t responderId;
    String devtype;

    void setData(uint8_t* PacketBuffer) override {
      responderId = PacketBuffer[28];
      uint8_t nameLength = PacketBuffer[29];
      uint8_t bytename[nameLength];
      memcpy(&bytename, &PacketBuffer[30], nameLength);
      devtype = uint8ArrayToString(bytename, nameLength);
    }

    void getData(uint8_t* targetBuffer) override {
      targetBuffer[28] = responderId;
      targetBuffer[29] = devtype.length();

      uint8_t namebytes[devtype.length()];
      stringToUint8Array(devtype, namebytes, devtype.length());
      memcpy(&targetBuffer[30], namebytes, devtype.length());
    }

    uint8_t getSize() override {
      return 2 + devtype.length();
    }
    
    uint8_t getType() override {
      return BEACONRESPONSE;
    }
};

class BeaconBroadcastPacket : public AxiusPacket {
  public:
    uint8_t broadcasterId;
    String devtype;

    void setData(uint8_t* PacketBuffer) override {
      broadcasterId = PacketBuffer[28];
      uint8_t nameLength = PacketBuffer[29];
      uint8_t bytename[nameLength];
      memcpy(&bytename, &PacketBuffer[30], nameLength);
      devtype = uint8ArrayToString(bytename, nameLength);
    }

    void getData(uint8_t* targetBuffer) override {
      targetBuffer[28] = broadcasterId;
      targetBuffer[29] = devtype.length();

      uint8_t namebytes[devtype.length()];
      stringToUint8Array(devtype, namebytes, devtype.length());
      memcpy(&targetBuffer[30], namebytes, devtype.length());
    }

    uint8_t getSize() override {
      return 2 + devtype.length();
    }
    
    uint8_t getType() override {
      return BEACONBROADCAST;
    }
};

class ParametersRequestPacket : public AxiusPacket {
  public:
    uint8_t requestFromDeviceId;

    void setData(uint8_t* PacketBuffer) override {
      requestFromDeviceId = PacketBuffer[28];
    }

    void getData(uint8_t* targetBuffer) override {
      targetBuffer[28] = requestFromDeviceId;
    }

    uint8_t getSize() override {
      return 1;
    }
    
    uint8_t getType() override {
      return PARAMETERSREQUEST;
    }
};


class ParametersUpdatePacket : public AxiusPacket {
  public:
    uint8_t sendToDeviceId;
    uint8_t totalParameterSize;
    std::map<uint8_t, String> updatables;

    void setData(uint8_t* packetBuffer) override {
      uint8_t index = 28;
      sendToDeviceId = packetBuffer[index++];
      totalParameterSize = packetBuffer[index++];
      uint8_t updsize = packetBuffer[index++];
      for (uint8_t i = 0; i < updsize; i++) {
        uint8_t parindex = packetBuffer[index++];
        uint8_t nameLength = packetBuffer[index++];
        uint8_t bytename[nameLength];
        memcpy(&bytename, &packetBuffer[index], nameLength);
        String name = uint8ArrayToString(bytename, nameLength);
        index += nameLength;
        updatables[parindex] = name;
      }
    }

    void getData(uint8_t* targetBuffer) override {
      uint16_t index = 28;
      targetBuffer[index++] = sendToDeviceId;
      targetBuffer[index++] = totalParameterSize;
      targetBuffer[index++] = updatables.size();
      for (const auto& pair : updatables) {
        if (index + 2 + pair.second.length() > 255) break;
        targetBuffer[index++] = pair.first;
        targetBuffer[index++] = pair.second.length();

        uint8_t namebytes[pair.second.length()];
        stringToUint8Array(pair.second, namebytes, pair.second.length());
        memcpy(&targetBuffer[index], namebytes, pair.second.length());
        index += pair.second.length();
      }

    }

    uint8_t getSize() override {
      uint8_t cursor = 3; //yk
      for (const auto& pair : updatables) {
        cursor += 2 + pair.second.length(); //index + name size + name
      }
      return cursor;
    }
    
    uint8_t getType() override {
      return PARAMETERSUPDATE;
    }
};

class ParameterClickPacket : public AxiusPacket {
  public:
    uint8_t clickDeviceId, param;

    void setData(uint8_t* packetBuffer) override {
      clickDeviceId = packetBuffer[28];
      param = packetBuffer[29];
    }

    void getData(uint8_t* targetBuffer) override {
      targetBuffer[28] = clickDeviceId;
      targetBuffer[29] = param;
    }

    uint8_t getSize() override {
      return 2;
    }
    
    uint8_t getType() override {
      return PARAMETERCLICK;
    }
};

class CustomPayloadPacket : public AxiusPacket {
  public:
    uint8_t targetDeviceID;
    char prefix[6];
    uint8_t payloadSize;
    uint8_t* payload = nullptr;

    ~CustomPayloadPacket() {
      disposePayload();
    }

    void setData(uint8_t* packetBuffer) override {
      targetDeviceID = packetBuffer[28];
      readUint8ArrayToCharArray(packetBuffer, 29, prefix, 6);
      payloadSize = packetBuffer[35];
      if (payloadSize == 0) return;
      payload = new uint8_t[payloadSize];
      for (uint8_t i = 0; i < payloadSize; i++) {
        payload[i] = packetBuffer[36+i];
      }
    }

    void getData(uint8_t* targetBuffer) override {
      targetBuffer[28] = targetDeviceID;
      writeCharArrayToUint8Array(prefix, 6, targetBuffer, 29);
      targetBuffer[35] = payloadSize;
      if (payloadSize == 0) return;
      for (uint8_t i = 0; i < payloadSize; i++) {
        targetBuffer[36+i] = payload[i];
      }
      disposePayload();
    }

    void disposePayload() {
      if (payload != nullptr) {
        delete[] payload;
        payload = nullptr;
      }
    }

    uint8_t getSize() override {
      return 8 + payloadSize;
    }
    
    uint8_t getType() override {
      return CUSTOMPAYLOAD;
    }
};





#endif // AXIUSPACKETS_H