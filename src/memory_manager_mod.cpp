#include "memory_manager_mod.h"
#include "Arduino.h"
#include <memory>
#include <AxiusSSD.h>

MemoryManagerMod::MemoryManagerMod() {

}

void MemoryManagerMod::firsttick() {
  state = 0;
  cursor = 0;
  startpos = 0;
  checkmem();
  //loadSavedData();
}

void MemoryManagerMod::checkmem() {
  Wire.begin();
  Wire.beginTransmission(EEPROM_ADDR);
  error = Wire.endTransmission();
  if (error == 0) memoryWorking = true;
  else memoryWorking = false;
}

void MemoryManagerMod::loadSavedData() {
  uint16_t curaddress = 10;
  for (uint8_t i = 1; i < settings.size(); i++) {
    if (memoryWorking) {
      settings[i]->address = curaddress;
      curaddress += settings[i]->getBytesLength();

      if (settings[i]->classType() == BOOLP) {
        BoolParameter *bp = (BoolParameter *) settings[i];
        bp->curval = (readEEPROM(bp->address) == 50 ? false : true);
      } else if (settings[i]->classType() == BYTEP) {
        ByteParameter *bp = (ByteParameter *) settings[i];
        bp->curval = readEEPROM(bp->address);
      } else if (settings[i]->classType() == FLOATP) {
        FloatParameter *fp = (FloatParameter *) settings[i];
        uint8_t* byteBuffer = readSeveralBytesEEPROM(fp->address, 4);
        memcpy(&fp->curval, &byteBuffer[0], 4);
      } else if (settings[i]->classType() == PETP) {
        PetDedicatedMemory *pet = (PetDedicatedMemory *) settings[i];
        memcpy(&pet->size, &readSeveralBytesEEPROM(pet->address, 2)[0], 2);
        pet->broken = false;
        if (pet->size > pet->getBytesLength() - 2) {
          pet->size = pet->getBytesLength() - 2;
          Serial.println("bad pet size");
          pet->broken = true;
        }
        if (pet->size == 0) {
          pet->name = "Empty pet slot";
        } else {
          //pet->curval = readSeveralBytesEEPROM(pet->address, pet->size);
          pet->name = "pet size: "+String(pet->size)+"b";
          if (pet->broken) {
            pet->name = "broken pet slot "+String(pet->size)+"b";
          }
        }
      }
    }
  }
}

void MemoryManagerMod::setup() {
  checkmem();
  loadSavedData();
}

uint8_t MemoryManagerMod::getStateImage() {
  if (memoryWorking) {//read3 write4
    if (iconanimstate == 3 || iconanimstate == 4) {
      if (millis() - diskIconAnimTime > 1500) {
        diskIconAnimTime = 0;
        iconanimstate = 1;
      }
    }
    return iconanimstate;
  } else {
    return 2;
  }
}

