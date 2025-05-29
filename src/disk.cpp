#include "AxiusSSD.h"
#include "disk.h"

void Disk::setup() {
  endAddr = axius->chip == MemoryChip::c256 ? 32768 : 2048;
};

void Disk::readDisk() {
  for (uint8_t i = 0; i < partitions.size(); i++) {
    Partition* part = partitions[i];
    delete part;
  }
  partitions.clear();
  uint16_t memCursor = startAddr;
  uint16_t curDiskKey = readU16(memCursor);
  memCursor += 2;

  if (curDiskKey != DISKKEY) {
    state = DiskModState::noDiskScreen;
    callUser();
    return;
  }
  /*                                                                                                                                                                                                                                                                                               
  .                  SI   si             key    SI      PART    SI     PART   SI  actua-    PART   SI  name    name      PART  SI
  diskkey     name   ZE   ze  data       body   ZE      KEY     ZE      ID    ZE  lly id    NAME   ZE  size   object     END   ZE
  [DD DD]    [DD D1  04   03  01 02 03] [DD D2  00    { AA A0   00 }  {AA A3  02  FA  BA}  {AA A1  04   03   FA BA CA} { AA AF 00 }                                                
   0  1       2  3   4    5   6  7  8    9  10  11      12 13   14     15 16  17  18  19    20 21  22   23   24 25 26    27 28 29
  
    FILE  SI     FILE    SI  si  name      FILE   SI  TYPE     PARENT  SI   parent    FILE   SI  part       BODY  SI          FILE   SI                FILE
    KEY   ZE     NAME    ZE  ze  object    TYPE   ZE  RECORD     ID    ZE     id      PART   ZE   id        SIZE  ZE _size    BODY   ZE  data---data   END
  ( FF F0 00 ) ( FF F1   03  02  09 09 )  (FF F4  02  FF 4F)  (FF F5   02   FA FA)   (FF F6  02  FA BA)    (FF F3 02 00 04)  (FF F2  00  FF FF FF FF) (FF FF)   ] 
    30 31 32     33 34   35  36  37 38     39 40  41  42 43    44 45   46   47 48     49 50  51  52 53      54 55 56 57 58    59 60  61  62 63 64 65   66 67
   .                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             */
  uint16_t diskBody = 0;

  while (true) { //disk reader loop
    uint16_t dkey = readU16(memCursor);
    memCursor += 2;
    uint8_t dObjectSize = axius->MGR.readEEPROM(memCursor++);
    
    if (dkey == DISKNAME) {
      uint8_t diskNameSize = axius->MGR.readEEPROM(memCursor);
      uint8_t* diskNameRaw = axius->MGR.readSeveralBytesEEPROM(memCursor+1, diskNameSize);
      diskName = (char*) diskNameRaw;
      memCursor += dObjectSize;
    } else if (dkey == DISKBODY) { // 9, 10
      if (dObjectSize != 0x00) break; // 11
      diskBody = memCursor + 1; // 12

      Partition* curPart = nullptr;
      bool processingFile = false, isFileFinalised = false;
      const char* fileName = nullptr;
      uint16_t fileAddress, fileSize, fileType, fileParent, filePart, fileBodyAddress;

      while (true) {
        uint16_t key = readU16(memCursor);
        memCursor += 2;
        uint8_t objectSize = axius->MGR.readEEPROM(memCursor++);

        if (key == PARTKEY) { // key 12 13
          if (objectSize != 0x00 || curPart != nullptr) break; // os 14

          curPart = new Partition();
          partitions.push_back(curPart);
          curPart->startAddr = memCursor - 4;

          //memCursor += pObjectSize; размер всегда нулевой
        } else if (key == PARTID) { // key 15 16
          if (objectSize != 0x02 || curPart == nullptr) break; // os 17

          curPart->ID = readU16(memCursor); // id 18 19

          memCursor += objectSize; // cursor 20
        } else if (key == PARTNAME) { // key 20 21
          uint8_t partNameSize = axius->MGR.readEEPROM(memCursor); // cursor = 23
          if (partNameSize + 1 != objectSize) break; // os 22
          uint8_t* partNameRaw = axius->MGR.readSeveralBytesEEPROM(memCursor+1, partNameSize); // cursor = 23, read on 24, 24 25 26
          curPart->name = (char*) partNameRaw;
          
          memCursor += objectSize; // cursor = 23 + 4 = 27
        } else if (key == PARTEND) { // key 27 28
          if (objectSize != 0x00 || curPart == nullptr) break; // os 29, cursor = 30

          curPart->end = memCursor;
          curPart = nullptr;
        } else if (key == FILEKEY) { // key 0 31
          if (objectSize != 0x00 || processingFile) return; // os 32
          processingFile = true;
        } else if (key = FILENAME) { // key 33 34
          if (fileName != nullptr || !processingFile) return;

          uint8_t fileNameSize = axius->MGR.readEEPROM(memCursor); // cursor = 36
          if (fileNameSize + 1 != objectSize) break; // os 35
          uint8_t* fileNameRaw = axius->MGR.readSeveralBytesEEPROM(memCursor+1, fileNameSize); //cursor = 36, read from 37,   37 38
          fileName = (char*) fileNameRaw;

          memCursor += objectSize; // cursor = 36 + 39
        } else if (key == FILETYPE) { // key 39 40
          if (objectSize != 0x02 || !processingFile) return; // os 41
          fileType = readU16(memCursor); // ft 42 43, cursor = 42
          memCursor += objectSize; // cursor = 42 + 2 = 44
        } else if (key == FILEPARENTID) { // key 44 45
          if (objectSize != 0x02 || !processingFile) return; // os 46
          fileParent = readU16(memCursor); // read 47 48
          memCursor += objectSize; // cursor = 47 + 2 = 49
        } else if (key == FILEPARTID) { // key 49 50
          if (objectSize != 0x02 || !processingFile) return; // os 51
          filePart = readU16(memCursor); // 52 53
          memCursor += objectSize; // cursor = 52 + 2 = 54
        } else if (key == FILEBODYSIZE) { // key 54 55
          if (objectSize != 0x02 || !processingFile) return; // os 56
          fileSize = readU16(memCursor); // 57 58
          memCursor += objectSize; // cursor = 57 + 2 = 59
        } else if (key == FILEBODY) { // key 59 60
          if (objectSize != 0x00 || !processingFile) return; // os 61
          fileBodyAddress = memCursor; // cursor = 62, 62

          uint16_t nextTag = readU16(memCursor + fileSize); // 62 + 4 = 66,   nexttag 66 67
          memCursor += fileSize; // cursor = 66
          if (nextTag == FILEEND) {
            isFileFinalised = true;
            memCursor += 2; // cursor 66 + tageSize 2 = 68,     cursor = 68
          } else isFileFinalised = false; // cursor = 66, on error

          processingFile = false;

          File* file;

          if (fileType == FTPARRAYMAP)      file = new ParameterMapFile(fileAddress, fileBodyAddress, fileSize, memCursor);
          else if (fileType == FTPICTURE)   file = new PictureFile(fileAddress, fileBodyAddress, fileSize, memCursor);
          else if (fileType == FTBYTEARRAY) file = new ByteArrayFile(fileAddress, fileBodyAddress, fileSize, memCursor);
          else if (fileType == FTBROKEN)    file = new File(fileAddress, fileBodyAddress, fileSize, memCursor, FileType::BROKEN);
          else if (fileType == FTRECORD)    file = new RecordFile(fileAddress, fileBodyAddress, fileSize, memCursor);
          else                              file = new UnknownFile(fileAddress, fileBodyAddress, fileSize, memCursor);

          if (fileName == nullptr) file.setNameFree("-NONAME-");
          else file.setNameFree(fileName);

          files.push_back(file);
        } else if (key == EMPTYKEY) {
          
        } else {
          if ((key < 0xFFF0 || key > 0xFFFF) && (key < 0xAAA0 || key > 0xAAAF)) {
            break;
          }
          memCursor += objectSize;
        }
      }
    } else {
      if (dkey < 0xDDD0 || dkey > 0xDDDF) {
        //memCursor += objectSize; //нужна ли вообще эта хуйня?????????
        break;
      }
      memCursor += dObjectSize;
    }
  }

  if (partitions.size() == 0) {
    callUser();
    state = DiskModState::createPartition;
    return;
  }
};

