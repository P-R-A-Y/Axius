#ifndef AxiusSSD_h
#define AxiusSSD_h

#define ESPPL_MAC_LEN 6
#define ESPPL_SSID_LEN 33
#define ESPPL_CHANNEL_MIN 1
#define ESPPL_CHANNEL_MAX 14
#define ESPPL_CHANNEL_DEFAULT 1
#define SHMMR_CHANNEL_DEFAULT 14
#define ESPPL_MANAGEMENT_MAC_HEADER_SIZE 36

#define rowHeight      7
#define columnTopShift 18

#include <Arduino.h>

static int* heapStart = ((int*)malloc(1));
static int* heapEnd = heapStart + ESP.getFreeHeap();

//#define WHITE 1
//#define BLACK 0

#include <Wire.h>
#include <Adafruit_GFX.h>
#include "compile_parameters.h"
//.................................
#ifdef USE_SH110X
#include <Adafruit_SH110X.h>
#else
#include <Adafruit_SSD1306.h>
#endif
//.................................
#include <vector>

#ifdef ESP32
#include <esp_wifi.h>
#endif

#include <Fonts/Picopixel.h>
#include <SuperSmallFont.h>
#include <CursedFont.h>

#include "common_utilities.h"
#include "image_storage.h"

#include "mod_class.h"

#include "manager_mod.h"
#include "link.h"
#include "basic_modules.h"
#include "disk.h"

enum class State {
  startup, menu, restart, modwork, lockscreen
};

enum class MemoryChip {
  c16, c256
};

struct RxControl {
  signed rssi: 8;
  unsigned rate: 4;
  unsigned is_group: 1;
  unsigned: 1;
  unsigned sig_mode: 2;
  unsigned legacy_length: 12;
  unsigned damatch0: 1;
  unsigned damatch1: 1;
  unsigned bssidmatch0: 1;
  unsigned bssidmatch1: 1;
  unsigned MCS: 7;
  unsigned CWB: 1;
  unsigned HT_length: 16;
  unsigned Smoothing: 1;
  unsigned Not_Sounding: 1;
  unsigned: 1;
  unsigned Aggregation: 1;
  unsigned STBC: 2;
  unsigned FEC_CODING: 1;
  unsigned SGI: 1;
  unsigned rxend_state: 8;
  unsigned ampdu_cnt: 8;
  unsigned channel: 4;
  unsigned: 12;
};

struct LenSeq {
  uint16_t length;
  uint16_t seq;
  uint8_t  address3[6];
};

struct sniffer_buf {
  struct RxControl rx_ctrl;
  uint8_t buf[36];
  uint16_t cnt;
  struct LenSeq lenseq[1];
};

struct sniffer_buf2 {
  struct RxControl rx_ctrl;
  uint8_t buf[112];
  uint16_t cnt;
  uint16_t len;
};

struct esppl_frame_info {
  uint8_t frametype;
  uint8_t framesubtype;
  uint8_t receiveraddr[ESPPL_MAC_LEN];
  uint8_t destinationaddr[ESPPL_MAC_LEN];
  uint8_t transmitteraddr[ESPPL_MAC_LEN];
  uint8_t sourceaddr[ESPPL_MAC_LEN];
  uint8_t bssid[ESPPL_MAC_LEN];  
  uint8_t ssid[ESPPL_SSID_LEN];
  uint8_t ssid_length;
  unsigned channel;
  signed rssi;
  uint16_t seq_num;
  uint8_t raw[512];
  uint8_t raw_length;
  bool isvalid;
};

const uint8_t ESPPL_DS_NO      = 0;
const uint8_t ESPPL_DS_TO      = 1;
const uint8_t ESPPL_DS_FROM    = 2;
const uint8_t ESPPL_DS_TOFROM  = 3;

const uint8_t ESPPL_MANAGEMENT  = 0;
const uint8_t ESPPL_CONTROL     = 1;
const uint8_t ESPPL_DATA        = 2;
const uint8_t ESPPL_EXTENSION   = 3; //or MISC

const uint8_t ESPPL_EXTENSION_DMG_BEACON = 0;
const uint8_t ESPPL_EXTENSION_S1G_BEACON = 1;