void MemoryManagerMod::tick() {
  AxiusSSD::instance->updateScreen = true;
  if (state == 0) {
    if (error == 4) {
      AxiusSSD::instance->drawText("Dead MEM module", 0);
      AxiusSSD::instance->drawText("OK - restart", 4);
      if (AxiusSSD::instance->readok()) AxiusSSD::instance->tomenu();
    } else { //if (error == 0 || ) {
      if (error == 0) AxiusSSD::instance->drawText("MEM module connected", 0);
      else AxiusSSD::instance->drawText("no MEM module", 0);
      AxiusSSD::instance->drawText("pick any option:", 1);
      if (AxiusSSD::instance->readdwn()) {
        if (cursor < statepick.size()-1) {
          cursor++;
          if (cursor-startpos > 4) startpos++;
        }
      }
      if (AxiusSSD::instance->readup()) {
        if (cursor > 0) {
          cursor--;
          if (cursor-startpos < 0) startpos--;
        }
      }
      if (AxiusSSD::instance->readok()) {
        if (cursor == 0) AxiusSSD::instance->tomenu();
        else if (cursor == 2) AxiusSSD::instance->restart();
        else if (cursor == 1) {
          state = cursor;
          cursor = 0;
          startpos = 0;
        } else if (cursor == 3) {
          state = 5;
          ignoreBrokenMemory = true;
        } else if (cursor == 4) {
          state = 2;
        } else if (cursor == 5) {
          setParameterBool("useWithcFont", !uwf.curval);
          AxiusSSD::instance->resetFont();
        }
      }
      for (byte i = startpos; i < startpos + (statepick.size() < 5 ? statepick.size() : 5); i++) {
        AxiusSSD::instance->drawTextSelector(statepick[i], i-startpos + 2, i == cursor);
      }
    }
  } else if (state == 50) {
    AxiusSSD::instance->drawText("Fatal memory error", 0);
    AxiusSSD::instance->drawText("OK - restart", 1);
    if (AxiusSSD::instance->readok()) AxiusSSD::instance->restart();
  } else if (state == 2) {
    AxiusSSD::instance->drawText("ok - exit", 0);
    AxiusSSD::instance->drawText("list of modules", 1);

    if (AxiusSSD::instance->readok()) state = 0;

    if (AxiusSSD::instance->readdwn()) {
      if (mcursor < AxiusSSD::instance->modules.size()-1) {
        mcursor++;
        if (mcursor-mstartpos > 4) mstartpos++;
      }
    }
    if (AxiusSSD::instance->readup()) {
      if (mcursor > 0) {
        mcursor--;
        if (mcursor-mstartpos < 0) mstartpos--;
      }
    }
    for (uint8_t i = mstartpos; i < mstartpos + (AxiusSSD::instance->modules.size() < 5 ? AxiusSSD::instance->modules.size() : 5); i++) {
      AxiusSSD::instance->drawTextSelector(AxiusSSD::instance->modules[i]->getName(), i-mstartpos + 2, i == mcursor);
    }
  } else if (state == 1) {
    AxiusSSD::instance->drawText("edit settings", 0);
    if (AxiusSSD::instance->readdwn()) {
      if (fixedOnParameter) {
        if (settings[cursor]->classType() == BYTEP) {
          ByteParameter *bp = (ByteParameter *) settings[cursor];
          bp->curval--;
        }
      } else {
        if (cursor < settings.size()-1) {
          cursor++;
          if (cursor-startpos > 5) startpos++;
        }
      }
    }
    if (AxiusSSD::instance->readup()) {
      if (fixedOnParameter) {
        if (settings[cursor]->classType() == BYTEP) {
          ByteParameter *bp = (ByteParameter *) settings[cursor];
          bp->curval++;
        }
      } else {
        if (cursor > 0) {
          cursor--;
          if (cursor-startpos < 0) startpos--;
        }
      }
    }
    if (AxiusSSD::instance->readok()) {
      if (cursor == 0) {
        state = 0;
        cursor = 0;
        startpos = 0;
      } else {
        if (settings[cursor]->classType() == BOOLP) {
          BoolParameter *bp = (BoolParameter *) settings[cursor];
          bp->curval = !bp->curval;
          if (memoryWorking) writeEEPROM(bp->address, (bp->curval ? 51 : 50));
        } else if (settings[cursor]->classType() == BYTEP) {
          ByteParameter *bp = (ByteParameter *) settings[cursor];
          if (fixedOnParameter && memoryWorking) {
            writeEEPROM(bp->address, bp->curval);
          }
          fixedOnParameter = !fixedOnParameter;
        } else if (settings[cursor]->classType() == RMEMP) {
          settings[cursor]->name = "0. " + settings[cursor]->name;
          writeEEPROM(settings[cursor]->address, 0x00);
        } else if (settings[cursor]->classType() == PETP) {
          writeEEPROM(settings[cursor]->address, 0x00);
          writeEEPROM(settings[cursor]->address, 0x00);
          settings[cursor]->name = "undefined";
        }
      }
    }
    for (byte i = startpos; i < startpos + (settings.size() < 6 ? settings.size() : 6); i++) {
      if (fixedOnParameter) {
        if (i == cursor) AxiusSSD::instance->drawTextSelector(settings[i]->toString(), i-startpos + 1, true);
      } else {
        if (i == 0) AxiusSSD::instance->drawTextSelector("exit?", i-startpos + 1, i == cursor);
        else AxiusSSD::instance->drawTextSelector(settings[i]->toString(), i-startpos + 1, i == cursor);
      }
    }
  } else if (state == 5) {
    AxiusSSD::instance->drawText("testing eeprom cells...", 0);
    AxiusSSD::instance->drawText("check serial output", 1);
    AxiusSSD::instance->drawLoadingLine(memcheckaddr, memcheckaddrmax, 3);
    for (uint8_t i = 0; i < 100; i++) {
      if (memcheckaddr >= memcheckaddrmax) {
        ESP.restart();
        return;
      }

      uint8_t before = readEEPROM(memcheckaddr);
      writeEEPROM(memcheckaddr, before++);
      writeEEPROM(memcheckaddr, before);

      memcheckaddr++;
    }
  }
}

