#include <AxiusSSD.h>
#include "compile_parameters.h"

volatile int encoderPos = 0, beforeEncoderPos = 0;
volatile bool turned = false;
uint8_t scrollSteps = 0;  
uint32_t lastInterruptTime = 0;
const uint32_t debounceDelay = 3;
int scrollAlfa = 0, scrollBeta = 0;

#ifdef ESP32

extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
  if (arg == 31337) return 1;
  else return 0;
}

extern "C" {
#include "esp_wifi.h"
  esp_err_t esp_wifi_set_channelnel(uint8_t primary, wifi_second_chan_t second);
  esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq);
}

#else
extern "C" {
  #include "user_interface.h"
  typedef void (*freedom_outside_cb_t)(uint8 status);
  int  wifi_register_send_pkt_freedom_cb(freedom_outside_cb_t cb);
  void wifi_unregister_send_pkt_freedom_cb(void);
  int  wifi_send_pkt_freedom(uint8 *buf, int len, bool sys_seq);
}
#endif

const char* keyboardLetters[] PROGMEM = {
  "1","2","3","4","5","6","7","8","9","0",
  "q","w","e","r","t","y","u","i","o","p",
  "a","s","d","f","g","h","j","k","l","z",
  "x","c","v","b","n","m", " ", ",", ".", "<-", "back", "next"
};

