#include "manager_mod.h"
#include <AxiusSSD.h>

void ManagerMod::firsttick() {
  state = 0;
  cursor = 0;
  startpos = 0;
  checkmem();
  fixedOnParameter = false;
  //loadSavedData();
}

void ManagerMod::checkmem() {
  Wire.begin();
  Wire.beginTransmission(EEPROM_ADDR);
  error = Wire.endTransmission();
  if (error == 0) memoryWorking = true;
  else memoryWorking = false;
}

void ManagerMod::loadSavedData() {
  memcheckaddrmax = axius->chip == MemoryChip::c256 ? 32768 : 2048;
  curaddress = 10;
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

void ManagerMod::setup() {
  checkmem();
  loadSavedData();

  icon.setSize(13, 7);
  icon.setEnabled(true);
  icon.setPic(IMAGEBUFFER_1[1]);
  axius->addIcon(&icon);

  axius->setContrast(contrast.curval);
}

void ManagerMod::updateIcon() {
  if (memoryWorking) {
    uint8_t iconIndex = 0;
    if (millis() - lastMemRead  <= 500) iconIndex += 1;
    if (millis() - lastMemWrite <= 500) iconIndex += 2;

    switch (iconIndex) {
      case 0: //idle
        icon.setSize(13, 7);
        icon.setPic(IMAGEBUFFER_1[0]);
        break;
      case 1: //read
        icon.setSize(13, 7);
        icon.setPic(IMAGEBUFFER_1[2]);
        break;
      case 2: //write
        icon.setSize(13, 7);
        icon.setPic(IMAGEBUFFER_1[3]);
        break;
      case 3: //read + write
        icon.setSize(13, 9);
        icon.setPic(IMAGEBUFFER_1[4]);
        break;
    }
  } else {
    icon.setSize(13, 7);
    icon.setPic(IMAGEBUFFER_1[1]);
  }
}

void ManagerMod::tick() {
  axius->updateScreen = true;
  if (state == 0) {
    if (error == 4) {
      axius->drawText("Dead MEM module", 0);
      axius->drawText("OK - restart", 4);
      if (axius->clickZ()) axius->tomenu();
    } else { //if (error == 0 || ) {
      if (error == 0) axius->drawText("MEM module connected", 0);
      else axius->drawText("no MEM module", 0);
      axius->drawText("pick any option:", 1);
      if (axius->clickY()) {
        if (fixedOnParameter) {
          if (cursor == 6) {
            if (contrast.curval < 100) contrast.curval += 5;
          }
        } else if (cursor < statepick.size()-1) {
          cursor++;
          if (cursor-startpos > 4) startpos++;
        }
      }
      if (axius->clickX()) {
        if (fixedOnParameter) {
          if (cursor == 6) {
            if (contrast.curval > 0) contrast.curval -= 5;
          }
        } else if (cursor > 0) {
          cursor--;
          if (cursor-startpos < 0) startpos--;
        }
      }
      if (axius->clickZ()) {
        if (cursor == 0) axius->tomenu();
        else if (cursor == 2) axius->restart();
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
          setParameterBool("useWitchFont", !ucf.curval);
          axius->resetFont();
        } else if (cursor == 6) {
          if (fixedOnParameter) {
            setParameterByte("displayContrast", contrast.curval);
            axius->setContrast(contrast.curval);
          }
          fixedOnParameter = !fixedOnParameter;
        } else if (cursor == 7) {
          axius->display.dim(dim = !dim);
        } else if (cursor == 8) {
          state = 51;
          axius->enableKeyboard("Keyboard Test", "Type something:    (limit = 10)", 10);
        } else if (cursor == 9) {
          int* hs = heapStart - (uintptr_t)130000000;
          int* ptr = (int*)(random(heapEnd - hs) + hs);
          *ptr = random(256);
          String ns = "RAM rot: ";
          ns += String((uintptr_t)ptr, HEX);
          statepick[9] = ns;
        }
      }
      for (byte i = startpos; i < startpos + (statepick.size() < 5 ? statepick.size() : 5); i++) {
        if (i == 6)
          axius->drawTextSelectorWithBorder(statepick[i]+String(contrast.curval), i-startpos + 2, i == cursor, fixedOnParameter && i == cursor);
        else if (i == 10) 
          axius->drawTextSelectorWithBorder("used "+String(curaddress)+" bytes of "+String(memcheckaddrmax), i-startpos + 2, i == cursor, fixedOnParameter && i == cursor);
        else
          axius->drawTextSelector(statepick[i], i-startpos + 2, i == cursor);
      }
    }
  } else if (state == 50) {
    axius->drawText("Fatal memory error", 0);
    axius->drawText("OK - restart", 1);
    if (axius->clickZ()) axius->restart();
  } else if (state == 51) {
    axius->keyboardRender();
    if (axius->isKeyboardBack()) state = 0;
    if (axius->isKeyboardNext()) state = 0;
  } else if (state == 2) {
    axius->drawText("ok - exit", 0);
    axius->drawText("list of modules", 1);

    if (axius->clickZ()) state = 0;

    if (axius->clickY()) {
      if (mcursor < axius->modules.size()-1) {
        mcursor++;
        if (mcursor-mstartpos > 4) mstartpos++;
      }
    }
    if (axius->clickX()) {
      if (mcursor > 0) {
        mcursor--;
        if (mcursor-mstartpos < 0) mstartpos--;
      }
    }
    for (uint8_t i = mstartpos; i < mstartpos + (axius->modules.size() < 5 ? axius->modules.size() : 5); i++) {
      axius->drawTextSelector(axius->modules[i]->getName(), i-mstartpos + 2, i == mcursor);
    }
  } else if (state == 1) {
    axius->drawText("edit settings", 0);
    if (axius->clickY()) {
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
    if (axius->clickX()) {
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
    if (axius->clickZ()) {
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
          if (readEEPROM(settings[cursor]->address) != 0x00 && readEEPROM(settings[cursor]->address+1) != 0x00) {
            writeEEPROM(settings[cursor]->address, 0x00);
            writeEEPROM(settings[cursor]->address+1, 0x00);
            settings[cursor]->name = "undefined";
          } else {
            uint16_t maximizedSize = settings[cursor]->getBytesLength();
            uint8_t* sBuffer = new uint8_t[2];
            memcpy(&sBuffer[0], &maximizedSize, 2);
            writeSeveralBytesEEPROM(settings[cursor]->address, sBuffer, 2);
            delete[] sBuffer;
            loadSavedData();
          }
        }
      }
    }
    for (byte i = startpos; i < startpos + (settings.size() < 6 ? settings.size() : 6); i++) {
      if (fixedOnParameter) {
        if (i == cursor) axius->drawTextSelector(settings[i]->toString(), i-startpos + 1, true);
      } else {
        if (i == 0) axius->drawTextSelector("exit?", i-startpos + 1, i == cursor);
        else axius->drawTextSelector(settings[i]->toString(), i-startpos + 1, i == cursor);
      }
    }
  } else if (state == 5) {
    axius->drawText("testing eeprom cells...", 0);
    axius->drawText("check serial output", 1);
    axius->drawLoadingLine(memcheckaddr, memcheckaddrmax, 3);
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

bool ManagerMod::getParameterBool(const String &alias) {
  for (uint8_t i = 1; i < settings.size(); i++) {
    if (settings[i]->name.equals(alias)) {
      if (settings[i]->classType() == BOOLP) {
        BoolParameter *bp = (BoolParameter *) settings[i];
        return bp->curval;
      }
    }
  }
  Serial.println("Unknown BoolParameter "+alias);
  axius->restart();
  return true;
}

void ManagerMod::setParameterBool(const String &alias, const byte value) {
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
  axius->restart();
}

uint8_t ManagerMod::getParameterByte(const String &alias, const uint8_t maxvalue) {
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
  axius->restart();
  return 0;
}

void ManagerMod::setParameterByte(const String &alias, const uint8_t value) {
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
  axius->restart();
}

void ManagerMod::setParameterFloat(const String &alias, const float value) {
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
  axius->restart();
}

float ManagerMod::getParameterFloat(const String &alias) {
  for (uint8_t i = 1; i < settings.size(); i++) {
    if (settings[i]->name.equals(alias)) {
      if (settings[i]->classType() == FLOATP) {
        FloatParameter *bp = (FloatParameter *) settings[i];
        return bp->curval;
      }
    }
  }
  Serial.println("Unknown FloatParameter "+alias);
  axius->restart();
  return .0;
}

uint16_t ManagerMod::getAddresOfParameter(const String &alias) {
  for (uint8_t i = 1; i < settings.size(); i++) {
    if (settings[i]->name.startsWith(alias)) {
      return settings[i]->address;
    }
  }
  Serial.println("Unknown parameter "+alias);
  axius->restart();
  return 0;
}

uint16_t ManagerMod::getBytearraySize(const String &alias) {
  for (uint8_t i = 1; i < settings.size(); i++) {
    if (settings[i]->name.startsWith(alias)) {
      if (settings[i]->classType() == RMEMP) {
        ReservedMemory *rm = (ReservedMemory *) settings[i];
        return rm->getBytesLength();
      }
    }
  }
  Serial.println("Unknown Bytearray "+alias);
  axius->restart();
  return 0;
}

void ManagerMod::onMemoryError() {
  if (ignoreBrokenMemory) return;
  axius->toerror();
  state = 50;
}

//64 page size
//32768 max mem address

void ManagerMod::writeEEPROM(uint16_t address, uint8_t data) {
  if (data == readEEPROM(address)) return;

  if (axius->chip == MemoryChip::c256) {
    Wire.beginTransmission(EEPROM_ADDR);
    Wire.write((int)(address >> 8)); //   MSB
    Wire.write((int)(address & 0xFF)); // LSB
    Wire.write((int)data);
    Wire.endTransmission();
  } else if (axius->chip == MemoryChip::c16) {
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
  lastMemWrite = millis();
}

void ManagerMod::writeSeveralBytesEEPROM(uint16_t address, const uint8_t* data, uint16_t numbytes) { //unsafe   если надумаешь переписать этот метод то знай что макс. запись за раз - 30 байт + 2 байта адресация
  lastMemWrite = millis();
  for (uint16_t offset = 0; offset < numbytes; ++offset) {
    writeEEPROM(address+offset, data[offset]);
  }
}

uint8_t ManagerMod::readEEPROM(uint16_t address) {
  lastMemRead = millis();

  if (axius->chip == MemoryChip::c256) {
    Wire.beginTransmission(EEPROM_ADDR);
    Wire.write((int)(address >> 8)); // MSB
    Wire.write((int)(address & 0xFF)); // LSB
    Wire.endTransmission();
    Wire.requestFrom(EEPROM_ADDR, uint8_t(1));
  } else if (axius->chip == MemoryChip::c16) {
    const uint8_t i2c_address = EEPROM_ADDR | ((address >> 8) & 0x07);
    const uint8_t addr_ = address;
    Wire.beginTransmission(i2c_address);
    Wire.write(int(addr_));
    Wire.endTransmission();
    Wire.requestFrom(i2c_address, uint8_t(1));
  }

  if (Wire.available()) {
    uint8_t e = Wire.read();
    return e;
  } else {
    memoryWorking = false;
    onMemoryError();
    Serial.println("readEEPROM error at "+String(address));
    return 0x00;
  }
}

uint8_t* ManagerMod::readSeveralBytesEEPROM(uint16_t eeaddress, uint16_t numbytes) {
  uint8_t* buffer = new uint8_t[numbytes];
  for (uint8_t i = 0; i < numbytes; i++) {
    buffer[i] = readEEPROM(eeaddress + i);
  }
  return buffer;
}

void ManagerMod::readSeveralBytesEEPROM(uint16_t address, uint16_t numbytes, uint8_t *targetBuffer, uint16_t targetBufferOffset) {
  lastMemRead = millis();
  uint8_t i = 0;

  if (axius->chip == MemoryChip::c256) {
    Wire.beginTransmission(EEPROM_ADDR);
    Wire.write((int)(address >> 8));
    Wire.write((int)(address & 0xFF));
    Wire.endTransmission();
    Wire.requestFrom(uint16_t(EEPROM_ADDR), numbytes);
    for (i = 0; i < numbytes && Wire.available(); i++) {
      targetBuffer[i + targetBufferOffset] = Wire.read();
    }
  } else if (axius->chip == MemoryChip::c16) {
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