const uint8_t ESPPL_MANAGEMENT_ASSOCIATION_REQUEST    = 0;
const uint8_t ESPPL_MANAGEMENT_ASSOCIATION_RESPONSE   = 1;
const uint8_t ESPPL_MANAGEMENT_REASSOCIATION_REQUEST  = 2;
const uint8_t ESPPL_MANAGEMENT_REASSOCIATION_RESPONSE  = 3;
const uint8_t ESPPL_MANAGEMENT_PROBE_REQUEST          = 4;
const uint8_t ESPPL_MANAGEMENT_PROBE_RESPONSE         = 5;
const uint8_t ESPPL_MANAGEMENT_TIMMING_ADVERTISEMENT  = 6;
const uint8_t ESPPL_MANAGEMENT_RESERVED1              = 7;
const uint8_t ESPPL_MANAGEMENT_BEACON                 = 8;
const uint8_t ESPPL_MANAGEMENT_ATIM                   = 9;
const uint8_t ESPPL_MANAGEMENT_DISASSOCIATION         = 10;
const uint8_t ESPPL_MANAGEMENT_AUTHENTICATION         = 11;
const uint8_t ESPPL_MANAGEMENT_DEAUTHENTICATION       = 12;
const uint8_t ESPPL_MANAGEMENT_ACTION                 = 13;
const uint8_t ESPPL_MANAGEMENT_ACTION_NO_ACK          = 14;
const uint8_t ESPPL_MANAGEMENT_RESERVED2              = 15;

const uint8_t ESPPL_CONTROL_RESERVED1                 = 0;
const uint8_t ESPPL_CONTROL_RESERVED2                 = 1;
const uint8_t ESPPL_CONTROL_RESERVED3                 = 2;
const uint8_t ESPPL_CONTROL_RESERVED4                 = 3;
const uint8_t ESPPL_CONTROL_RESERVED5                 = 4;
const uint8_t ESPPL_CONTROL_RESERVED6                 = 5;
const uint8_t ESPPL_CONTROL_RESERVED7                 = 6;
const uint8_t ESPPL_CONTROL_CONTROL_WRAPPER           = 7;
const uint8_t ESPPL_CONTROL_BLOCK_ACK_REQUEST         = 8;
const uint8_t ESPPL_CONTROL_BLOCK_ACK                 = 9;
const uint8_t ESPPL_CONTROL_PS_POLL                   = 10;
const uint8_t ESPPL_CONTROL_RTS                       = 11;
const uint8_t ESPPL_CONTROL_CTS                       = 12;
const uint8_t ESPPL_CONTROL_ACK                       = 13;
const uint8_t ESPPL_CONTROL_CF_END                    = 14;
const uint8_t ESPPL_CONTROL_CF_END_CF_ACK             = 15;

const uint8_t ESPPL_DATA_DATA                         = 0;
const uint8_t ESPPL_DATA_DATA_CF_ACK                  = 1;
const uint8_t ESPPL_DATA_DATA_CF_POLL                 = 2;
const uint8_t ESPPL_DATA_DATA_CF_ACK_CF_POLL          = 3;
const uint8_t ESPPL_DATA_NULL                         = 4;
const uint8_t ESPPL_DATA_CF_ACK                       = 5;
const uint8_t ESPPL_DATA_CF_POLL                      = 6;
const uint8_t ESPPL_DATA_CF_ACK_CF_POLL               = 7;
const uint8_t ESPPL_DATA_QOS_DATA                     = 8;
const uint8_t ESPPL_DATA_QOS_DATA_CF_ACK              = 9;
const uint8_t ESPPL_DATA_QOS_DATA_CF_ACK_CF_POLL      = 10;
const uint8_t ESPPL_DATA_QOS_NULL                     = 11;
const uint8_t ESPPL_DATA_RESERVED1                    = 12;
const uint8_t ESPPL_DATA_QOS_CF_POLL                  = 13;
const uint8_t ESPPL_DATA_QOS_CF_ACK_CF_POLL           = 14;

void ICACHE_RAM_ATTR readEncoder();

class AxiusSSD {
  public:
    #ifdef USE_SH110X
      AxiusSSD() : display(Adafruit_SH1106G(128, 64, &Wire, -1)), about(this, 0x0001), link(this, 0x0002), MGR(this, 0x0000), disk(this, 0x0003) {};
    #else
      AxiusSSD() : display(Adafruit_SSD1306(128, 64, &Wire, -1)), about(this, 0x0001), link(this, 0x0002), MGR(this, 0x0000), disk(this, 0x0003) {};
    #endif
    
