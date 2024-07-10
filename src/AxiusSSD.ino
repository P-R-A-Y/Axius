#include "AxiusSSD.h"
AxiusSSD axius;

int upb = D8, dwnb = D0, okb = D7;

void setup() {
  randomSeed(148854271337);
  Serial.begin(115200);
  Wire.setClock(400000);

  AxiusSSD::instance->setPSMRender(renderInPSM);
  AxiusSSD::instance->setLastPreparation(lastPreparation);
  AxiusSSD::instance->setIconApplyer(applyIcons);
  AxiusSSD::instance->setPSMAnimOverrideChecker(isPSMAnimationOverrided);
  AxiusSSD::instance->setIncomingPacketListener(onIncomingPacket);

  AxiusSSD::instance->begin("AXIUS TEST", MemoryChip::c16, 10000.0f);
}


void lastPreparation() {
  upb = D5, dwnb = D6;
}

void onIncomingPacket(esppl_frame_info *info) {
  
}

bool isPSMAnimationOverrided() {
  return false;
}

void renderInPSM() {
  
}

void loop() {
  updatebuttons();

  AxiusSSD::instance->tick();

  AxiusSSD::instance->endRender();
}

void applyIcons() {
  AxiusSSD::instance->additionalTextField = "test";
}

int voltage;
void updatebuttons() {
  voltage = digitalRead(okb);
  if (voltage == HIGH) {
    if (AxiusSSD::instance->oks == 0) AxiusSSD::instance->oks = 1;
  } else if (voltage == LOW) {
    if (AxiusSSD::instance->oks == 1) {
      AxiusSSD::instance->oks = 3;
      AxiusSSD::instance->lastActionTime = millis();
    }
  }
  
  if (!AxiusSSD::instance->FULLPREPARED) return;

  voltage = digitalRead(upb);
  if (voltage == HIGH) {
    if (AxiusSSD::instance->ups == 0) AxiusSSD::instance->ups = 1;
  } else if (voltage == LOW) {
    if (AxiusSSD::instance->ups == 1) {
      AxiusSSD::instance->ups = 3;
      AxiusSSD::instance->lastActionTime = millis();
    }
  }
  voltage = digitalRead(dwnb);
  if (voltage == HIGH) {
    if (AxiusSSD::instance->dwns == 0) AxiusSSD::instance->dwns = 1;
  } else if (voltage == LOW) {
    if (AxiusSSD::instance->dwns == 1) {
      AxiusSSD::instance->dwns = 3;
      AxiusSSD::instance->lastActionTime = millis();
    }
  }
}