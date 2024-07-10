#include "axius_link.h"
#include <AxiusSSD.h>

String AxiusLink::getName() {return "AxiusLink["+String(nearbyDevices.size())+"]";};

void AxiusLink::firsttick() {

};
void AxiusLink::setup() {
  finalBeaconPacket.senderId = AxiusSSD::instance->MEM.getParameterByte("deviceId", 255);
};
void AxiusLink::tick() {
  if (state == 0) {
    if (nearbyDevices.size() == 0) {
      if (AxiusSSD::instance->readok()) AxiusSSD::instance->tomenu();
      AxiusSSD::instance->drawText("there is nothing", 0);
      AxiusSSD::instance->drawText("near you", 1);

      AxiusSSD::instance->drawText("ok - leave", 4);
    } else {
      //do not copypaste this code
      if (scursor > nearbyDevices.size()) scursor = nearbyDevices.size();
      if (AxiusSSD::instance->readdwn()) {
        if (scursor < nearbyDevices.size()) {
          scursor++;
          if (scursor-sstartpos > 4) sstartpos++;
        }
      }
      if (AxiusSSD::instance->readup() && scursor > 0) {
        scursor--;
        if (scursor-sstartpos < 0) sstartpos--;
      }
      if (AxiusSSD::instance->readok()) {
        if (scursor == 0) AxiusSSD::instance->tomenu();
        else {
          pcursor = 0;
          pstartpos = 0;
          state = 1;
          ParametersRequestPacket parreq;
          parreq.requestFromDeviceId = nearbyDevices[scursor-1].id;
          sendPacket(&parreq);
        }
      }
      for (uint8_t i = sstartpos; i < sstartpos+5; i++) {
        if (i > nearbyDevices.size()) break;
        if (i == 0) AxiusSSD::instance->drawTextSelector("exit", i-sstartpos, i == scursor);
        else AxiusSSD::instance->drawTextSelector(nearbyDevices[i-1].stype, i-sstartpos, i == scursor);
      }
      //end
    }
  } else if (state == 1) {
    AxiusSSD::instance->drawText("loading parameters", 0);
    AxiusSSD::instance->drawText("ok - exit", 1);
    if (AxiusSSD::instance->readok()) {
      state = 0;
      AxiusSSD::instance->tomenu();
    }
  } else if (state == 2) {
    AxiusSSD::instance->drawText("selected "+nearbyDevices[scursor-1].stype, 0);
    if (pcursor > curparameters.size()) {
      pcursor = 0;
      pstartpos = 0;
    }
    if (AxiusSSD::instance->readdwn()) {
      if (pcursor < curparameters.size()) {
        pcursor++;
        if (pcursor-pstartpos > 3) pstartpos++;
      }
    }
    if (AxiusSSD::instance->readup() && pcursor > 0) {
      pcursor--;
      if (pcursor-pstartpos < 0) pstartpos--;
    }
    if (AxiusSSD::instance->readok()) {
      if (pcursor == 0) {
        AxiusSSD::instance->tomenu();
        state = 0;
      } else {
        ParameterClickPacket pcp;
        pcp.param = pcursor - 1;
        //Serial.println("clicked"+String(pcp.param));
        pcp.clickDeviceId = nearbyDevices[scursor-1].id;
        sendPacket(&pcp);
      }
    }
    for (uint8_t i = pstartpos; i < pstartpos+4; i++) {
      if (i > curparameters.size()) break;
      if (i == 0) AxiusSSD::instance->drawTextSelector("exit", i - pstartpos + 1, i == pcursor);
      else AxiusSSD::instance->drawTextSelector(curparameters[i-1], i - pstartpos + 1, i == pcursor);
    }
  }
};