    void begin(String devname, MemoryChip c, float maxAfkSeconds, const uint8_t defaultOKButton, bool isInvertButtonReadMethod, uint32_t onBootFreeHeap);
    void setButtons(bool usingEncoder, const uint8_t UPButtonInput, const uint8_t DOWNButtonInput, const uint8_t OKButtonInput, bool isOkFromEncoder, bool isInvertButtonReadMethod);
    void setFlip(bool f) {
      if (f == isFlip) return;
      isFlip = f;
      if (f) display.setRotation(2);
      else display.setRotation(0);
    }
    void setContrast(uint8_t contrastPercent) {
      display.ssd1306_command(SSD1306_SETCONTRAST);
      display.ssd1306_command(uint8_t(255.0 * (contrastPercent / 100.0)));
    }
    void setDisplayPower(bool state) {
      if(state) {
        display.ssd1306_command(SSD1306_DISPLAYON);
      } else {
        display.ssd1306_command(SSD1306_DISPLAYOFF);
      }
    }
    void addMod(Mod* m)           {  mods.push_back(m);   };
    void addModule(Module* m)     { modules.push_back(m); };
    void addIcon(IconProvider* i) {  icons.push_back(i);  };
    void setLockScreen                (void (*func)(                      ));
    void setLastPreparation           (void (*func)(                      ));
    void setIconApplyer               (void (*func)(                      ));
    void setLockScreenOverrideChecker (bool (*func)(                      ));
    void setIncomingPacketListener    (void (*func)(esppl_frame_info *info));
    void setIncomingPayloadListener   (void (*func)(float rssi, uint8_t sender, char* prefix, uint8_t payloadSize, uint8_t* payload)) { onCustomPayloadReceive = func; hasIncomingPayloadListener = true;}
    
    void resetFont() {
      if (MGR.getParameterBool("useCursedFont")) display.setFont(&CursedFont);
      else                                       display.setFont(&SuperSmallFont);
    }

    uint16_t getTextWidth(String text) {
      int16_t buff;
      uint16_t textWidth, ubuff;
      display.getTextBounds(text, 0, 0, &buff, &buff, &textWidth, &ubuff);
      return textWidth;
    }

    void updateButtons();
    void tick();
    void endRender();
    void updateStatusBar();

    void drawTextSelector(String text, uint8_t row, bool isselected);
    void drawTextSelectorWithBorder(String text, uint8_t row, bool isselected, bool border);
    void drawTextWithBorder(String text, uint8_t row, bool border);
    void drawLoadingLine(float cur, float max, uint8_t row);
    void drawText(String text, uint8_t row);
    void drawTextMiddle(String text, uint8_t row);
    bool clickX();
    bool clickY();
    bool clickZ();  
    void resetButtons();
    uint8_t modsCount();
    
    void enableKeyboard(String infobar1, String infobar2, uint8_t limitInputSize);
    void keyboardRender();
    bool isKeyboardBack();
    bool isKeyboardNext();
    String keyboardResult();
    void setKeyboardUserInput(String userinput);

#if defined(USE_SH110X)
    Adafruit_SH1106G display;
#else
    Adafruit_SSD1306 display;
#endif
    ManagerMod MGR;
    Link link;
    Disk disk;

    bool showStatusBar = false;
    //bool fullDisableRender = false;
    bool updateScreen = true;
    bool ISREADY = false;
    String deviceName = "#";
    MemoryChip chip;
    bool hasIncomingPayloadListener = false;
    bool wifiInitialized = false;

    bool disableLogo = false;
    bool minimalStatusBar = false;

    char loadingCharacter;
    String sLoadingCharacter = ".";

    void tomenu();
    void toerror();
    void restart();
    void forceRestart();
    void disableWIFI();
    void enableWIFI();
    bool sendWifiFrame(uint8_t *buf, int len);
    void set_channel(uint8_t channel);
    void processUnknown(uint8_t* frame, int rssi);
    void reverseWIFIBackToNormalMode();

    static AxiusSSD* instance;
    
    int frame_waitlist = 0;
    bool esppl_sniffing_enabled = false;
#ifdef ESP32
    static void esppl_rx_cb(void* buff, wifi_promiscuous_pkt_type_t type);
#else
    static void esppl_rx_cb(uint8_t *buf, uint16_t len);
#endif
    void set_max_tx_power(float percent);

    GyroscopeModule gyroscope;
    VoltmeterModule voltmeter;

    std::vector<Module*> modules;
    
    void (*onCustomPayloadReceive)(float rssi, uint8_t sender, char* prefix, uint8_t payloadSize, uint8_t* payload);