void Disk::updateNormal() {
  if (normalState == 0) {
    axius->drawTextMiddle("D \""+String(diskName)+"\" P("+String(partitions.size())+") ", 0);
    if (axius->clickY()) {
      if (cursor < partitions.size()) {
        cursor++;
        if (cursor-startpos > 5) startpos++;
      }
    }
    if (axius->clickX()) {
      if (cursor > 0) {
        cursor--;
        if (cursor-startpos < 0) startpos--;
      }
    }
    if (axius->clickZ()) {
      if (cursor == 0) {
        axius->tomenu();
      } else {
        normalState = 1;
        cursor--; //shift "back"*/
      }
    }
    for (uint8_t i = startpos; i < startpos + 6; i++) {
      if (i > partitions.size()) break;
      if (i == 0) axius->drawTextSelector("BACK", i-startpos+1, i == cursor);
      else        axius->drawTextSelector(
        String(partitions[i-1]->name)
        +"["+String(partitions[i-1]->startAddr)+"-"+String(partitions[i-1]->bodyEnd)+"]",
      i-startpos+1, i == cursor);
    }
  } else if (normalState == 1) {
    axius->drawTextMiddle("P \""+String(partitions[cursor]->name)+"\" P("+String(partitions[cursor]->files.size())+") ", 0);
    if (axius->clickY()) {
      if (scursor < partitions[cursor]->files.size()) {
        scursor++;
        if (scursor-sstartpos > 5) sstartpos++;
      }
    }
    if (axius->clickX()) {
      if (scursor > 0) {
        scursor--;
        if (scursor-sstartpos < 0) sstartpos--;
      }
    }
    if (axius->clickZ()) {
      if (scursor == 0) {
        normalState = 0;
        scursor = 0;
        sstartpos = 0;
      } else {
        /*substate = 1;
        scursor--; //shift "back"*/
      }
    }
    for (uint8_t i = sstartpos; i < sstartpos + 6; i++) {
      if (i > partitions[cursor]->files.size()) break;
      if (i == 0) axius->drawTextSelector("BACK", i-sstartpos+1, i == scursor);
      else        axius->drawTextSelector(String(partitions[cursor]->files[i]->getName()), i-sstartpos+1, i == scursor);
    }
  }
}

