#include "link.h"
#include <AxiusSSD.h>

String Link::getName() {
  return "Link["+String(nearbyDevices.size())+"]";
};

void Link::firsttick() {

};
void Link::setup() {
  finalBeaconPacket.broadcasterId = axius->MGR.getParameterByte("deviceId", 255);
  finalBeaconPacket.devtype = axius->deviceName;
};
void Link::tick() {
  axius->updateScreen = true;
  if (state == 0) {
    if (nearbyDevices.size() == 0) {
      if (axius->clickZ()) axius->tomenu();
      axius->drawText("there is nothing", 0);
      axius->drawText("near you", 1);

      axius->drawText("ok - leave", 4);
    } else {
      //do not copypaste this code
      if (scursor > nearbyDevices.size()) scursor = nearbyDevices.size();
      if (axius->clickY()) {
        if (scursor < nearbyDevices.size()) {
          scursor++;
          if (scursor-sstartpos > 6) sstartpos++;
        }
      }
      if (axius->clickX() && scursor > 0) {
        scursor--;
        if (scursor-sstartpos < 0) sstartpos--;
      }
      if (axius->clickZ()) {
        if (scursor == 0) axius->tomenu();
        else {
          pcursor = 0;
          pstartpos = 0;
          state = 1;
          ParametersRequestPacket parreq;
          parreq.requestFromDeviceId = nearbyDevices[scursor-1].id;
          sendPacket(&parreq);
        }
      }
      for (uint8_t i = sstartpos; i < sstartpos+7; i++) {
        if (i > nearbyDevices.size()) break;
        if (i == 0) axius->drawTextSelector("exit", i-sstartpos, i == scursor);
        else axius->drawTextSelector(String(nearbyDevices[i-1].ss)+" | "+nearbyDevices[i-1].stype, i-sstartpos, i == scursor);
      }
      //end
    }
  } else if (state == 1) {
    axius->drawText("loading parameters", 0);
    axius->drawText("ok - exit", 1);
    if (axius->clickZ()) {
      state = 0;
      axius->tomenu();
    }
  } else if (state == 2) {
    axius->drawText("selected "+nearbyDevices[scursor-1].stype, 0);
    if (pcursor > curparameters.size()) {
      pcursor = 0;
      pstartpos = 0;
    }
    if (axius->clickY()) {
      if (pcursor < curparameters.size()) {
        pcursor++;
        if (pcursor-pstartpos > 5) pstartpos++;
      }
    }
    if (axius->clickX() && pcursor > 0) {
      pcursor--;
      if (pcursor-pstartpos < 0) pstartpos--;
    }
    if (axius->clickZ()) {
      if (pcursor == 0) {
        axius->tomenu();
        state = 0;
      } else {
        ParameterClickPacket pcp;
        pcp.param = pcursor - 1;
        //Serial.println("clicked"+String(pcp.param));
        pcp.clickDeviceId = nearbyDevices[scursor-1].id;
        sendPacket(&pcp);
      }
    }
    for (uint8_t i = pstartpos; i < pstartpos+6; i++) {
      if (i > curparameters.size()) break;
      if (i == 0) axius->drawTextSelector("exit", i - pstartpos + 1, i == pcursor);
      else axius->drawTextSelector(curparameters[i-1], i - pstartpos + 1, i == pcursor);
    }
  }
};

void Link::supertick() {
  if (!axius->wifiInitialized) {
    if (bufferedPackets.size() > 0) {
      for (uint8_t* packet : bufferedPackets) { delete[] packet; }
      bufferedPackets.clear();
    }
  }
  if (axius->esppl_sniffing_enabled && millis() - lastRequestTime > 1000) {
    lastRequestTime = millis();
    sendPacket(&finalBeaconPacket);
  }
  if (bufferedPackets.size() > 0 && ++ticksAfterLastPacket > 3) {
    ticksAfterLastPacket = 0;
    std::vector<uint8_t*> uniqueData;
    std::vector<int> uniqueRssi;
    size_t dataSize = bufferedPackets.size();

    for (size_t i = 0; i < dataSize; ++i) {
      bool isDuplicate = false;
      uint8_t* element = bufferedPackets[i];
      for (size_t j = 0; j < uniqueData.size(); ++j) {
        if (element[26] == uniqueData[j][26] && element[27] == uniqueData[j][27]) {
          isDuplicate = true;
          break;
        }
      }
      if (!isDuplicate) {
        uniqueData.push_back(element);
        uniqueRssi.push_back(rssiPack[i]);
      } else {
        delete[] element;
      }
    }
    
    bufferedPackets = std::move(uniqueData);
    rssiPack = std::move(uniqueRssi);

    //Serial.println(bufferedPackets.size());
    for (uint8_t i = 0; i < bufferedPackets.size(); i++) {
      uint8_t* packet = bufferedPackets[i];
      processPacket(packet[25], packet, rssiPack[i]);
    }

    for (uint8_t* packet : bufferedPackets) {
      delete[] packet;
    }

    bufferedPackets.clear();
    rssiPack.clear();
  }

  if (nearbyDevices.size() > 0) {
    for (auto it = nearbyDevices.begin(); it != nearbyDevices.end();) {
      if (millis() - it->lastTimeSeen > 2500) {
        if ((state == 1 || state == 2) && nearbyDevices[scursor-1].id == it->id) state = 0;
        it = nearbyDevices.erase(it);
      } else ++it;
    }
  }
};

