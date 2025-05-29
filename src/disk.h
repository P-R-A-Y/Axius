#ifndef DISK_H
#define DISK_H

#include "mod_class.h"
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

enum class FileType {   PARAMETER_MAP, BYTEARRAY, BROKEN, RECORD, PICTURE, UNKNOWN   };

class File {
public:
  File(uint16_t address, uint16_t bodyAddress, uint16_t bodySize, uint16_t end, FileType ft) : address(address), bodyAddress(bodyAddress), bodySize(bodySize), end(end), filetype(ft) {};
  
  void setNameFree(const char* nname) {
    name = nname;
  };

  const char* getName() { return name; }
private:
  FileType filetype;
  uint16_t address, bodyAddress, bodySize, end;
  const char* name = nullptr;
  uint8_t* body = nullptr;
  bool keepInRam = false;
};

class ParameterMapFile : public File {
public:
  ParameterMapFile(uint16_t address, uint16_t bodyAddress, uint16_t bodySize, uint16_t endAddress) : File(address, bodyAddress, bodySize, endAddress, FileType::PARAMETER_MAP) {};
};

class ByteArrayFile : public File {
public:
  ByteArrayFile(uint16_t address, uint16_t bodyAddress, uint16_t bodySize, uint16_t endAddress) : File(address, bodyAddress, bodySize, endAddress, FileType::BYTEARRAY) {};
};

class RecordFile : public File {
public:
  RecordFile(uint16_t address, uint16_t bodyAddress, uint16_t bodySize, uint16_t endAddress) : File(address, bodyAddress, bodySize, endAddress, FileType::RECORD) {};
};

class PictureFile : public File {
public:
  PictureFile(uint16_t address, uint16_t bodyAddress, uint16_t bodySize, uint16_t endAddress) : File(address, bodyAddress, bodySize, endAddress, FileType::PICTURE) {};
};

class UnknownFile : public File {
public:
  UnknownFile(uint16_t address, uint16_t bodyAddress, uint16_t bodySize, uint16_t endAddress) : File(address, bodyAddress, bodySize, endAddress, FileType::UNKNOWN) {};
};

class Partition {
public:
  Partition() {};
  uint16_t startAddr = 0x0000, ID = 0x0000, end = 0x0000; // end - последний байт файла + 1
  const char* name = nullptr;
};

enum class DiskModState {
  preparing, normal, noDiskScreen, createPartition
};

class Disk : public Mod {
public:
  Disk(AxiusSSD* axiusInstance) : Mod(axiusInstance) {};
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
  std::vector<File*> files;

  uint16_t endAddr = 0, newPartSize = 0, memCursor = startAddr;

  uint8_t cursor, startpos, scursor, sstartpos;

  void callUser();
  void updateNormal();
  void updateNoDisk();
  void updateNoPart();

  uint16_t readU16(uint16_t addr);

  void writeU16(uint16_t addr, uint16_t num16);
};

#endif