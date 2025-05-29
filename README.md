
# Axius

Ядро для моих проектиков на esp8266/esp32 и с 128x64 OLED монохромками на чипах SSD1306 и SH110

## Пример raw-запуска сией библиотеки

```c++
# узнаем таким нехитрым путем сколько изначально свободной оператвики под кучу
uint32_t onBootFreeHeap = ESP.getFreeHeap(); 

#include <AxiusSSD.h>
AxiusSSD axius;


void setup() {
  randomSeed(148854271337);
  Serial.begin(115200);
  Wire.begin(D5, D6);
  Wire.setClock(400000);

  axius.setLockScreen(lockScreenRender);
  axius.setLastPreparation(lastPreparation);
  axius.setIconApplyer(applyIcons);
  axius.setLockScreenOverrideChecker(isLockScreenOverrided);
  axius.setIncomingPacketListener(onIncomingPacket);
  axius.setIncomingPayloadListener(onIncomingPayload);

  # устанавливаем имя устройства, чип памяти, время афк таймаута, пин с кнопкой ok, принцип считывания показаний о нажатии кнопки, и передаем изначальный размер кучи
  axius.begin("Test Device", MemoryChip::c256, 10000.0f, D0, true, onBootFreeHeap); 
}

void onIncomingPayload(float rssi, uint8_t sender, char* prefix, uint8_t payloadSize, uint8_t* payload) {
  # обработка пакетов полезной нагрузки используемых при общении аксиусов между собой
}

void lastPreparation() {
  # функция вызывающаяся после завершения подготовки всех режимов
  axius.setButtons(false, D0, D1, D2, false, true);
}

void onIncomingPacket(esppl_frame_info *info) {
  # сюда приходят все фреймы которые видит еспшка
}

bool isLockScreenOverrided() {
  # указывает на использование кастомного афк-экрана
  return true;
}

void lockScreenRender() {
  # что рендерится в кастомном афк-экране
  axius.showStatusBar = false;
  axius.drawText("AFK MODE", -1);
}

uint32_t lastSensorsCheck = 0;
void loop() {
  axius.tick();
  axius.endRender();
}

void applyIcons() {
  # вызывается между tick() и endRender()
  # самое время нарисовать свои иконки на верхней панели
}
```


## Демо

Так выглядит дефолтная программа на примере корпуса AxiusNetMTTv1. На данный момент я его уже разобрал и это единственная фотка которая от него осталась

![Image](images/preview.jpg)