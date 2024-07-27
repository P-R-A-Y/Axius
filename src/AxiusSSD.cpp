#include <AxiusSSD.h>

extern "C" {
  #include "user_interface.h"
  typedef void (*freedom_outside_cb_t)(uint8 status);
  int  wifi_register_send_pkt_freedom_cb(freedom_outside_cb_t cb);
  void wifi_unregister_send_pkt_freedom_cb(void);
  int  wifi_send_pkt_freedom(uint8 *buf, int len, bool sys_seq);
}

void AxiusSSD::addModule(Module* m) {modules.push_back(m);}
AxiusSSD::AxiusSSD() : display(Adafruit_SSD1306(128, 64, &Wire, -1)) {}
AxiusSSD* AxiusSSD::instance = nullptr;
void AxiusSSD::begin(String devname, MemoryChip c, float nmaxAfkSeconds) {
  deviceName = devname;
  chip = c;
  maxAfkSeconds = nmaxAfkSeconds;
  while (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 screen allocation failed"));
    delay(50);
    ESP.restart();
  }
  display.clearDisplay();
  display.display();

  for (uint8_t i = 0; i < modules.size(); i++) {
    modules[i]->connect();
  }
  
  if (mods.size() == 0) {
    mods.push_back(&MEM);
    mods.push_back(&al);
    mods.push_back(&a);
  } else {
    mods.insert(mods.begin(), &al);
    mods.insert(mods.begin(), &MEM);
    mods.push_back(&a);
  }
  instance = this;
}

void AxiusSSD::drawTextSelector(String text, uint8_t row, bool isselected) {
  uint8_t y = row*11+columnTopShift;
  if (isselected)
    display.fillCircle(3, y+3, 2, SSD1306_WHITE);
  else
    display.drawCircle(3, y+3, 2, SSD1306_WHITE);
  display.setCursor(8,y);
  display.println(text);
}

void AxiusSSD::drawTextSelectorWithBorder(String text, uint8_t row, bool isselected, bool border) {
  uint8_t y = row*11+columnTopShift;
  if (isselected)
    display.fillCircle(3, y+3, 2, SSD1306_WHITE);
  else
    display.drawCircle(3, y+3, 2, SSD1306_WHITE);
  
  if (border) display.fillRect(8, y-1, 122, 9, SSD1306_WHITE);

  display.setTextColor(border ? SSD1306_BLACK : SSD1306_WHITE);
  display.setCursor(9,y);
  display.println(text);
  display.setTextColor(SSD1306_WHITE);
}

void AxiusSSD::drawLoadingLine(float cur, float max, uint8_t row) {
  uint8_t miny = row*11+columnTopShift;
  display.drawRect(llminx, miny, llwidth, 11, SSD1306_WHITE);
  if (cur == 0) return;

  float cpm = (cur / max);
  uint8_t midx = (uint8_t)(cpm * llwidth);
  
  display.fillRect(llminx, miny, midx, 11, SSD1306_WHITE);
  display.setCursor(llwidth+8, miny);
  display.println(String((uint8_t)(cpm*100))+"%");
}

void AxiusSSD::drawText(String text, uint8_t row) {
  uint8_t y = row*11+columnTopShift;
  display.setCursor(0,y);
  display.println(text);
}

void AxiusSSD::updatestatusbar() {

}

void AxiusSSD::addMod(Mod* m) {
  mods.push_back(m);
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
  display.setCursor(0,2);
  display.println("AX");

  display.setFont(&Picopixel);
  float frequency = 1000.0 / (millis() - previousMillis);
  previousMillis = millis();
  //Serial.println(frequency);
  display.setCursor(107,4);
  display.print(frequency);
  display.print("F");

  display.setCursor(107,10);
  display.print(additionalTextField);

  display.drawRect(13, 2, afkBarWidth, 7, SSD1306_WHITE);
  if (millis() - lastActionTime < maxAfkSeconds) {
    int16_t ww = afkBarWidth - ((millis() - lastActionTime) / maxAfkSeconds * afkBarWidth);
    display.fillRect(13, 2, ww, 7, SSD1306_WHITE);
  }

  applyIcons();

  display.setFont();
}

void AxiusSSD::endRender() {
  if (!fullDisableRender) {
    if (millis() - lastActionTime > maxAfkSeconds) {
      state = State::lockscreen;
    }

    updateStatusBar();
    display.display();
    display.clearDisplay();
  }

  al.supertick();
  resetbuttons();
  updateScreen = false;
}

