#ifndef DISK_H
#define DISK_H

#include "mod_class.h"
#include "common_utilities.h"
#include <vector>

// 0x___0 - 0x___F
#define FILEKEY      0xFFF0
#define FILENAME     0xFFF1
#define FILEBODY     0xFFF2
#define FILEBODYSIZE 0xFFF3
#define FILETYPE     0xFFF4
#define FILEPARENTID 0xFFF5
#define FILEPARTID   0xFFF6
#define FILEEND      0xFFFF

#define DISKKEY      0xDDD0
#define DISKNAME     0xDDD1
#define DISKBODY     0xDDD2

#define PARTKEY      0xAAA0
#define PARTNAME     0xAAA1
#define PARTID       0xAAA3
#define PARTEND      0xAAAF

#define EMPTYKEY     0xEEE0

#define FTPARRAYMAP  0xFF0F
#define FTPICTURE    0xFF1F
#define FTBYTEARRAY  0xFF2F
#define FTBROKEN     0xFF3F
#define FTRECORD     0xFF4F

enum class FileType {   PARAMETER_MAP, BYTEARRAY, BROKEN, RECORD, PICTURE, UNKNOWN, EMPTY   };

class DFile {
public:
  DFile(uint16_t address, uint16_t bodyAddress, uint16_t bodySize, uint16_t end, FileType ft, uint16_t fileParent, uint16_t filePart) 
  : address(address), bodyAddress(bodyAddress), bodySize(bodySize), end(end), filetype(ft), fileParent(fileParent), filePart(filePart) {};
  
  void setNameFree(const char* nname) {
    name = nname;
  };

  virtual const char* getName() { return name; }
  uint16_t getParentPArtitionID() { return filePart; };
private:
  FileType filetype;
  uint16_t address, bodyAddress, bodySize, end, fileParent, filePart;
  const char* name = nullptr;
  uint8_t* body = nullptr;
  bool keepInRam = false;
};

class ParameterMapFile : public DFile {
public:
  ParameterMapFile(uint16_t address, uint16_t bodyAddress, uint16_t bodySize, uint16_t endAddress, uint16_t fileParent, uint16_t filePart) 
  : DFile(address, bodyAddress, bodySize, endAddress, FileType::PARAMETER_MAP, fileParent, filePart) {};
};

class ByteArrayFile : public DFile {
public:
  ByteArrayFile(uint16_t address, uint16_t bodyAddress, uint16_t bodySize, uint16_t endAddress, uint16_t fileParent, uint16_t filePart) 
  : DFile(address, bodyAddress, bodySize, endAddress, FileType::BYTEARRAY, fileParent, filePart) {};
};

class RecordFile : public DFile {
public:
  RecordFile(uint16_t address, uint16_t bodyAddress, uint16_t bodySize, uint16_t endAddress, uint16_t fileParent, uint16_t filePart) 
  : DFile(address, bodyAddress, bodySize, endAddress, FileType::RECORD, fileParent, filePart) {};
};

class PictureFile : public DFile {
public:
  PictureFile(uint16_t address, uint16_t bodyAddress, uint16_t bodySize, uint16_t endAddress, uint16_t fileParent, uint16_t filePart) 
  : DFile(address, bodyAddress, bodySize, endAddress, FileType::PICTURE, fileParent, filePart) {};
};

class UnknownFile : public DFile {
public:
  UnknownFile(uint16_t address, uint16_t bodyAddress, uint16_t bodySize, uint16_t endAddress, uint16_t fileParent, uint16_t filePart) 
  : DFile(address, bodyAddress, bodySize, endAddress, FileType::UNKNOWN, fileParent, filePart) {};
};

class Empty : public DFile {
public:
  Empty(uint16_t address, uint16_t emptySize) : DFile(address, address+5, emptySize-5, address+emptySize, FileType::UNKNOWN, 0xFFFF, 0xFFFF) {};

  const char* getName() override { return "EMPTY"; };
};

class Partition {
public:
  Partition() {};
  uint16_t startAddr = 0x0000, ID = 0x0000, end = 0x0000;
  const char* name = nullptr;
};



enum class DiskModState {
  preparing, normal, noDiskScreen, createPartition
};

class Disk : public Mod {
public:
  Disk(AxiusSSD* axiusInstance, uint16_t ID) : Mod(axiusInstance, ID) {};
  void setup() override;
  void firsttick() override {

  };
  String getName() override { return "DISK"; };

  void tick() override;

  void readDisk();
private:
  const uint16_t startAddr = 20000;
  uint16_t diskBodyAddr = startAddr;
  char* diskName = "NONAME DISK";
  DiskModState state = DiskModState::preparing;
  uint8_t substate = 0, normalState = 0;
  std::vector<Partition*> partitions;
  std::vector<DFile*> files;

  uint16_t endAddr = 0, newPartSize = 0, memCursor = startAddr;

  uint8_t cursor, startpos, scursor, sstartpos;

  void callUser();
  void updateNormal();
  void updateNoDisk();
  void updateNoPart();
  void createDisk(uint16_t address, String name);
  void createPart(uint16_t address, uint16_t ID, String name);

  uint16_t readU16(uint16_t addr);
  void writeU16(uint16_t addr, uint16_t num16);
};

#endif