void Link::onPacket(uint8_t* frame, int rssi) {
  bufferedPackets.push_back(frame);
  rssiPack.push_back(rssi);
};




void Link::processPacket(uint8_t id, uint8_t* packet, int rssi) {
  if (id == BEACONREQUEST) {
    BeaconRequestPacket p;
    p.setData(packet);
    if (p.senderId == axius->MGR.getParameterByte("deviceId", 255)) return;
    BeaconResponsePacket rp;
    rp.responderId = axius->MGR.getParameterByte("deviceId", 255);
    rp.devtype = axius->deviceName;
    sendPacket(&rp);
    updateDevice(p.senderId, "old (id:"+String(p.senderId)+")", rssi);
  } else if (id == BEACONRESPONSE || id == BEACONBROADCAST) {
    String devtype;
    uint8_t devid;
    if (id == BEACONBROADCAST) {
      BeaconBroadcastPacket brp;
      brp.setData(packet);
      devtype = brp.devtype;
      devid = brp.broadcasterId;
    } else {
      BeaconResponsePacket respp;
      respp.setData(packet);
      devtype = respp.devtype;
      devid = respp.responderId;
    }
    updateDevice(devid, devtype, rssi);
  } else if (id == PARAMETERSREQUEST) {
    ParametersRequestPacket parreq;
    parreq.setData(packet);
    if (parreq.requestFromDeviceId == axius->MGR.getParameterByte("deviceId", 255)) {
      ParametersUpdatePacket parupd;
      parupd.sendToDeviceId = packet[27];
      parupd.totalParameterSize = parameters.size();
      for (uint8_t i = 0; i < parameters.size(); i++) {
        parupd.updatables[i] = parameters[i];
      }
      sendPacket(&parupd);
    }
  } else if (id == PARAMETERSUPDATE) {
    ParametersUpdatePacket parupd;
    parupd.setData(packet);
    if (parupd.sendToDeviceId == axius->MGR.getParameterByte("deviceId", 255) && state != 0) {
      if (state == 1) state = 2;
      if (curparameters.size() != parupd.totalParameterSize) { // refill parameters
        curparameters.clear();
        curparameters.resize(parupd.totalParameterSize);
        for (const auto& pair : parupd.updatables) {
          curparameters[pair.first] = pair.second;
        }
      } else { //update just a bunch
        for (const auto& pair : parupd.updatables) {
          //Serial.println("replacing at "+String(pair.first)+" with "+String(pair.second));
          curparameters[pair.first] = pair.second;
        }
      }
    }
  } else if (id == PARAMETERCLICK) {
    ParameterClickPacket pcp;
    pcp.setData(packet);
    if (pcp.clickDeviceId == axius->MGR.getParameterByte("deviceId", 255)) {
      if (pcp.param == 0) axius->restart();
      else if (pcp.param == 1) axius->tomenu();
      else {
        ParametersUpdatePacket pup;
        pup.sendToDeviceId = packet[27];
        pup.totalParameterSize = parameters.size();
        pup.updatables[pcp.param] = "clicked!";
        sendPacket(&pup);
      }
    }
  } else if (id == CUSTOMPAYLOAD) {
    CustomPayloadPacket cpp;
    cpp.setData(packet);
    if (cpp.targetDeviceID != axius->MGR.getParameterByte("deviceId", 255)) return;
    if (axius->hasIncomingPayloadListener)
      axius->onCustomPayloadReceive(rssi, packet[27], cpp.prefix, cpp.payloadSize, cpp.payload);
  } else {
    Serial.print("unknown packet: ");
    Serial.println(id);
    return;
  }
}

bool Link::sendPacket(AxiusPacket* p) {
  uint8_t size = 28 + p->getSize();
  uint8_t* packet = new uint8_t[size];
  memcpy_P(&packet[0], &pseudoRealisticHeader, 28);
  packet[24] = size - 28; //set data size
  packet[25] = p->getType(); //set type 
  uint8_t newid = random(254);
  if (newid == beforesentid) newid = 255;
  packet[26] = newid; //set packet id
  packet[27] = axius->MGR.getParameterByte("deviceId", 255); //device id
  p->getData(packet);
  bool sended = false;
  for (uint8_t i = 0; i < 5; i++) {
    sended = axius->sendWifiFrame(packet, size);
    delay(5);
    if (sended) break;
  }
  beforesentid = newid;
  delete[] packet;
  return sended;
}