void AxiusSSD::tick() {
  if (FULLPREPARED && modules.size() > 0) {
    if (++curModule >= modules.size()) curModule = 0;
    modules[curModule] -> update();
    modules[curModule] -> updated = true;
  }
  switch (state) {
    case State::startup:
      if (HPS == 0) {
        display.setTextColor(SSD1306_WHITE);
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
        drawLoadingLine(HPS-1, mods.size(), 2);
        b3 = !b3;
      }
      if (HPS >= mods.size()) {
        tomenu();
        HPS = -1;
        onLastPreparation();
        FULLPREPARED = true;
      }
      break;
    case State::menu:
      if (readdwn()) {
        if (cursor < mods.size()-1) {
          cursor++;
          if (cursor-startpos > 4) startpos++;
        }
      }
      if (readup()) {
        if (cursor > 0) {
          cursor--;
          if (cursor-startpos < 0) startpos--;
        }
      }
      if (readok()) {
        state = State::modwork;
        MEM.setParameterByte("cursor", cursor);
        mods[cursor] -> firsttick();
      }
      for (uint8_t i = startpos; i < startpos+5; i++) {
        if (i >= mods.size()) return;
        drawTextSelector(mods[i] -> getName(), i-startpos, i == cursor);
      }
      break;
    case State::modwork:
      mods[cursor] -> tick();
      lastActionTime = millis();
      break;
    case State::restart:
      drawText("restart", 3);
      display.display();
      delay(1000);
      ESP.restart();
      break;
    case State::lockscreen:
      //drawText("AFK", -1);

      if (isPSMAnimationOverrided()) {
        renderInPSM();
      } else {
        display.drawBitmap(0, 16, epd_bitmap_allArray[curLogoFrame], 128, 44, SSD1306_WHITE);
        if (++curLogoFrame >= epd_bitmap_allArray_LEN) curLogoFrame = 0;
        delay(130);
      }
      
      if (disableWifiInLockScreen) {
        if (!isWifiTurnedOff) {
          isWifiTurnedOff = true;
          WiFi.mode(WIFI_OFF);
          wifi_set_sleep_type(LIGHT_SLEEP_T);
          WiFi.forceSleepBegin();
        }
      }
      
      if ((UP_DOWN_canWakeFromLockScreen && (readup() || readdwn())) || (OK_canWakeFromLockScreen && readok())) {
        tomenu();
        if (disableWifiInLockScreen) {
          isWifiTurnedOff = false;
          WiFi.forceSleepWake();
        }
      }
      break;
    default:
      state = State::restart;
  }
}

uint8_t AxiusSSD::modsCount() {
  return mods.size();
}

bool AxiusSSD::readup() {
  if (ups == 3) {
    ups = 0;
    return true;
  } else return false;
}

bool AxiusSSD::readdwn() {
  if (dwns == 3) {
    dwns = 0;
    return true;
  } else return false;
}

bool AxiusSSD::readok() {
  if (oks == 3) {
    oks = 0;
    return true;
  } else return false;
}

void AxiusSSD::resetbuttons() {
  if (ups == 3) ups = 0;
  if (dwns == 3) dwns = 0;
  if (oks == 3) oks = 0;
}

void AxiusSSD::tomenu() {  ///-------------------------------------------ESPPL USED HERE
  state = State::menu;
  showStatusBar = true;
  esppl_set_channel(ESPPL_CHANNEL_MAX);
  startPacketListening();
  cursor = 0;
  startpos = 0;
  for (uint8_t i = 0; i < MEM.getParameterByte("cursor", modsCount()-1); i++) {
    if (++cursor-startpos > 4) startpos++;
  }
  updateScreen = true;
}

bool AxiusSSD::sendWifiFrame(uint8 *buf, int len) {
  return wifi_send_pkt_freedom(buf, len, 0) == 0;
}

void AxiusSSD::toerror() {
  state = State::modwork;
  cursor = 0;
}

void AxiusSSD::restart() {
  state = State::restart;
}

void AxiusSSD::forceRestart() {
  
}

void AxiusSSD::stopPacketListening() {  ///-------------------------------------------ESPPL USED HERE
  wifi_promiscuous_enable(false);
  esppl_sniffing_stop();
}

void AxiusSSD::startPacketListening() {  ///-------------------------------------------ESPPL USED HERE
  esppl_init(onIncomingPacket);
  esppl_sniffing_start();
}

void AxiusSSD::wifi_set_chan(uint8_t channel) {  ///-------------------------------------------ESPPL USED HERE
  esppl_set_channel(channel);
}

void AxiusSSD::processUnknown(uint8_t* frame) {
  for (uint8_t i = 0; i < 6; ++i) {
    if (frame[4+i] != myAddress[i]) {
      return;
    }
  }
  al.onPacket(frame);
}









void AxiusSSD::esppl_rx_cb(uint8_t *buf, uint16_t len) {
  if (len == sizeof(struct sniffer_buf2)) {
    struct sniffer_buf2 *sniffer = (struct sniffer_buf2*) buf;
    instance->esppl_buf_to_info(sniffer->buf, sniffer->rx_ctrl.rssi, len);
  } else if (len == sizeof(struct RxControl)) {
    //struct RxControl *sniffer = (struct RxControl*) buf;
  } else {
    struct sniffer_buf *sniffer = (struct sniffer_buf*) buf;
    instance->esppl_buf_to_info(sniffer->buf, sniffer->rx_ctrl.rssi, len);
  }

  if (!buf || len < 26) return;

  if (buf[12] == 0xC0) {
    uint8_t* withoutheader = new uint8_t[len-12];
    memcpy(withoutheader, &buf[12], len-12);
    instance->processUnknown(withoutheader);
  }
}

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
                  if (info.ssid_length > 32/* || info.ssid_length < 0*/) {
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
      default:
        info.isvalid = false; //TODO Proper checksum validation
    }
  
    // - User callback function
    if (info.isvalid) {
      user_cb(&info);
    }
    frame_waitlist--;
  }
}

void AxiusSSD::esppl_set_channel(int channel) {
  wifi_set_channel(channel);
  esppl_channel = channel;
}

bool AxiusSSD::esppl_process_frames() {
  delay(10);
  return frame_waitlist != 0;
}

void AxiusSSD::esppl_init(void (*cb)(esppl_frame_info *info)) {
  user_cb = cb;
  frame_waitlist = 0;
  wifi_station_disconnect();
  wifi_set_opmode(STATION_MODE);
  wifi_set_channel(esppl_channel);
  wifi_promiscuous_enable(false);
  wifi_set_promiscuous_rx_cb(AxiusSSD::esppl_rx_cb);
  wifi_promiscuous_enable(true);
  esppl_sniffing_enabled = false;
}

void AxiusSSD::esppl_sniffing_start() {
  esppl_sniffing_enabled = true;
}

void AxiusSSD::esppl_sniffing_stop() {
  esppl_sniffing_enabled = false;
}