void Disk::callUser() {
  axius->tryForceSwitchToMod(getName());
};

uint16_t Disk::readU16(uint16_t addr) {
  uint8_t lowb = axius->MGR.readEEPROM(addr+1);
  uint8_t highb = axius->MGR.readEEPROM(addr);
  return (highb << 8) | lowb;
};

void Disk::writeU16(uint16_t addr, uint16_t num16) {
  uint8_t highb = (num16 >> 8) & 0xFF;
  uint8_t lowb = num16 & 0xFF;
  axius->MGR.writeEEPROM(addr+1, lowb);
  axius->MGR.writeEEPROM(addr, highb);
};

void Disk::updateNoDisk() {
  if (substate == 0) {
    axius->drawTextMiddle("No disks found", 1);
    axius->drawTextMiddle("[Z] - Create new", 2);

    if (axius->clickZ()) {
      writeU16(startAddr, DISKKEY);
      axius->enableKeyboard("Creating new disk", "Enter new disk name:", 20);
      substate = 1;
    }
  } else if (substate == 1) {
    axius->keyboardRender();
    if (axius->isKeyboardNext()) {
      writeU16(startAddr+2, DISKNAME);
      axius->MGR.writeEEPROM(startAddr+4, axius->keyboardResult().length());
      axius->MGR.writeSeveralBytesEEPROM(startAddr+5, (uint8_t*)axius->keyboardResult().c_str(), axius->keyboardResult().length());
      state = DiskModState::preparing;
      diskBodyAddr = startAddr+5+axius->keyboardResult().length();
    }
  }
};


void Disk::updateNoPart() {
  if (substate == 0) {
    axius->drawTextMiddle("No partitions found", 1);
    axius->drawTextMiddle("[Z] - Create new", 2);

    if (axius->clickZ()) {
      axius->enableKeyboard("Creating new partition", "Enter new part name:", 20);
      substate = 1;
    }
  } else if (substate == 1) {
    axius->keyboardRender();
    if (axius->isKeyboardNext()) {
      /*
      writeU16(diskBodyAddr, PARTKEY);
      writeU16(diskBodyAddr+2, PARTID);
      writeU16(diskBodyAddr+4, random(0xFFFF));      
      writeU16(diskBodyAddr+6, PARTNAME);
      axius->MGR.writeEEPROM(diskBodyAddr+8, axius->keyboardResult().length());
      axius->MGR.writeSeveralBytesEEPROM(diskBodyAddr+9, (uint8_t*)axius->keyboardResult().c_str(), axius->keyboardResult().length());
      writeU16(diskBodyAddr+9+axius->keyboardResult().length(), PARTBODY);
      state = DiskModState::preparing;*/
      substate = 2;
      newPartSize = 128;
    }
  } else if (substate == 2) {
    axius->drawTextMiddle("Select the size", 0);
    axius->drawTextMiddle("of the partition", 1);

    if (axius->clickX()) {
      //if (newPartSize ) TODO
    }
  }
};

void Disk::tick() {
  axius->updateScreen = true;
  if (state == DiskModState::preparing) {
    state = DiskModState::normal;
    substate = 0;
    readDisk();
  } else if (state == DiskModState::normal) {
    updateNormal();
  } else if (state == DiskModState::noDiskScreen) {
    updateNoDisk();
  } else if (state == DiskModState::createPartition) {
    updateNoPart();
  }
};