void AxiusLink::supertick() {
  if (millis() - lastRequestTime > 1000) {
    lastRequestTime = millis();
    sendPacket(&finalBeaconPacket);
  }

  if (bufferedPackets.size() > 0 && ++ticksAfterLastPacket > 3) {
    ticksAfterLastPacket = 0;
    //Serial.println("supertick");
    //Serial.println(bufferedPackets.size());
    std::vector<uint8_t*> uniqueData;
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
      } else {
        delete[] element;
      }
    }
    
    bufferedPackets = std::move(uniqueData);

    //Serial.println(bufferedPackets.size());
    for (uint8_t i = 0; i < bufferedPackets.size(); i++) {
      uint8_t* packet = bufferedPackets[i];
      uint8_t id = packet[25];
      processPacket(id, packet);
    }

    for (uint8_t* packet : bufferedPackets) {
      delete[] packet;
    }

    bufferedPackets.clear();
  }

  if (nearbyDevices.size() > 0) {
    for (auto it = nearbyDevices.begin(); it != nearbyDevices.end();) {
      if (millis() - it->lastTimeSeen > 5000) {
        it = nearbyDevices.erase(it);
      } else ++it;
    }
  }
};

void AxiusLink::onPacket(uint8_t* frame) {
  bufferedPackets.push_back(frame);
};




void AxiusLink::processPacket(uint8_t id, uint8_t* packet) {
  if (id == BEACONREQUEST) {
    BeaconRequestPacket p;
    p.setData(packet);
    if (p.senderId == AxiusSSD::instance->MEM.getParameterByte("deviceId", 255)) return;
    BeaconResponsePacket rp;
    rp.responderId = AxiusSSD::instance->MEM.getParameterByte("deviceId", 255);
    rp.devtype = AxiusSSD::instance->deviceName;
    sendPacket(&rp);
  } else if (id == BEACONRESPONSE) {
    BeaconResponsePacket respp;
    respp.setData(packet);
    if (nearbyDevices.size() > 0) {
      for (auto dev = nearbyDevices.begin(); dev != nearbyDevices.end();) {
        if (dev->id == respp.responderId) {
          dev -> lastTimeSeen = millis();
          return;
        }
      }
    }
    Device d;
    d.id = respp.responderId;
    d.lastTimeSeen = millis();
    d.stype = respp.devtype;
    nearbyDevices.push_back(d);
  } else if (id == PARAMETERSREQUEST) {
    ParametersRequestPacket parreq;
    parreq.setData(packet);
    if (parreq.requestFromDeviceId == AxiusSSD::instance->MEM.getParameterByte("deviceId", 255)) {
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
    if (parupd.sendToDeviceId == AxiusSSD::instance->MEM.getParameterByte("deviceId", 255) && state != 0) {
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
    if (pcp.clickDeviceId == AxiusSSD::instance->MEM.getParameterByte("deviceId", 255)) {
      if (pcp.param == 0) AxiusSSD::instance->restart();
      else if (pcp.param == 1) AxiusSSD::instance->tomenu();
      else {
        ParametersUpdatePacket pup;
        pup.sendToDeviceId = packet[27];
        pup.totalParameterSize = parameters.size();
        pup.updatables[pcp.param] = "clicked!";
        sendPacket(&pup);
      }
    }
  } else {
    Serial.print("unknown packet: ");
    Serial.println(id);
  }
}

void AxiusLink::sendPacket(AxiusPacket* p) {
  uint8_t size = 28 + p->getSize();
  uint8_t* packet = new uint8_t[size];
  memcpy(&packet[0], &pseudoRealisticHeader, 28);
  packet[24] = size - 28; //set data size
  packet[25] = p->getType(); //set type
  uint8_t newid = random(254);
  if (newid == beforesentid) newid = 255;
  packet[26] = newid; //set packet id
  packet[27] = AxiusSSD::instance->MEM.getParameterByte("deviceId", 255); //device id
  p->getData(packet);
  for (uint8_t i = 0; i < 5; i++) {
    bool sended = AxiusSSD::instance->sendWifiFrame(packet, size);
    delay(5);
    if (sended) break;
  }
  beforesentid = newid;
  delete[] packet;
}