AxiusSSD* AxiusSSD::instance = nullptr;
void AxiusSSD::begin(String devname, MemoryChip c, float nmaxAfkSeconds , const uint8_t defaultOKButton, bool isInvertButtonReadMethod, uint32_t onBootFreeHeapL) {
  onBootFreeHeap = onBootFreeHeapL;
  okb = defaultOKButton;
  if (isInvertButtonReadMethod) {
    invertButtonReadMethod = true;
    pinMode(okb, INPUT_PULLUP);
  }
  MGR.checkmem();
  Serial.println(MGR.readEEPROM(4));
  if (MGR.readEEPROM(4) == 0x42) {
    int volt = digitalRead(okb);
    bool disable = false;
    if (invertButtonReadMethod) {
      if (volt == LOW) disable = true;
    } else {
      if (volt == HIGH) disable = true;
    }
    if (disable) {
      MGR.writeEEPROM(4, 0x00);
    } else {
      link.sendSomeElectronsIntoTheAir(devname, 200);
      ESP.deepSleep(1e6 * 5);
      return;
    }
  }
  
  deviceName = devname;
  chip = c;
  maxAfkSeconds = nmaxAfkSeconds;
#ifdef USE_SH110X
  while (!display.begin(0x3C, true)) {
#else
  while (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
#endif
    Serial.println(F("screen allocation failed"));
    delay(1000);
  }
  display.clearDisplay();
  display.setFont(&SuperSmallFont);

  for (uint8_t i = 0; i < modules.size(); i++) {
    modules[i]->connect();
  }
  
  mods.insert(mods.begin(), &link);
  mods.insert(mods.begin(), &MGR);
  mods.push_back(&about);
  mods.push_back(&disk);

  addIcon(&wifiIcon);
  wifiIcon.setSize(9, 10);
  wifiIcon.setPic(wifiListeningDisabled);
  wifiIcon.setEnabled(true);

  instance = this;
}

void AxiusSSD::drawTextSelector(String text, uint8_t row, bool isselected) {
  if (!updateScreen) return;
  uint8_t y = row*rowHeight+columnTopShift;
  if (isselected)
    display.fillCircle(2, y-2, 2, WHITE);
  else
    display.drawCircle(2, y-2, 2, WHITE);
  display.setCursor(6,y);
  display.println(text);
}

void AxiusSSD::drawTextWithBorder(String text, uint8_t row, bool border) {
  if (!updateScreen) return;
  uint8_t y = row*rowHeight+columnTopShift;
  
  if (border) display.fillRect(0, y-rowHeight+1, 128, rowHeight+1, WHITE);

  display.setTextColor(border ? BLACK : WHITE);
  display.setCursor(1,y);
  display.println(text);
  display.setTextColor(WHITE);
}

void AxiusSSD::drawTextSelectorWithBorder(String text, uint8_t row, bool isselected, bool border) {
  if (!updateScreen) return;
  uint8_t y = row*rowHeight+columnTopShift;
  if (isselected)
    display.fillCircle(3, y-2, 2, WHITE);
  else
    display.drawCircle(3, y-2, 2, WHITE);
  
  if (border) display.fillRect(8, y-rowHeight+1, 122, rowHeight+1, WHITE);

  display.setTextColor(border ? BLACK : WHITE);
  display.setCursor(9,y);
  display.println(text);
  display.setTextColor(WHITE);
}

void AxiusSSD::drawLoadingLine(float cur, float max, uint8_t row) {
  if (!updateScreen) return;
  uint8_t miny = row*rowHeight+columnTopShift;
  
  display.drawRect(llminx, miny-rowHeight+2, llwidth, rowHeight-1, WHITE);
  display.setCursor(llwidth+8, miny);
  float norm = normalize(0.0, max, cur);
  display.println(String((uint16_t)(norm*100))+"%");
  if (cur == 0) return;
  display.fillRect(llminx, miny-rowHeight+2, norm * llwidth, rowHeight-1, WHITE);
}

void AxiusSSD::drawText(String text, uint8_t row) {
  if (!updateScreen) return;
  uint8_t y = row*rowHeight+columnTopShift;
  display.setCursor(1,y);
  display.println(text);
}

void AxiusSSD::drawTextMiddle(String text, uint8_t row) {
  if (!updateScreen) return;
  uint8_t y = row*rowHeight+columnTopShift;
  uint16_t width = getTextWidth(text);
  display.setCursor(128/2-width/2, y);
  display.println(text);
}

void AxiusSSD::setLockScreen(void (*func)()) {
  renderInPSM = func;
}

void AxiusSSD::setLastPreparation(void (*func)()) {
  onLastPreparation = func;
}

void AxiusSSD::setIconApplyer(void (*func)()) {
  applyIcons = func;
}

void AxiusSSD::setLockScreenOverrideChecker(bool (*func)()) {
  isPSMAnimationOverrided = func;
}

void AxiusSSD::setIncomingPacketListener(void (*func)(esppl_frame_info *info)) {
  onIncomingPacket = func;
}

void AxiusSSD::updateStatusBar() {
  if (!showStatusBar) return;
  uint8_t startX = 0;//34;

  if (!disableLogo) {
    startX += 12;
    
    if (millis() - lastLAFChange > 400) {
      lastLAFChange = millis();
      logoanim1stframe = !logoanim1stframe;
    }
    display.drawBitmap(0, 1, (logoanim1stframe ? logoanim1 : logoanim2), 11, 11, WHITE);
  }

  if (!minimalStatusBar) {
    if (onBootFreeHeap != 1) {
      float delta = 1.0 - (float)ESP.getFreeHeap() / onBootFreeHeap;
      display.drawRect(106, 0, 22, 11, WHITE);
      const float totalPoints = 20 * 9;
      uint16_t points = ceil(totalPoints * delta);
      for (uint8_t x = 0; x <= 19; x++) {
        for (uint8_t y = 0; y <= 8; y++) {
          if (points == 0.0) break; 
          display.drawPixel(107 + x, 1 + 8 - y, WHITE);
          points--;
        }
      }
    }

    if (state == State::menu) {
      startX += afkBarWidth + 2;

      display.drawRect(13, 2, afkBarWidth, 7, WHITE);
      if (millis() - lastActionTime < maxAfkSeconds) {
        int16_t ww = afkBarWidth - ((millis() - lastActionTime) / maxAfkSeconds * afkBarWidth);
        display.fillRect(13, 2, ww, 7, WHITE);
      }
    }
  }

  const uint8_t midY = 11 / 2;
  MGR.updateIcon();

  if (esppl_sniffing_enabled) wifiIcon.setPic(wifiListeningEnabled);
  else wifiIcon.setPic(wifiListeningDisabled);

  applyIcons();
  for (IconProvider* icon : icons) {
    if (icon->getEnabled()) {
      icon->render(&display, startX, uint8_t(ceil(midY - (icon->getHeight() / 2.0))));
      startX += icon->getWidth() + 1;
    }
  }
}

void AxiusSSD::setButtons(bool isUsingEncoder, const uint8_t UPButtonInput, const uint8_t DOWNButtonInput, const uint8_t OKButtonInput, bool isOkFromEncoder, bool isInvertButtonReadMethod) {
  invertButtonReadMethod = isInvertButtonReadMethod;
  if (isUsingEncoder) {
    useEncoder = true;
    scrollAlfa = UPButtonInput, scrollBeta = DOWNButtonInput;
    pinMode(UPButtonInput, INPUT_PULLUP);
    pinMode(DOWNButtonInput, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(UPButtonInput), readEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(DOWNButtonInput), readEncoder, CHANGE);
  } else {
    useEncoder = false;
    upb = UPButtonInput;
    dwnb = DOWNButtonInput;
    if (invertButtonReadMethod) {
      pinMode(upb, INPUT_PULLUP);
      pinMode(dwnb, INPUT_PULLUP);
    }
  }
  if (isOkFromEncoder) {
    //TODO
  } else {
    if (invertButtonReadMethod) {
      pinMode(okb, INPUT_PULLUP);
    }
    okb = OKButtonInput;
  }
}

void AxiusSSD::endRender() {
  if (updateScreen) {
    updateScreen = false;
    if (millis() - lastActionTime > maxAfkSeconds) {
      state = State::lockscreen;
    }

    updateStatusBar();
    display.display();
    display.clearDisplay();
  }

  link.supertick();
  resetButtons();
}

void ICACHE_RAM_ATTR readEncoder() {
  if (millis() - lastInterruptTime > debounceDelay) {
    static uint8_t lastState = 0;
    uint8_t scrollState = (digitalRead(scrollAlfa) << 1) | digitalRead(scrollBeta);
    
    if (lastState == scrollState) return;
    
    switch (lastState) {
      case 0: 
        if (scrollState == 2) encoderPos--;
        else if (scrollState == 1) encoderPos++;
        break;
      case 1: 
        if (scrollState == 0) encoderPos--;
        else if (scrollState == 3) encoderPos++;
        break;
      case 2: 
        if (scrollState == 3) encoderPos--;
        else if (scrollState == 0) encoderPos++;
        break;
      case 3: 
        if (scrollState == 1) encoderPos--;
        else if (scrollState == 2) encoderPos++;
        break;
    }
    
    lastState = scrollState;
    turned = true;
    scrollSteps++;
  }

  lastInterruptTime = millis();
}

int voltage;
void AxiusSSD::updateButtons() {
  voltage = digitalRead(okb);
  if (invertButtonReadMethod) {
    if (voltage == LOW) {
      if (oks == 0) {
        oks = 1;
        okClickTime = millis();
      } else if (oks == 1 && millis() - okClickTime > HOLDTIME) {
        oks = 4;
      }
    } else if (voltage == HIGH) {
      if (oks == 1 || oks == 4) {
        oks = 3;
        lastActionTime = millis();
      }
    }
  } else {
    if (voltage == HIGH) {
      if (oks == 0) oks = 1;
    } else if (voltage == LOW) {
      if (oks == 1) {
        oks = 3;
        lastActionTime = millis();
      }
    }
  }
  
  if (!ISREADY) return;

  if (useEncoder) {
    if (turned && scrollSteps >= 6) {
      turned = false;
      if (encoderPos != beforeEncoderPos) {
        if (encoderPos > beforeEncoderPos) {
          ups = 3;
        } else {
          dwns = 3;
        }
        lastActionTime = millis();
        beforeEncoderPos = encoderPos;
      }
      scrollSteps = 0;
    }
  } else {
    voltage = digitalRead(upb);
    if (voltage == LOW) {
      if (ups == 0) {
        ups = 1;
        upClickTime = millis();
      } else if (ups == 1 && millis() - upClickTime > HOLDTIME) {
        ups = 4;
      }
    } else if (voltage == HIGH) {
      if (ups == 1 || ups == 4) {
        ups = 3;
        lastActionTime = millis();
      }
    }

    voltage = digitalRead(dwnb);
    if (voltage == LOW) {
      if (dwns == 0) {
        dwns = 1;
        dwnClickTime = millis();
      } else if (dwns == 1 && millis() - dwnClickTime > HOLDTIME) {
        dwns = 4;
      }
    } else if (voltage == HIGH) {
      if (dwns == 1 || dwns == 4) {
        dwns = 3;
        lastActionTime = millis();
      }
    }
  }
}

bool AxiusSSD::clickX() {
  if (ups == 3) {
    ups = 0;
    return true;
  } else if (ups == 4 && millis() - upHoldCooldown > lerp(MIN_HOLD_COOLDOWN, MAX_HOLD_COOLDOWN, normalize(HOLDTIME, HOLDTIME+2000, millis() - upClickTime)) ) {
    upHoldCooldown = millis();
    return true;
  }
  return false;
}

bool AxiusSSD::clickY() {
  if (dwns == 3) {
    dwns = 0;
    return true;
  } else if (dwns == 4 && millis() - dwnHoldCooldown > lerp(MIN_HOLD_COOLDOWN, MAX_HOLD_COOLDOWN, normalize(HOLDTIME, HOLDTIME+2000, millis() - dwnClickTime)) ) {
    dwnHoldCooldown = millis();
    return true;
  }
  return false;
}

bool AxiusSSD::clickZ() {
  if (oks == 3) {
    oks = 0;
    return true;
  } else if (oks == 4 && millis() - okHoldCooldown > lerp(MIN_HOLD_COOLDOWN, MAX_HOLD_COOLDOWN, normalize(HOLDTIME, HOLDTIME+2000, millis() - okClickTime)) ) {
    okHoldCooldown = millis();
    return true;
  }
  return false;
}

void AxiusSSD::resetButtons() {
  if (ups == 3) ups = 0;
  if (dwns == 3) dwns = 0;
  if (oks == 3) oks = 0;
}

void AxiusSSD::tick() {
  updateButtons();

  if (millis() - lastLoadingCharacterSwingTime > 150) {
    lastLoadingCharacterSwingTime = millis();
    loadingCharacterIndex++;
    if (loadingCharacterIndex == 4) loadingCharacterIndex = 0;
    loadingCharacter = loadingCharacters[loadingCharacterIndex];
    sLoadingCharacter[0] = loadingCharacter;
  }

  if (ISREADY && modules.size() > 0) {
    if (++curModule >= modules.size()) curModule = 0;
    modules[curModule] -> update();
    modules[curModule] -> updated = true;
  }
  switch (state) {
    case State::startup:
      updateScreen = true;
      if (HPS == 0) {
        display.setTextColor(WHITE);
        display.setTextWrap(false);
        HPS++;
      } else {
        if (b3 == 1) {
          mods[HPS++-1] -> setup();
          String output = "Preparing " + mods[HPS-1] -> getName();
          Serial.println(output);
        }
        drawText("Preparing", 0);
        drawText(mods[HPS-1] -> getName(), 1);
        drawLoadingLine(HPS-1, mods.size(), 3);
        b3 = !b3;
        lastActionTime = millis();
      }
      if (HPS >= mods.size()) {
        tomenu();
        HPS = -1;
        onLastPreparation();
        resetFont();
        ISREADY = true;
        if (MGR.getParameterBool("modBackUp") && MGR.getParameterBool("crashedInMod")) {
          oks = 1;
        }
      }
      break;
    case State::menu:
      updateScreen = true;
      if (clickY()) {
        if (cursor < mods.size()-1) {
          cursor++;
          if (cursor-startpos > 6) startpos++;
        }
      }
      if (clickX()) {
        if (cursor > 0) {
          cursor--;
          if (cursor-startpos < 0) startpos--;
        }
      }
      if (clickZ()) {
        state = State::modwork;
        MGR.setParameterByte("cursor", cursor);
        if (mods[cursor] -> isRebootable()) MGR.setParameterBool("crashedInMod", true);
        mods[cursor] -> firsttick();
        firstOperationalTick = true;
      }
      for (uint8_t i = startpos; i < startpos+7; i++) {
        if (i >= mods.size()) return;
        drawTextSelector(mods[i] -> getName(), i-startpos, i == cursor);
      }
      break;
    case State::modwork:
      if (firstOperationalTick) {
        updateScreen = true;
        firstOperationalTick = false;
      }
      mods[cursor] -> tick();
      lastActionTime = millis();
      break;
    case State::restart:
      updateScreen = true;
      drawText("restart", 3);
      display.display();
      delay(1000);
      ESP.restart();
      break;
    case State::lockscreen:
      updateScreen = true;
      if (isPSMAnimationOverrided()) {
        renderInPSM();
      } else {
        display.drawBitmap(0, 16, epd_bitmap_allArray[curLogoFrame], 128, 44, WHITE);
        if (++curLogoFrame >= epd_bitmap_allArray_LEN) curLogoFrame = 0;
        delay(130);
      }
      
      if ((UP_DOWN_canWakeFromLockScreen && (clickX() || clickY())) || (OK_canWakeFromLockScreen && clickZ())) {
        tomenu();
      }
      break;
    default:
      state = State::restart;
      break;
  }
}

uint8_t AxiusSSD::modsCount() {
  return mods.size();
}

bool skipFirstCall = true;
void AxiusSSD::tomenu() {
  state = State::menu;
  lastActionTime = millis();
  showStatusBar = true;
  updateScreen = true;
  esppl_set_channel(SHMMR_CHANNEL_DEFAULT);
  enableWIFI();
  set_max_tx_power(100.0);
  cursor = 0;
  startpos = 0;
  for (uint8_t i = 0; i < MGR.getParameterByte("cursor", modsCount()-1); i++) {
    if (++cursor-startpos > 6) startpos++;
  }
  updateScreen = true;
  if (skipFirstCall) {
    skipFirstCall = false;
    return;
  }
  MGR.setParameterBool("crashedInMod", false);
}

bool AxiusSSD::sendWifiFrame(uint8_t *buf, int len) {
#ifdef ESP32
  esp_err_t result = esp_wifi_80211_tx(WIFI_IF_STA, buf, len, false);
  if (result != ESP_OK) {
    Serial.println("cannot send packet: "+String(result));
    return false;
  } else return true;
#else
  return wifi_send_pkt_freedom(buf, len, false) == 0;
#endif
}

void AxiusSSD::toerror() {
  state = State::modwork;
  cursor = 0;
}

void AxiusSSD::restart() {
  state = State::restart;
}

void AxiusSSD::disableWIFI() {  ///-------------------------------------------ESPPL USED HERE
#ifdef ESP32
  esp_wifi_set_promiscuous(false);
#else
  wifi_promiscuous_enable(false);
#endif
  esppl_sniffing_stop();
}

void AxiusSSD::enableWIFI() {  ///-------------------------------------------ESPPL USED HERE
  esppl_init(onIncomingPacket);
  esppl_sniffing_start();
}

void AxiusSSD::set_channel(uint8_t channel) {  ///-------------------------------------------ESPPL USED HERE
  esppl_set_channel(channel);
}

void AxiusSSD::processUnknown(uint8_t* frame, int rssi) {
  for (uint8_t i = 0; i < 6; ++i) {
    if (frame[4+i] != myAddress[i]) {
      delete[] frame;
      return;
    }
  }
  link.onPacket(frame, rssi);
}








#ifdef ESP32
void AxiusSSD::esppl_rx_cb(void* buff, wifi_promiscuous_pkt_type_t type) {
  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;

  uint8_t* data = (uint8_t *)buff;
  uint16_t len = sizeof(data);

  if (len == sizeof(struct sniffer_buf2)) {
    struct sniffer_buf2 *sniffer = (struct sniffer_buf2*) data;
    instance->esppl_buf_to_info(sniffer->buf, sniffer->rx_ctrl.rssi, len);
  } else if (len == sizeof(struct RxControl)) {
    return;
    //struct RxControl *sniffer = (struct RxControl*) buf;
  } else {
    struct sniffer_buf *sniffer = (struct sniffer_buf*) data;
    instance->esppl_buf_to_info(sniffer->buf, sniffer->rx_ctrl.rssi, len);
  }

  if (ppkt->rx_ctrl.sig_len < 26) return;

  if (ppkt->payload[0] == 0xC0) {
    uint8_t* noconst = new uint8_t[ppkt->rx_ctrl.sig_len];
    memcpy(noconst, &ppkt->payload[0], ppkt->rx_ctrl.sig_len);
    //fuck constants
    instance->processUnknown(noconst, ppkt->rx_ctrl.rssi);
    //delete[] noconst;
  }
}
#else
void AxiusSSD::esppl_rx_cb(uint8_t *buf, uint16_t len) {
  int rssi = -1;
  if (len == sizeof(struct sniffer_buf2)) {
    struct sniffer_buf2 *sniffer = (struct sniffer_buf2*) buf;
    rssi = sniffer->rx_ctrl.rssi;
    instance->esppl_buf_to_info(sniffer->buf, rssi, len);
    
  } else if (len == sizeof(struct RxControl)) {
    return;
    //struct RxControl *sniffer = (struct RxControl*) buf;
  } else {
    struct sniffer_buf *sniffer = (struct sniffer_buf*) buf;
    rssi = sniffer->rx_ctrl.rssi;
    instance->esppl_buf_to_info(sniffer->buf, rssi, len);
  }
  if (!buf || len < 26) return;

  if (buf[12] == 0xC0) { //192
    uint8_t* withoutheader = new uint8_t[len-12];
    memcpy(withoutheader, &buf[12], len-12);
    instance->processUnknown(withoutheader, rssi);

    //delete[] withoutheader;
  }
}
#endif

void AxiusSSD::esppl_buf_to_info(uint8_t *frame, signed rssi, uint16_t len) {
  struct esppl_frame_info info;
  uint8_t dstofrom;
  uint8_t pos;
  uint8_t parsecount;

  if (esppl_sniffing_enabled) {
    frame_waitlist++;
    // - "Resets" some fields
    memcpy(info.receiveraddr, esppl_default_mac, ESPPL_MAC_LEN);
    memcpy(info.destinationaddr, esppl_default_mac, ESPPL_MAC_LEN);
    memcpy(info.transmitteraddr, esppl_default_mac, ESPPL_MAC_LEN);
    memcpy(info.sourceaddr, esppl_default_mac, ESPPL_MAC_LEN);
    memcpy(info.bssid, esppl_default_mac, ESPPL_MAC_LEN);
    info.ssid_length = 0;
    info.isvalid = true;
    info.channel = 0;
    info.seq_num = 0;
    // - Populates the fields
    memcpy(info.raw, frame, len - 1);
    info.raw_length = len;
    
    info.frametype = (frame[0] & B00001100) >> 2;
    info.framesubtype = (frame[0] & B11110000) >> 4;
    info.rssi = rssi;
  
    dstofrom = (frame[1] & 3);
  
    switch (info.frametype) {
      case ESPPL_CONTROL: // - Control Frames
        switch (info.framesubtype) {
          case ESPPL_CONTROL_RTS: // - RTS
            memcpy(info.receiveraddr, frame + 4, ESPPL_MAC_LEN);
            memcpy(info.destinationaddr, frame + 4, ESPPL_MAC_LEN); //?
            memcpy(info.transmitteraddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
            memcpy(info.sourceaddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN); //?
            break;
          case ESPPL_CONTROL_CTS: // - CTS
            memcpy(info.receiveraddr, frame + 4, ESPPL_MAC_LEN);
            memcpy(info.destinationaddr, frame + 4, ESPPL_MAC_LEN); //?
            break;
          case ESPPL_CONTROL_ACK: // - ACK
            memcpy(info.receiveraddr, frame + 4, ESPPL_MAC_LEN);
            memcpy(info.destinationaddr, frame + 4, ESPPL_MAC_LEN); //?
            break;
          case ESPPL_CONTROL_PS_POLL: // - PS-POLL
            memcpy(info.receiveraddr, frame + 4, ESPPL_MAC_LEN);
            memcpy(info.destinationaddr, frame + 4, ESPPL_MAC_LEN); //?
            memcpy(info.bssid, frame + 4, ESPPL_MAC_LEN);
            memcpy(info.transmitteraddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
            memcpy(info.sourceaddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN); //?
            break;
          case ESPPL_CONTROL_CF_END: // - CF-END
            memcpy(info.receiveraddr, frame + 4, ESPPL_MAC_LEN);
            memcpy(info.destinationaddr, frame + 4, ESPPL_MAC_LEN);
            memcpy(info.transmitteraddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
            memcpy(info.sourceaddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);          
            memcpy(info.bssid, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
            break;
          case ESPPL_CONTROL_CF_END_CF_ACK: // - CF-END+CF-ACK
            memcpy(info.receiveraddr, frame + 4, ESPPL_MAC_LEN);
            memcpy(info.destinationaddr, frame + 4, ESPPL_MAC_LEN);
            memcpy(info.transmitteraddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
            memcpy(info.sourceaddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);          
            memcpy(info.bssid, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
            break;
          case ESPPL_CONTROL_BLOCK_ACK_REQUEST: // - BlockAckReq
            memcpy(info.receiveraddr, frame + 4, ESPPL_MAC_LEN);
            memcpy(info.destinationaddr, frame + 4, ESPPL_MAC_LEN);
            memcpy(info.transmitteraddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
            memcpy(info.sourceaddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
            break;
          case ESPPL_CONTROL_BLOCK_ACK: // - BlockAck
            memcpy(info.receiveraddr, frame + 4, ESPPL_MAC_LEN);
            memcpy(info.destinationaddr, frame + 4, ESPPL_MAC_LEN);
            memcpy(info.transmitteraddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
            memcpy(info.sourceaddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
            break;
          case ESPPL_CONTROL_CONTROL_WRAPPER: // - Control Wrapper
            //TODO
            break;
        }
        break;
      case ESPPL_DATA: // - Data Frames
        info.seq_num = frame[23] * 0xFF + (frame[22] & 0xF0);
        switch (dstofrom) { // - ToDS FromDS
          case ESPPL_DS_NO: // - ToDS=0 FromDS=0
            memcpy(info.receiveraddr, frame + 4, ESPPL_MAC_LEN);
            memcpy(info.destinationaddr, frame + 4, ESPPL_MAC_LEN);
            memcpy(info.transmitteraddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
            memcpy(info.sourceaddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
            memcpy(info.bssid, frame + 4 + ESPPL_MAC_LEN * 2, ESPPL_MAC_LEN);
            break;
          case ESPPL_DS_TO: // - ToDS=1 FromDS=0
            memcpy(info.receiveraddr, frame + 4, ESPPL_MAC_LEN);
            memcpy(info.bssid, frame + 4, ESPPL_MAC_LEN);
            memcpy(info.transmitteraddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
            memcpy(info.sourceaddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
            memcpy(info.destinationaddr, frame + 4 + ESPPL_MAC_LEN * 2, ESPPL_MAC_LEN); //MSDU
            break;
          case ESPPL_DS_FROM: // - ToDS=0 FromDS=1
            memcpy(info.receiveraddr, frame + 4, ESPPL_MAC_LEN);
            memcpy(info.destinationaddr, frame + 4, ESPPL_MAC_LEN);
            memcpy(info.transmitteraddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
            memcpy(info.bssid, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
            memcpy(info.sourceaddr, frame + 4 + ESPPL_MAC_LEN * 2, ESPPL_MAC_LEN);
            break;
          case ESPPL_DS_TOFROM: // - ToDS=1 FromDS=1
            memcpy(info.receiveraddr, frame + 4, ESPPL_MAC_LEN);
            memcpy(info.transmitteraddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
            memcpy(info.destinationaddr, frame + 4 + ESPPL_MAC_LEN * 2, ESPPL_MAC_LEN);
            memcpy(info.sourceaddr, frame + 4 + ESPPL_MAC_LEN * 2 + 2, ESPPL_MAC_LEN);
            break;
        }
        break;
      case ESPPL_MANAGEMENT: // - Management Frames
        switch (info.framesubtype) {
          case ESPPL_MANAGEMENT_PROBE_RESPONSE:
          case ESPPL_MANAGEMENT_BEACON:
            // - Parses management frame body
            info.seq_num = frame[23] * 0xFF + (frame[22] & 0xF0);
            pos = ESPPL_MANAGEMENT_MAC_HEADER_SIZE;
            parsecount = 0;
            while (pos < len && parsecount < 4) {
              switch (frame[pos]) {
                case 0: // - SSID
                  info.ssid_length = frame[pos + 1];
                  if (info.ssid_length > 32 || info.ssid_length < 0) {
                    info.ssid_length = 0;
                  }
                  memcpy(info.ssid, frame + pos + 2, info.ssid_length);
                  break;
                case 3: // - Channel
                  info.channel = (int) frame[pos + 2];
                  break;
                default:
                  break;
              }
              parsecount++; // - Avoid bad parsing or infinite loop
              pos += frame[pos + 1] + 2;
            }
          default:
            break;
        }
        memcpy(info.receiveraddr, frame + 4, ESPPL_MAC_LEN);
        memcpy(info.destinationaddr, frame + 4, ESPPL_MAC_LEN);
        memcpy(info.transmitteraddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
        memcpy(info.sourceaddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
        memcpy(info.bssid, frame + 4 + ESPPL_MAC_LEN * 2, ESPPL_MAC_LEN);
        break;
      case ESPPL_EXTENSION:
        switch (info.framesubtype) {
          case ESPPL_EXTENSION_DMG_BEACON:
            break;
          case ESPPL_EXTENSION_S1G_BEACON:
            break;
          default:
            break;
        }
        break;
      default:
        info.isvalid = false; //TODO Proper checksum validation
        //Serial.println("invalid ebaniy "+String(info.frametype));
    }
  
    // - User callback function
    if (info.isvalid) {
      user_cb(&info);
    }
    frame_waitlist--;
  }
}

void AxiusSSD::set_max_tx_power(float percent) {
  if (percent > 100.0) percent = 100.0;
  else if (percent < 0.0) percent = 0.0;
  percent /= 100.0;
#ifdef ESP32
  esp_wifi_set_max_tx_power(82.0 * percent);
#else
  system_phy_set_max_tpw(82.0 * percent);
#endif
}

void AxiusSSD::esppl_set_channel(int channel) {
#ifdef ESP32
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
#else
  wifi_set_channel(channel);
#endif
  esppl_channel = channel;
}

bool AxiusSSD::esppl_process_frames() {
  delay(10);
  return frame_waitlist != 0;
}

#ifdef ESP32
static wifi_country_t wifi_country = {.cc="CN", .schan = 1, .nchan = 14};
#endif

void AxiusSSD::esppl_init(void (*cb)(esppl_frame_info *info)) {
  user_cb = cb;
  frame_waitlist = 0;
  #ifdef ESP32
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
  //WiFi.mode(WIFI_AP_STA);
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_set_channelnel(SHMMR_CHANNEL_DEFAULT, WIFI_SECOND_CHAN_NONE)); //error
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(AxiusSSD::esppl_rx_cb));
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
  #else
  wifi_fpm_do_wakeup();
  wifi_fpm_close();
  wifi_set_opmode(STATION_MODE);
  wifi_station_disconnect();
  set_channel(SHMMR_CHANNEL_DEFAULT);
  wifi_promiscuous_enable(false);
  wifi_set_promiscuous_rx_cb(AxiusSSD::esppl_rx_cb);
  wifi_promiscuous_enable(true);
  #endif
  esppl_sniffing_enabled = false;
  wifiInitialized = true;
}

void AxiusSSD::esppl_sniffing_start() {
  esppl_sniffing_enabled = true;
}

void AxiusSSD::esppl_sniffing_stop() {
  wifiInitialized = false;
  esppl_sniffing_enabled = false;
#ifdef ESP32
  ESP_ERROR_CHECK(esp_wifi_disconnect());
  yield();
  ESP_ERROR_CHECK(esp_wifi_stop());
  ESP_ERROR_CHECK(esp_event_loop_delete_default());
  yield();
  ESP_ERROR_CHECK(esp_wifi_deinit());
  yield();
#else
  wifi_station_disconnect(); 
  wifi_set_opmode(NULL_MODE);
  wifi_fpm_set_sleep_type(MODEM_SLEEP_T);
  yield();
  wifi_fpm_open();
  yield();
  auto ret = wifi_fpm_do_sleep(0xFFFFFFF);
  if (ret != 0) Serial.println("error with wifi sleep");
  delay(10);
#endif
}

void AxiusSSD::reverseWIFIBackToNormalMode() {
#ifdef ESP32
  #pragma message("incomplete method reverseWIFIBackToNormalMode() for ESP32")
#else
  disableWIFI();
  wifi_promiscuous_enable(false);
  wifi_fpm_close();
  wifi_set_opmode(STATION_MODE);
  wifi_station_disconnect();
#endif
}





void AxiusSSD::enableKeyboard(String infobar1, String infobar2, uint8_t limitInputSize = 32) {
  keyboardUserInput = "";
  keyboardLimit = limitInputSize;
  keyboardBack = false;
  keyboardNext = false;
  keyboardInfo1 = infobar1;
  keyboardInfo2 = infobar2;
  keyboardCurrentKey = 0;
}

void AxiusSSD::keyboardRender() {
  drawText(keyboardInfo1, 0);
  drawText(keyboardInfo2, 1);

  drawText(keyboardUserInput+((blinkNow(800) && (keyboardUserInput.length() < keyboardLimit))?"_":" "), 2);
  
  uint8_t xshift = 0;
  for (uint i = 0; i < keyboardLettersSize; i++) {
    const char* currentStr = (const char*)pgm_read_ptr(&keyboardLetters[i]);
    uint16_t strLength = strlen_P(currentStr);
    
    uint16_t x = xshift % 128,
             y = 43 + (i / 16) * 9;
    xshift += strLength * 8;
    if (strLength > 1) xshift -= strLength;

    display.setCursor(x, y);
    if (i == keyboardCurrentKey) {
      display.fillRect(x-1, y-rowHeight+1, 7*strLength, rowHeight+1, WHITE);
      display.setTextColor(BLACK);
      
      char buffer[5];
      strncpy_P(buffer, currentStr, sizeof(buffer));
      display.print(buffer);
      
      display.setTextColor(WHITE);
    } else {
      char buffer[5];
      strncpy_P(buffer, currentStr, sizeof(buffer));
      display.print(buffer);
    }
  }

  if (clickX()) {
    if (keyboardCurrentKey > 0) keyboardCurrentKey--;
    else keyboardCurrentKey = keyboardLettersSize-1;
  }
  if (clickY()) {
    if (keyboardCurrentKey < keyboardLettersSize-1) keyboardCurrentKey++;
    else keyboardCurrentKey = 0;
  }
  if (clickZ()) {
    const char* currentStr = (const char*)pgm_read_ptr(&keyboardLetters[keyboardCurrentKey]);
    char buffer[5];
    strncpy_P(buffer, currentStr, sizeof(buffer));
    String currentKeyStr = String(buffer);
    
    if (keyboardCurrentKey < keyboardLettersSize-3 && keyboardUserInput.length() < keyboardLimit) {
      keyboardUserInput += currentKeyStr;
    } else if (keyboardCurrentKey == keyboardLettersSize-3 && keyboardUserInput.length() > 0) {
      keyboardUserInput = keyboardUserInput.substring(0, keyboardUserInput.length() - 1);
    } else if (keyboardCurrentKey == keyboardLettersSize-2) {
      keyboardBack = true;
    } else if (keyboardCurrentKey == keyboardLettersSize-1) {
      keyboardNext = true;
    }
  }
}

bool AxiusSSD::isKeyboardBack() {
  return keyboardBack;
}

bool AxiusSSD::isKeyboardNext() {
  return keyboardNext;
}

String AxiusSSD::keyboardResult() {
  return keyboardUserInput;
}

void AxiusSSD::setKeyboardUserInput(String userinput) {
  keyboardUserInput = userinput;
}