bool MemoryManagerMod::getParameterBool(const String &alias) {
  for (uint8_t i = 1; i < settings.size(); i++) {
    if (settings[i]->name.equals(alias)) {
      if (settings[i]->classType() == BOOLP) {
        BoolParameter *bp = (BoolParameter *) settings[i];
        return bp->curval;
      }
    }
  }
  Serial.println("Unknown BoolParameter "+alias);
  AxiusSSD::instance->restart();
  return true;
}

void MemoryManagerMod::setParameterBool(const String &alias, const byte value) {
  for (uint8_t i = 1; i < settings.size(); i++) {
    if (settings[i]->name.equals(alias)) {
      if (settings[i]->classType() == BOOLP) {
        BoolParameter *bp = (BoolParameter *) settings[i];
        bp->curval = value;
        if (memoryWorking) writeEEPROM(bp->address, (bp->curval ? 51 : 50));
        return;
      }
    }
  }
  Serial.println("Unknown BoolParameter "+alias);
  AxiusSSD::instance->restart();
}

uint8_t MemoryManagerMod::getParameterByte(const String &alias, const uint8_t maxvalue) {
  for (uint8_t i = 1; i < settings.size(); i++) {
    if (settings[i]->name.equals(alias)) {
      if (settings[i]->classType() == BYTEP) {
        ByteParameter *bp = (ByteParameter *) settings[i];
        if (bp->curval > maxvalue) {
          setParameterByte(alias, bp->defaultv);
        }
        return bp->curval;
      }
    }
  }
  Serial.println("Unknown ByteParameter "+alias);
  AxiusSSD::instance->restart();
  return 0;
}

void MemoryManagerMod::setParameterByte(const String &alias, const uint8_t value) {
  for (uint8_t i = 1; i < settings.size(); i++) {
    if (settings[i]->name.equals(alias)) {
      if (settings[i]->classType() == BYTEP) {
        ByteParameter *bp = (ByteParameter *) settings[i];
        bp->curval = value;
        if (memoryWorking) writeEEPROM(bp->address, value);
        return;
      }
    }
  }
  Serial.println("Unknown ByteParameter "+alias);
  AxiusSSD::instance->restart();
}

void MemoryManagerMod::setParameterFloat(const String &alias, const float value) {
  for (uint8_t i = 1; i < settings.size(); i++) {
    if (settings[i]->name.equals(alias)) {
      if (settings[i]->classType() == FLOATP) {
        FloatParameter *bp = (FloatParameter *) settings[i];
        bp->curval = value;
        if (memoryWorking) {
          uint8_t byteBuffer[4];
          memcpy(byteBuffer, &value, 4);
          writeSeveralBytesEEPROM(bp->address, byteBuffer, 4);
        }
        return;
      }
    }
  }
  Serial.println("Unknown FloatParameter "+alias);
  AxiusSSD::instance->restart();
}

float MemoryManagerMod::getParameterFloat(const String &alias) {
  for (uint8_t i = 1; i < settings.size(); i++) {
    if (settings[i]->name.equals(alias)) {
      if (settings[i]->classType() == FLOATP) {
        FloatParameter *bp = (FloatParameter *) settings[i];
        return bp->curval;
      }
    }
  }
  Serial.println("Unknown FloatParameter "+alias);
  AxiusSSD::instance->restart();
  return .0;
}

uint16_t MemoryManagerMod::getBytearrayAddres(const String &alias) {
  for (uint8_t i = 1; i < settings.size(); i++) {
    if (settings[i]->name.startsWith(alias)) {
      return settings[i]->address;
    }
  }
  Serial.println("Unknown Bytearray "+alias);
  AxiusSSD::instance->restart();
  return 0;
}

uint16_t MemoryManagerMod::getBytearraySize(const String &alias) {
  for (uint8_t i = 1; i < settings.size(); i++) {
    if (settings[i]->name.startsWith(alias)) {
      if (settings[i]->classType() == RMEMP) {
        ReservedMemory *rm = (ReservedMemory *) settings[i];
        return rm->getBytesLength();
      }
    }
  }
  Serial.println("Unknown Bytearray "+alias);
  AxiusSSD::instance->restart();
  return 0;
}

void MemoryManagerMod::onMemoryError() {
  if (ignoreBrokenMemory) return;
  AxiusSSD::instance->toerror();
  state = 50;
}

//64 page size
//32768 max mem address

