#include "mod_class.h"
#include "common_utilities.h"

void Mod::tick() {}
void Mod::firsttick() {}
void Mod::setup() {}
String Mod::getName() {return "mod.name";}

#include <AxiusSSD.h>

void About::setup() {}
void About::firsttick() {}
String About::getName() {return "About";}
void About::tick() {
  axius->updateScreen = true;
  axius->drawText(axius->deviceName, 0);
  axius->drawText("Powered by Axius", 1);
  axius->drawText("Version 5.2.0", 2);
  if (axius->clickZ()) axius->tomenu();
}