    void tryForceSwitchToMod(String name) {
      uint8_t n = findModeIndexByName(name);
      if (n != 256) {
        cursor = n;
        mods[cursor] -> firsttick();
        firstOperationalTick = true;
        lastActionTime = millis();
        state = State::modwork;
      }
    }
    void esppl_buf_to_info(uint8_t *frame, signed rssi, uint16_t len);
    void (*user_cb)(esppl_frame_info *info);
  private:
    int8_t HPS = 0;
    State state = State::startup;
    uint8_t cursor = 0;
    uint8_t startpos = 0;
    uint32_t lastActionTime = 0;
    uint8_t ups = 0, dwns = 0, oks = 0;
    uint32_t upClickTime = 0, dwnClickTime = 0, okClickTime = 0, upHoldCooldown = 0, dwnHoldCooldown = 0, okHoldCooldown = 0;
    const uint32_t HOLDTIME = 350;
    const uint32_t MAX_HOLD_COOLDOWN = 50, MIN_HOLD_COOLDOWN = 200;
    uint32_t onBootFreeHeap = 1;
    uint8_t esppl_channel = ESPPL_CHANNEL_DEFAULT;
    uint8_t esppl_default_mac[ESPPL_MAC_LEN] = {0x00,0x00,0x00,0x00,0x00,0x00};
    void esppl_set_channel(int channel);
    bool esppl_process_frames();
    void esppl_init(void (*cb)(esppl_frame_info *info));
    About about;
    IconProvider wifiIcon;
    char loadingCharacters[5] = {'|', '/', '-', '\\'};
    uint8_t loadingCharacterIndex = 0;
    uint32_t lastLoadingCharacterSwingTime = 0;
    bool isFlip = false;
    void esppl_sniffing_start();
    void esppl_sniffing_stop();
    int upb = 0, dwnb = 0, okb = 0;
    bool useEncoder = true;
    //settings
    float maxAfkSeconds = 6000;
    bool UP_DOWN_canWakeFromLockScreen = true;
    bool OK_canWakeFromLockScreen = true;
    bool invertButtonReadMethod = false;
    //settings
    uint8_t myAddress[6] = {0xFA, 0xBA, 0xCA, 0xBA, 0x08, 0x01};
    const uint8_t llminx = 5, llwidth = 94;
    bool b3 = false, firstOperationalTick = false;
    unsigned long previousMillis = 0;
    const uint8_t afkBarWidth = 20;
    uint8_t curLogoFrame = 0;
    std::vector<IconProvider*> icons;
    std::vector<Mod*> mods;
    uint8_t findModeIndexByName(String name) {
      for (uint8_t i = 0; i < mods.size(); i++) {
        if (mods[i]->getName().equals(name)) {
          return i;
        }
      }
      return 255;
    }

    bool logoanim1stframe = true;
    uint32_t lastLAFChange = 0;
    uint8_t curModule = 0;

    void (*renderInPSM)();
    void (*onLastPreparation)();
    void (*applyIcons)();
    bool (*isPSMAnimationOverrided)();
    void (*onIncomingPacket)(esppl_frame_info *info);

    const int epd_bitmap_allArray_LEN = 31;  
    const uint8_t* epd_bitmap_allArray[31] = {
      epd_bitmap_aspose_video_133626583510124466_out_000,
      epd_bitmap_aspose_video_133626583510124466_out_001,
      epd_bitmap_aspose_video_133626583510124466_out_002,
      epd_bitmap_aspose_video_133626583510124466_out_003,
      epd_bitmap_aspose_video_133626583510124466_out_004,
      epd_bitmap_aspose_video_133626583510124466_out_005,
      epd_bitmap_aspose_video_133626583510124466_out_006,
      epd_bitmap_aspose_video_133626583510124466_out_007,
      epd_bitmap_aspose_video_133626583510124466_out_008,
      epd_bitmap_aspose_video_133626583510124466_out_009,
      epd_bitmap_aspose_video_133626583510124466_out_010,
      epd_bitmap_aspose_video_133626583510124466_out_011,
      epd_bitmap_aspose_video_133626583510124466_out_012,
      epd_bitmap_aspose_video_133626583510124466_out_013,
      epd_bitmap_aspose_video_133626583510124466_out_014,
      epd_bitmap_aspose_video_133626583510124466_out_015,
      epd_bitmap_aspose_video_133626583510124466_out_016,
      epd_bitmap_aspose_video_133626583510124466_out_017,
      epd_bitmap_aspose_video_133626583510124466_out_018,
      epd_bitmap_aspose_video_133626583510124466_out_019,
      epd_bitmap_aspose_video_133626583510124466_out_020,
      epd_bitmap_aspose_video_133626583510124466_out_021,
      epd_bitmap_aspose_video_133626583510124466_out_022,
      epd_bitmap_aspose_video_133626583510124466_out_023,
      epd_bitmap_aspose_video_133626583510124466_out_024,
      epd_bitmap_aspose_video_133626583510124466_out_025,
      epd_bitmap_aspose_video_133626583510124466_out_026,
      epd_bitmap_aspose_video_133626583510124466_out_027,
      epd_bitmap_aspose_video_133626583510124466_out_028,
      epd_bitmap_aspose_video_133626583510124466_out_029,
      epd_bitmap_aspose_video_133626583510124466_out_030
    };

    uint8_t keyboardLimit, keyboardCurrentKey;
    String keyboardUserInput = "";
    String keyboardInfo1 = "";
    String keyboardInfo2 = "";
    const static uint8_t keyboardLettersSize = 42;
    bool keyboardBack = false, keyboardNext = false;
};

#endif