void MemoryManagerMod::writeEEPROM(uint16_t address, uint8_t data) {
  if (data == readEEPROM(address)) return;
  //Serial.println("writing "+String(eeaddress)+" "+String(data));

  if (AxiusSSD::instance->chip == MemoryChip::c256) {
    Wire.beginTransmission(EEPROM_ADDR);
    Wire.write((int)(address >> 8)); //   MSB
    Wire.write((int)(address & 0xFF)); // LSB
    Wire.write((int)data);
    Wire.endTransmission();
  } else if (AxiusSSD::instance->chip == MemoryChip::c16) {
    const uint8_t i2c_address = EEPROM_ADDR | ((address >> 8) & 0x07);
    const uint8_t addr_ = address;
    Wire.beginTransmission(i2c_address);
    Wire.write(int(addr_));
    Wire.write(data);
    Wire.endTransmission();
  }
  delay(5);
  uint8_t result = readEEPROM(address);
  if (result != data) {
    memoryWorking = false;
    onMemoryError();
    Serial.println("writeEEPROM error (at: "+String(address)+" expected "+String(data)+", but got "+String(result));  
  }
  iconanimstate = 4;
  diskIconAnimTime = millis();
}

void MemoryManagerMod::writeSeveralBytesEEPROM(uint16_t address, const uint8_t* data, uint16_t numbytes) { //unsafe   если надумаешь переписать этот метод то знай что макс. запись за раз - 30 байт + 2 байта адресация
  iconanimstate = 4;
  diskIconAnimTime = millis();
  for (uint16_t offset = 0; offset < numbytes; ++offset) {
    writeEEPROM(address+offset, data[offset]);
  }
}

uint8_t MemoryManagerMod::readEEPROM(uint16_t address) {
  iconanimstate = 3;
  diskIconAnimTime = millis();

  if (AxiusSSD::instance->chip == MemoryChip::c256) {
    Wire.beginTransmission(EEPROM_ADDR);
    Wire.write((int)(address >> 8)); // MSB
    Wire.write((int)(address & 0xFF)); // LSB
    Wire.endTransmission();
    Wire.requestFrom(EEPROM_ADDR, uint8_t(1));
  } else if (AxiusSSD::instance->chip == MemoryChip::c16) {
    const uint8_t i2c_address = EEPROM_ADDR | ((address >> 8) & 0x07);
    const uint8_t addr_ = address;
    Wire.beginTransmission(i2c_address);
    Wire.write(int(addr_));
    Wire.endTransmission();
    Wire.requestFrom(i2c_address, uint8_t(1));
  }

  if (Wire.available()) {
    uint8_t e = Wire.read();
    //Serial.println("read available "+String(eeaddress)+" "+String(e));
    return e;
  } else {
    memoryWorking = false;
    onMemoryError();
    Serial.println("readEEPROM error at "+String(address));
    return 0x00;
  }
}

uint8_t* MemoryManagerMod::readSeveralBytesEEPROM(uint16_t eeaddress, uint16_t numbytes) {
  uint8_t* buffer = new uint8_t[numbytes];
  for (uint8_t i = 0; i < numbytes; i++) {
    buffer[i] = readEEPROM(eeaddress + i);
  }
  return buffer;
}

void MemoryManagerMod::readSeveralBytesEEPROM(uint16_t address, uint16_t numbytes, uint8_t *targetBuffer, uint16_t targetBufferOffset) {
  iconanimstate = 3;
  diskIconAnimTime = millis();
  //uint8_t* buffer = new uint8_t[numbytes];
  uint8_t i = 0;

  if (AxiusSSD::instance->chip == MemoryChip::c256) {
    Wire.beginTransmission(EEPROM_ADDR);
    Wire.write((int)(address >> 8)); // MSB
    Wire.write((int)(address & 0xFF)); // LSB
    Wire.endTransmission();
    Wire.requestFrom(uint16_t(EEPROM_ADDR), numbytes);
    for (i = 0; i < numbytes && Wire.available(); i++) {
      targetBuffer[i + targetBufferOffset] = Wire.read();
    }
  } else if (AxiusSSD::instance->chip == MemoryChip::c16) {
    const uint8_t devaddr = EEPROM_ADDR | ((address >> 8) & 0x07);
    const uint8_t addr = address;
    Wire.beginTransmission(devaddr);
    Wire.write(int(addr));
    Wire.endTransmission();
    delay(5);
    Wire.requestFrom(uint16_t(devaddr), numbytes);
    for (; i < numbytes && Wire.available(); i++) {
      targetBuffer[i + targetBufferOffset] = Wire.read();
    }
  }

  if (numbytes != i) {
    memoryWorking = false;
    onMemoryError();
    Serial.println("readEEPROM error. last address: "+String(address+i));
  }
}