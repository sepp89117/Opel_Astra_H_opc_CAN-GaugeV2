/*
   Autor: Sebastian Balzer
   Version: 2.4 XL + PC-Out
   Date: 03.04.2021

   Hardware: Teensy 4.0, Waveshare CAN Board SN65HVD230, Waveshare 4inch RPi LCD (C) ILI9486 with XPT2046 Touch, LM2596 DC/DC Converter - set 5.0 Volt out
   Car: Opel Astra H opc
   Interface 1: HSCAN 500 kBaud (Pin 6 and 14 on OBD connector)
   Interface 2: MSCAN  95.2 kBaud (Pin 3 and 11 on OBD connector)
   or Interface 2: MSCAN  33.3 kBaud (Pin 1 and 5(GND) on OBD connector)
*/

#include "SPI.h"
#include <XPT2046_Touchscreen.h>
#include <FlexCAN_T4.h>
#include "TeensyThreads.h"
#include <EEPROM.h>
#include "ILI9486_t3n.h"
#include "ili9486_t3n_font_Arial.h"
#include "ili9486_t3n_font_ArialBold.h"
#include "opcNumFont9486.h"
#include "graphics.c"
#include "bcButton.h"

class info {
  public:
    char* Text;
    uint16_t TextColor;
    int Prio;
    unsigned long Dur;
    bool firstDrawn = false;
    bool enabled = true;

    info(char* text, uint16_t color, int prio, int dur) {
      //if (sizeof(text) >= 27) { //wenn Text zu lang für Infozeile ist wird er gekürzt
      //Text = text.substring(0, 26);
      //} else {
      Text = text;
      //}
      TextColor = color;
      Prio = prio;
      Dur = dur;
    };

    info() {
      Text = (char*)"";
      TextColor = 0;
      Prio = 0;
      Dur = 0;
    };

    bool operator == (info cInfo) {
      if (Text == cInfo.Text) return true;
      else return false;
    };

    bool operator != (info cInfo) {
      if (Text != cInfo.Text) return true;
      else return false;
    };
};

//Threads
int tech2ThreadID;
int displayDataThreadID;
int heartBeatThreadID;
int spiThreadID;
int alarmThreadID = -1;

//Touchscreen config
#define CS_PIN  7
XPT2046_Touchscreen ts(CS_PIN);
#define TIRQ_PIN  2
// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 3800
#define TS_MINY 190
#define TS_MAXX 120
#define TS_MAXY 3800

//TFT config
#define TFT_DC       9
#define TFT_CS      10
#define TFT_RST      8
#define TFT_MOSI    11
#define TFT_SCLK    13
#define TFT_MISO    12
ILI9486_t3n tft = ILI9486_t3n(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);

//Can 500 config
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_32> can500;
uint32_t canBaud500 = (uint32_t)500000;
int responseTimeout = 1000;
int lastResponseTime = responseTimeout * 2;

//Can 95 config - unused
//FlexCAN_T4<CAN1, RX_SIZE_128, TX_SIZE_16> can95;
//uint32_t canBaud95 = (uint32_t)95238;

//Can 33.3 config
FlexCAN_T4<CAN1, RX_SIZE_128, TX_SIZE_16> can33;
uint32_t canBaud33 = (uint32_t)33333;

//Buzzer pin
const int buzzer = 4;

//Definition of CAN-IDs
#define CANID_REPLY     0x7E8
#define CANID_REQUEST   0x7E0
#define CANID_DATAREPLY 0x5E8

//Display Views
#define HOME_VIEW  0
#define FC_VIEW    1
#define MAX_VIEW   2
#define ACC_VIEW   3
#define GRAPH_VIEW  4
#define IGNITION_VIEW  5
#define FUEL_VIEW  6
#define SETT_VIEW  7
int actualView;

//Styles
uint16_t backColor = ILI9486_BLACK;
uint16_t foreColor = ILI9486_WHITE;

//Motordaten-Variablen
int ect = 0;
float vBatt = 0.0f;
int boost = 0;
int iat = 0;
int ambientTemp = 0;
float maf = 0.0f;
float mafVolt = 0.0f;
float ign = 0;
int sft = 0;
float lambdaVal = 0.0f;
int fcs = 0;
int rpm = 0;
int speed = 0;
float inj = 0.0f;
bool isMilOn = false;
float aat = 0.0f; //Außentemp. (CAN 33,3kbps)
int distanceBehind = 0;
float knockVal = 0.0f;
byte knockRet1;
byte knockRet2;
byte knockRet3;
byte knockRet4;

//berechnete Daten
float absolutConsume;
int injAuslast = 0;
int power = 0;
float consume = 0.0f;
float afr = 0.0f;
bool waschwasserLeer = false;
bool rGang = false;
bool rOut = false;
long lastRGang = 0;
unsigned long EngineStartTime = 0;
unsigned long lastSpeedTime = 0;
unsigned long lastConsumeTime = 0;
double strecke = 0;
double odoMeter = 0;
float tankInhalt = 0;

//Alte Motordaten-Variablen
float oldvBatt = 0.0f;
int oldfcs = 0;
int oldect = 0;
int oldboost = 0;
int oldiat = 0;
float oldmaf = 0.0f;
float oldmafVolt = 0.0f;
int oldign = 0;
int oldsft = 0;
int oldrpm = 0;
int oldspeed = 0;
float oldinj = 0.0f;
int oldpower = 0;
int oldmoment = 0;
int oldinjAuslast = 0;
bool oldisMilOn = true;
float oldconsume = 0.0f;
float oldafr = 0.0f;
float oldaat;

//max-reached-vals
int rMAXect = 0;
int rMAXboost = 0;
int rMAXiat = 0;
int rMAXmaf = 0;
float rMAXign = 0.0f;
int rMAXrpm = 0;
int rMAXspeed = 0;
float rMAXinj = 0.0f;
int rMAXinjAuslast = 0;
int rMAXpower = 0;
int rMAXmoment = 0;
int rMAXconsume = 0;
float rMAXafr = 0.0f;

//Beschleunigungszeiten Variablen
long t100 = 0;
long t200 = 0;
long t100Start = 0;
long t200Start = 0;
long t100Stop = 0;
long t200Stop = 0;
bool t100Started = false;
bool t200Started = false;
bool gotT100 = false;
bool gotT200 = false;
int tachoabw = 0;
int oldactSpeed;
int actSpeed;

//Programm-Variablen
char bcVersion[7] = "2.4 XL";
const int arrayLength = 39;
int boostArray[arrayLength];
int mafArray[arrayLength];
int consumeArray[arrayLength];
bool ecuConnected = false;
bool listSet = false;
bool response = false;
int noResponseCount = 0;
info actualInfo;
info oldInfo;
unsigned long infoWritten;
bool dashViewLoad = true;
bool answered = false;
char bufInt[12]; //global variable created because malloc cannot be used here
char bufFloat[12]; //global variable created because malloc cannot be used here
bool waschwasserLeerOk = false;
bool dispBufReady = false;
bool graphHold = false;
bool firstBoostValRec = true;
int firstBoostVal = 100; //atmosperic pressure, will be corrected by first value received
int engineStarts = 0;

//Settings
int lcdBright = 255;
int startLcdBright = 255;
int secondLcdBright = 100;
int thirdLcdBright = 50;
bool skipBoot = false;
bool playAlarms = true;

//Buttons
Button btnDTCs;
Button btnGraph;
Button btnMax;
Button btnAcc;
Button btnBack;
Button btnHome;
Button btnDelDtc;
Button btnResetMax;
Button btnResetAcc;
Button btnPlus;
Button btnMinus;
Button btnHold;
Button btnFuel;
Button btnIgnition;
Button btnLcdBright;
Button btnSettings;
Button btnSave;
Button setBtnPlus1;
Button setBtnMinus1;
Button setBtnPlus2;
Button setBtnMinus2;
Button setBtnPlus3;
Button setBtnMinus3;
Button setBtnSwitch1;
Button setBtnSwitch2;

void setup() {
  analogWrite(3, 0); //Disp. LED off

  Serial.begin(115200);

  //init TFT
  tft.begin();
  tft.fillScreen(backColor);
  tft.useFrameBuffer(1);
  tft.setRotation(3);

  //init CAN500
  can500.begin();
  can500.setBaudRate(canBaud500);
  for (int i = 0; i < 10; i++) {
    can500.setMB(FLEXCAN_MAILBOX(i), RX, STD);
  }
  can500.enableFIFO();
  can500.enableFIFOInterrupt();
  can500.setFIFOFilter(REJECT_ALL);
  can500.setFIFOFilter(0, CANID_DATAREPLY, STD);
  can500.setFIFOFilter(1, CANID_REPLY, STD);
  can500.onReceive(handleData500Msg);
  //Serial.print("CAN 500 init okay with ");
  //can500.mailboxStatus();

  //init CAN33
  can33.begin();
  can33.setBaudRate(canBaud33);
  for (int i = 0; i < 16; i++) {
    can33.setMB(FLEXCAN_MAILBOX(i), RX, STD);
  }
  can33.enableFIFO();
  can33.enableFIFOInterrupt();
  can33.setFIFOFilter(REJECT_ALL);

  can33.setFIFOFilter(0, 0x175, STD);
  can33.setFIFOFilter(1, 0x430, STD);
  can33.setFIFOFilter(2, 0x445, STD);
  can33.setFIFOFilter(3, 0x626, STD);

  can33.onReceive(handleData33Msg);
  //Serial.print("CAN 33 init okay with ");
  //can33.mailboxStatus();

  //init touch
  ts.setRotation(3);
  if (!ts.begin()) {
    //Serial.println("Couldn't start touchscreen controller");
  } else {
    //Serial.println("Touchscreen init okay");
  }
  
  if(EEPROM.read(5) < 2) skipBoot = EEPROM.read(5);
  playAlarms = EEPROM.read(6);
  startLcdBright = EEPROM.read(7);
  secondLcdBright = EEPROM.read(8);
  thirdLcdBright = EEPROM.read(9);

  if(startLcdBright < 1) startLcdBright = 255;
  
  analogWrite(3, startLcdBright); //Disp. LED on
  if(!skipBoot) bootAnimation(); //show bootlogo
  
  //--- init Buttons
  //Button(int x, int y, int w, int h, char *text, uint16_t textColor, const short unsigned int *bgImage)
  btnDTCs = Button(0, 285, 84, 34, (char*)"DTCs", backColor, styleBtn);
  btnGraph = Button(90, 285, 84, 34, (char*)"Graph", backColor, styleBtn);
  btnMax = Button(180, 285, 84, 34, (char*)"MaxV", backColor, styleBtn);
  btnAcc = Button(270, 285, 84, 34, (char*)"AccM", backColor, styleBtn);
  btnFuel = Button(360, 285, 84, 34, (char*)"Fuel", backColor, styleBtn);
  btnResetAcc = Button(90, 285, 84, 34, (char*)"Reset", backColor, styleBtn);
  btnBack = Button(0, 0, 84, 34, (char*)"Back", backColor, styleBtn);
  btnHome = Button(0, 285, 84, 34, (char*)"Home", backColor, styleBtn);
  btnDelDtc = Button(90, 0, 84, 34, (char*)"Clear", backColor, styleBtn);
  btnResetMax = Button(90, 285, 84, 34, (char*)"Reset", backColor, styleBtn);
  btnPlus = Button(345, 210, 30, 30, (char*)"+", backColor, styleBtnSmall);
  btnMinus = Button(390, 210, 30, 30, (char*)"-", backColor, styleBtnSmall);
  btnHold = Button(90, 285, 84, 34, (char*)"Hold", backColor, styleBtn);
  btnIgnition = Button(180, 285, 84, 34, (char*)"Ign", backColor, styleBtn);
  btnSave = Button(90, 285, 84, 34, (char*)"Save", backColor, styleBtn);
  //--- Header buttons (x-area is from 130 to 350 px)
  btnLcdBright = Button(160, 0, 30, 30, (char*)"", backColor, brightnessBtn);
  btnSettings = Button(290, 0, 30, 30, (char*)"", backColor, settingsBtn);
  //--- Settings Buttons
  //- skip boot logo
  setBtnSwitch1 = Button(345, 55, 30, 30, (char*)"S", backColor, styleBtnSmall);
  //- startup LCD brigthness (default = 255 = 100%)
  setBtnPlus1 = Button(345, 100, 30, 30, (char*)"+", backColor, styleBtnSmall);
  setBtnMinus1 = Button(390, 100, 30, 30, (char*)"-", backColor, styleBtnSmall);
  //- second LCD brigthness (default = 100 = 39%)
  setBtnPlus2 = Button(345, 145, 30, 30, (char*)"+", backColor, styleBtnSmall);
  setBtnMinus2 = Button(390, 145, 30, 30, (char*)"-", backColor, styleBtnSmall);
  //- third LCD brigthness (default = 50 = 20%)
  setBtnPlus3 = Button(345, 190, 30, 30, (char*)"+", backColor, styleBtnSmall);
  setBtnMinus3 = Button(390, 190, 30, 30, (char*)"-", backColor, styleBtnSmall);
  //- play alarms
  setBtnSwitch2 = Button(345, 235, 30, 30, (char*)"S", backColor, styleBtnSmall);

  //initialisiere EEPROM auf 1 Start
  //  byte bytes[2];
  //  bytes[0] = (engineStarts + 1) & 0xFF;
  //  bytes[1] = ((engineStarts + 1) >> 8) & 0xFF;
  //  EEPROM.write(4, bytes[0]);
  //  EEPROM.write(3, bytes[1]);

  //init Homeview
  tft.fillScreen(backColor);
  switchView(HOME_VIEW);

  //start threads
  spiThreadID = threads.addThread(spiThread);
  displayDataThreadID = threads.addThread(displayData);

  //starte Kommunikation mit ECU
  tech2ThreadID = threads.addThread(tech2);

  engineStarts = EEPROM.read(3) * 256 + EEPROM.read(4);

  if (engineStarts % 100 == 0) { //ca. alle 2000 km (100 * ca. 20 km) Meldung geben
    actualInfo = info((char*)"Fluessigkeiten pruefen !", ILI9486_RED, 5, 10000);
  }
}

void loop(void) {
  can500.events();
  can33.events();
}

void spiThread() {
  while (1) {
    //    //---DEBUG FPS---
    //    long start = millis();

    if (dispBufReady) {
      tft.updateScreen();
    }

    //    Serial.println(1000.0f/(float)(millis()-start));
    //    //---DEBUG FPS---

    checkTouch();

    //    //---DEBUG VALS---
    //        if(boost >= 160) boost = 0;
    //        boost += boost/8 + 1;
    //        if(boost > rMAXboost) rMAXboost = boost;
    //        addValue(boostArray, boost);
    //        if(maf >= 900) maf = 0;
    //        maf += maf/8 +1;
    //        addValue(mafArray, maf);
    //        if(iat >= 65) iat = 0;
    //        iat++;
    //    if(speed >= 260) speed = 0, threads.delay(50);
    //    speed++;
    //    speedHandle();
    //    //---DEBUG VALS---

    threads.yield();
  }
}

void checkTouch() {
  if (ts.touched()) {
    //Serial.println("Touched");
    TS_Point p = ts.getPoint();
    //Serial.print((String)p.x + ", " + (String)p.y); //for Calibration
    p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
    p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
    //Serial.println(" = " + (String)p.x + ", " + (String)p.y); //for Calibration

    switch (actualView) {
      case HOME_VIEW:
        if (btnDTCs.isTouched(p.x, p.y)) {
          switchView(FC_VIEW);
        } else if (btnGraph.isTouched(p.x, p.y)) {
          switchView(GRAPH_VIEW);
        } else if (btnMax.isTouched(p.x, p.y)) {
          switchView(MAX_VIEW);
        } else if (btnAcc.isTouched(p.x, p.y)) {
          switchView(ACC_VIEW);
        } else if (btnFuel.isTouched(p.x, p.y)) {
          switchView(FUEL_VIEW);
        } else if (btnLcdBright.isTouched(p.x, p.y)) {
          if (lcdBright == startLcdBright) {
            lcdBright = secondLcdBright;
          } else if (lcdBright == secondLcdBright) {
            lcdBright = thirdLcdBright;
          } else {
            lcdBright = startLcdBright;
          }

          analogWrite(3, lcdBright); //Disp. LED
          threads.delay(250); //avoids bouncing
        } else if (btnSettings.isTouched(p.x, p.y)) {
          switchView(SETT_VIEW);
        }
        break;
      case FC_VIEW:
        if (btnDelDtc.isTouched(p.x, p.y)) {
          clearFCs();
          fcs = 0;
          threads.delay(250); //avoids bouncing
        } else if (btnBack.isTouched(p.x, p.y) || btnHome.isTouched(p.x, p.y)) {
          ecuConnected = false;
          listSet = false;
          tech2ThreadID = threads.addThread(tech2);
          switchView(HOME_VIEW);
        }
        break;
      case GRAPH_VIEW:
        if (btnHold.isTouched(p.x, p.y)) {
          graphHold = !graphHold;
          threads.delay(250); //avoids bouncing
        } else if (btnHome.isTouched(p.x, p.y)) {
          graphHold = false;
          switchView(HOME_VIEW);
        } else if (btnIgnition.isTouched(p.x, p.y)) {
          graphHold = false;
          switchView(IGNITION_VIEW);
        } else if (btnLcdBright.isTouched(p.x, p.y)) {
          if (lcdBright == startLcdBright) {
            lcdBright = secondLcdBright;
          } else if (lcdBright == secondLcdBright) {
            lcdBright = thirdLcdBright;
          } else {
            lcdBright = startLcdBright;
          }

          analogWrite(3, lcdBright); //Disp. LED
          threads.delay(250); //avoids bouncing
        } else if (btnSettings.isTouched(p.x, p.y)) {
          switchView(SETT_VIEW);
        }
        break;
      case IGNITION_VIEW:
        if (btnHold.isTouched(p.x, p.y)) {
          graphHold = !graphHold;
          threads.delay(250); //avoids bouncing
        } else if (btnHome.isTouched(p.x, p.y)) {
          graphHold = false;
          switchView(HOME_VIEW);
        }
        break;
      case MAX_VIEW:
        if (btnResetMax.isTouched(p.x, p.y)) {
          rMAXect = 0;
          rMAXboost = 0;
          rMAXiat = 0;
          rMAXmaf = 0;
          rMAXign = 0;
          rMAXrpm = 0;
          rMAXspeed = 0;
          rMAXinj = 0.0f;
          rMAXinjAuslast = 0;
          rMAXpower = 0;
          rMAXmoment = 0;

          drawMaxView();
          threads.delay(250); //avoids bouncing
        } else if (btnHome.isTouched(p.x, p.y)) {
          graphHold = false;
          switchView(HOME_VIEW);
        }
        break;
      case ACC_VIEW:
        if (btnResetAcc.isTouched(p.x, p.y)) {
          //Beschleunigungszeiten zurücksetzen
          t100Started = false;
          gotT100 = false;
          t100 = 0;
          t200Started = false;
          gotT200 = false;
          t200 = 0;
        } else if (btnPlus.isTouched(p.x, p.y)) { //tachoabw
          tachoabw++;
          threads.delay(250); //avoids bouncing
        } else if (btnMinus.isTouched(p.x, p.y)) { //tachoabw
          tachoabw--;
          threads.delay(250); //avoids bouncing
        } else if (btnHome.isTouched(p.x, p.y)) {
          graphHold = false;
          switchView(HOME_VIEW);
        }
        break;
      case FUEL_VIEW:
        if (btnHome.isTouched(p.x, p.y)) {
          switchView(HOME_VIEW);
        }
        break;
      case SETT_VIEW:
        tft.setFont(Arial_20);
        int pxLen = tft.strPixelLen("Saved!");
        
        if (btnHome.isTouched(p.x, p.y)) {
          switchView(HOME_VIEW);
        } else if (btnSave.isTouched(p.x, p.y)) {
          //save Settings
          byte bytes[5];
          bytes[0] = skipBoot;
          bytes[1] = playAlarms;
          bytes[2] = startLcdBright;
          bytes[3] = secondLcdBright;
          bytes[4] = thirdLcdBright;
        
          EEPROM.write(5, bytes[0]);
          EEPROM.write(6, bytes[1]);
          EEPROM.write(7, bytes[2]);
          EEPROM.write(8, bytes[3]);
          EEPROM.write(9, bytes[4]);

          //show info "saved"          
          tft.fillRoundRect(230 - (pxLen / 2), 0, pxLen + 20, 40, 5, 0xCFF9);
          tft.setTextColor(backColor);
          tft.setFont(Arial_20);
          tft.setCursor(240 - (pxLen / 2), 10);
          tft.print("Saved!");
          
        } else if (setBtnSwitch1.isTouched(p.x, p.y)) {
          skipBoot = !skipBoot;
          threads.delay(250); //avoids bouncing
          tft.fillRoundRect(230 - (pxLen / 2), 0, pxLen + 20, 40, 5, backColor); //kill info "saved"    
        } else if (setBtnSwitch2.isTouched(p.x, p.y)) {
          playAlarms = !playAlarms;
          threads.delay(250); //avoids bouncing
          tft.fillRoundRect(230 - (pxLen / 2), 0, pxLen + 20, 40, 5, backColor); //kill info "saved"
        } else if (setBtnPlus1.isTouched(p.x, p.y)) {
          if(startLcdBright < 255){
            if(lcdBright == startLcdBright) lcdBright++;
            startLcdBright++;
            tft.fillRoundRect(230 - (pxLen / 2), 0, pxLen + 20, 40, 5, backColor); //kill info "saved"
          }
        } else if (setBtnMinus1.isTouched(p.x, p.y)) {
          if(startLcdBright > 1){
            if(lcdBright == startLcdBright) lcdBright--;
            startLcdBright--;
            tft.fillRoundRect(230 - (pxLen / 2), 0, pxLen + 20, 40, 5, backColor); //kill info "saved"
          }
        } else if (setBtnPlus2.isTouched(p.x, p.y)) {
          if(secondLcdBright < 255){
            if(lcdBright == secondLcdBright) lcdBright++;
            secondLcdBright++;
          tft.fillRoundRect(230 - (pxLen / 2), 0, pxLen + 20, 40, 5, backColor); //kill info "saved"
          }
        } else if (setBtnMinus2.isTouched(p.x, p.y)) {
          if(secondLcdBright > 1){
            if(lcdBright == secondLcdBright) lcdBright--;
            secondLcdBright--;
          tft.fillRoundRect(230 - (pxLen / 2), 0, pxLen + 20, 40, 5, backColor); //kill info "saved"
          }
        } else if (setBtnPlus3.isTouched(p.x, p.y)) {
          if(thirdLcdBright < 255){
            if(lcdBright == thirdLcdBright) lcdBright++;
            thirdLcdBright++;
          tft.fillRoundRect(230 - (pxLen / 2), 0, pxLen + 20, 40, 5, backColor); //kill info "saved"
          }
        } else if (setBtnMinus3.isTouched(p.x, p.y)) {
          if(thirdLcdBright > 1){
            if(lcdBright == thirdLcdBright) lcdBright--;
            thirdLcdBright--;
          tft.fillRoundRect(230 - (pxLen / 2), 0, pxLen + 20, 40, 5, backColor); //kill info "saved"
          }
        }
        analogWrite(3, lcdBright); //Disp. LED
        break;
    }
  }
}

void displayData() {
  while (1) {
    consume = getConsum();
    if (consume > rMAXconsume) rMAXconsume = consume;
    afr = getAFR();

    //r-gang-abfrage
    if ((millis() - lastRGang >= 3000 && rGang) || (rOut && rGang)) { //R-Gang länger als 1 Sec aus
      //Musik wieder etwas lauter machen
      //175 0 0 0 0 0 1 0 1 33 - Befehl "Radio lauter (Fernbedienung)"
      sendBus33Data(0x175, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01);
      threads.delay(100);
      sendBus33Data(0x175, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01);
      threads.delay(100);
      sendBus33Data(0x175, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01);
      threads.delay(100);
      sendBus33Data(0x175, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01);
      threads.delay(100);
      sendBus33Data(0x175, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01);
      threads.delay(100);
      sendBus33Data(0x175, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01);
      threads.delay(100);
      sendBus33Data(0x175, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01);
      rGang = false;
      rOut = false;
    }

    dispBufReady = false;
    if (actualView == HOME_VIEW) {
      drawHomeView();
      askInfo(); //Zeigt Infofeld falls notwendig
      drawInfo();
    } else if (actualView == GRAPH_VIEW) {
      //drawInfo(actualInfo);
      drawGraphView();
      //askInfo(); //Zeigt Infofeld falls notwendig
    } else if (actualView == IGNITION_VIEW) {
      //drawInfo(actualInfo);
      drawIgnitionView();
      //askInfo(); //Zeigt Infofeld falls notwendig
    } else if (actualView == FC_VIEW) {
      //Fehlercodes anzeigen
      threads.delay(500);
    } else if (actualView == MAX_VIEW) {
      drawMaxView();
      threads.delay(500);
    } else if (actualView == ACC_VIEW) {
      drawAccView();
      threads.delay(20);
    } else if (actualView == FUEL_VIEW) {
      drawFuelView();
    } else if (actualView == SETT_VIEW) {
      drawSettingsView();
    }
    dispBufReady = true;

    threads.yield();
  }
}

void switchView(int view) {
  actualView = view;
  
  threads.kill(displayDataThreadID);
  tft.fillScreen(backColor);

  if (actualView == HOME_VIEW) {
    btnDTCs.drawButton(true, tft);
    btnGraph.drawButton(true, tft);
    btnMax.drawButton(true, tft);
    btnAcc.drawButton(true, tft);
    btnFuel.drawButton(true, tft);
    drawHomeView();

  } else if (actualView == GRAPH_VIEW) {
    btnHome.drawButton(true, tft);
    btnHold.drawButton(true, tft);
    btnIgnition.drawButton(true, tft);
    drawGraphView();

  } else if (actualView == IGNITION_VIEW) {
    btnHome.drawButton(true, tft);
    btnHold.drawButton(true, tft);
    drawGraphView();

  } else if (actualView == FC_VIEW) {
    tft.writeRect(420, 2, 60, 23, (uint16_t*)opc23px);
    btnBack.drawButton(true, tft);
    btnDelDtc.drawButton(true, tft);

    tft.setCursor(0, 38); tft.setFont(Arial_12); tft.setTextColor(foreColor);
    tft.updateScreen();

    requestFCs();

  } else if (actualView == MAX_VIEW) {
    tft.writeRect(420, 2, 60, 23, (uint16_t*)opc23px);
    tft.setCursor(0, 0); tft.setFont(Arial_20); tft.setTextColor(foreColor);
    tft.print((const char*)"Maximalwerte");
    btnHome.drawButton(true, tft);
    btnResetMax.drawButton(true, tft);
    drawMaxView();

  } else if (actualView == ACC_VIEW) {
    tft.writeRect(420, 2, 60, 23, (uint16_t*)opc23px);
    tft.setCursor(0, 0); tft.setFont(Arial_20); tft.setTextColor(foreColor);
    tft.print((const char*)"Acceleration Monitor");
    btnHome.drawButton(true, tft);
    btnResetAcc.drawButton(true, tft);
    btnMinus.drawButton(true, tft);
    btnPlus.drawButton(true, tft);
    drawAccView();

  } else if (actualView == FUEL_VIEW) {
    tft.writeRect(420, 2, 60, 23, (uint16_t*)opc23px);
    tft.setCursor(0, 0); tft.setFont(Arial_20); tft.setTextColor(foreColor);
    tft.print((const char*)"Fuel Monitor");
    btnHome.drawButton(true, tft);
    drawFuelView();
    
  } else if (actualView == SETT_VIEW) {
    tft.writeRect(420, 2, 60, 23, (uint16_t*)opc23px);
    tft.setCursor(0, 0); tft.setFont(Arial_20); tft.setTextColor(foreColor);
    tft.print((const char*)"Settings");
    btnHome.drawButton(true, tft);
    btnSave.drawButton(true, tft);
    
    setBtnSwitch1.drawButton(true, tft);
    setBtnPlus1.drawButton(true, tft);
    setBtnMinus1.drawButton(true, tft);
    setBtnPlus2.drawButton(true, tft);
    setBtnMinus2.drawButton(true, tft);
    setBtnPlus3.drawButton(true, tft);
    setBtnMinus3.drawButton(true, tft);
    setBtnSwitch2.drawButton(true, tft);
    
    drawSettingsView();
  }
  
  threads.delay(250); //avoids bouncing
  displayDataThreadID = threads.addThread(displayData);
}

//-------------------------------------------------------------//
//----------------------------Views----------------------------//
//-------------------------------------------------------------//
void drawHeadLine() {
  //separator1
  tft.drawLine(128,0,128,30,0xFF4A);
  tft.drawLine(129,0,129,30,0xFF4A);
  //separator2
  tft.drawLine(351,0,351,30,0xFF4A);
  tft.drawLine(352,0,352,30,0xFF4A);
  
  tft.setTextColor(foreColor);
  tft.setFont(Arial_12);
  tft.setCursor(3, 9);

  if (!rGang) { //Wenn R-Gang nicht drin ist, zeige Batteriespannung
    //battery voltage
    tft.print("BATT ");
    tft.setFont(Arial_14);
    tft.setCursor(3 + 46, 8);
    tft.print(vBatt);
    tft.print(" V");
  } else { //Wenn R-Gang drin ist, zeige Abstand
    //distance behind
    tft.print("DIST ");
    tft.setFont(Arial_14);
    tft.setCursor(3 + 43, 8);
    if (distanceBehind <= 19)
      tft.print("< 19");
    else if (distanceBehind >= 255)
      tft.print("> 100");
    else
      tft.print(distanceBehind);
    tft.print(" cm");
  }

  //Außentemperatur
  tft.setFont(Arial_12);
  tft.setCursor(360, 9);
  tft.print("OUT ");
  tft.setFont(Arial_14);
  tft.setCursor(412, 8);
  tft.print(aat, 1);
  tft.print(" *C");

  //header buttons
  btnLcdBright.drawButton(true, tft);
  btnSettings.drawButton(true, tft);
}

void drawMaxView() {
  tft.fillRect(215, 35, 50, 250, 0x0000);//backColor

  tft.setFont(Arial_12);
  tft.setTextColor(foreColor);

  int y = 36;
  tft.setCursor(2, y);
  tft.print("Motortemperatur");
  tft.setCursor(215, y);
  tft.print(rMAXect);
  tft.setCursor(270, y);
  tft.print("*C");

  y += 21;
  tft.setCursor(2, y);
  tft.print("Saugrohrdruck");
  tft.setCursor(215, y);
  tft.print(rMAXboost);
  tft.setCursor(270, y);
  tft.print("kPa");

  y += 21;
  tft.setCursor(2, y);
  tft.println("Ladelufttemp.");
  tft.setCursor(215, y);
  tft.println(rMAXiat);
  tft.setCursor(270, y);
  tft.println("*C");

  y += 21;
  tft.setCursor(2, y);
  tft.println("Motordrehzahl");
  tft.setCursor(215, y);
  tft.println(rMAXrpm);
  tft.setCursor(270, y);
  tft.println("U / min");

  y += 21;
  tft.setCursor(2, y);
  tft.println("Zuend-Fruehverstellung");
  tft.setCursor(215, y);
  tft.println(rMAXign);
  tft.setCursor(270, y);
  tft.println("* vor OT");

  y += 21;
  tft.setCursor(2, y);
  tft.println("Luftmasse");
  tft.setCursor(215, y);
  tft.println(rMAXmaf);
  tft.setCursor(270, y);
  tft.println("kg / h");

  y += 21;
  tft.setCursor(2, y);
  tft.println("Injektorauslastung");
  tft.setCursor(215, y);
  tft.println(rMAXinjAuslast);
  tft.setCursor(270, y);
  tft.println("%");

  //  y += 21;
  //  tft.setCursor(2, y);
  //  tft.println("Einspritzzeit");
  //  tft.setCursor(215, y);
  //  tft.println(rMAXinj);
  //  tft.setCursor(270, y);
  //  tft.println("ms");

  y += 21;
  tft.setCursor(2, y);
  tft.println("Motorstarts");
  tft.setCursor(215, y);
  tft.println(engineStarts);
  tft.setCursor(270, y);
  tft.println("Starts");

  y += 21;
  tft.setCursor(2, y);
  tft.println("Power");
  tft.setCursor(215, y);
  tft.println(rMAXpower);
  tft.setCursor(270, y);
  tft.println("PS");

  y += 21;
  tft.setCursor(2, y);
  tft.println("Drehmoment");
  tft.setCursor(215, y);
  tft.println(rMAXmoment);
  tft.setCursor(270, y);
  tft.println("Nm");

  y += 21;
  tft.setCursor(2, y);
  tft.println("Verbrauch");
  tft.setCursor(215, y);
  tft.println(rMAXconsume);
  tft.setCursor(270, y);
  tft.println("l / h");

  y += 21;
  tft.setCursor(2, y);
  tft.println("Geschwindigkeit");
  tft.setCursor(215, y);
  tft.println(rMAXspeed);
  tft.setCursor(270, y);
  tft.println("km / h");
}

void drawHomeView() {
  tft.fillRect(0, 0, 480, 33, backColor);

  drawHeadLine();

  //--------- First Row Y = 33
  oldect = ect;
  //gtrMeter(val, min, max, x , y , unit, crit, title, maxVal, decis, alarm)
  gtrMeter(oldect, 40, 120, 0, 33, (char*)"*C", 110, (char*)"WATER TEMP", rMAXect, 0, true); //Motortemp

  oldboost = boost;
  gtrMeter(oldboost, 0, 200, 162, 33, (char*)"kPa", 130, (char*)"BOOST", rMAXboost, 0, false); //Ladedruck

  oldmaf = maf;
  gtrMeter(oldmaf, 0, 900, 324, 33, (char*)"kg/h", 800, (char*)"AIR FLOW", rMAXmaf, 0, false); //Luftmasse

  //--------- Second Row Y = 156
  oldiat = iat;
  gtrMeter(oldiat, 0, 80, 0, 156, (char*)"*C", 70, (char*)"BOOST TEMP", rMAXiat, 0, true); //Ladelufttemp

  oldrpm = rpm;
  gtrMeter(oldrpm, 0, 8000, 162, 156, (char*)"rpm", 6500, (char*)"ENGINE SPEED", rMAXrpm, 0, true);

  oldinjAuslast = injAuslast;
  gtrMeter(oldinjAuslast, 0, 100, 324, 156, (char*)"%", 95, (char*)"DUTY CYCLE", rMAXinjAuslast, 0, true);
}

void drawGraphView() {
  tft.fillRect(0, 0, 480, 33, backColor);

  drawHeadLine();

  //--------- First Row Y = 33
  oldect = ect;
  gtrMeter(oldect, 40, 120, 0, 33, (char*)"*C", 110, (char*)"WATER TEMP", rMAXect, 0, true); //Motortemp

  if (!graphHold) {
    //drawGraph(int x, int y, int w, int h, int vmin, int vmax, char *title,        int *values)
    drawGraph(162,   33,    313,   119,   0,        160,      (char*)"BOOST kPa", boostArray); //Ladedruck
  }

  //--------- Second Row Y = 156
  oldiat = iat;
  gtrMeter(oldiat, 0, 80, 0, 156, (char*)"*C", 70, (char*)"BOOST TEMP", rMAXiat, 0, true); //Ladelufttemp

  if (!graphHold) {
    drawGraph(162, 156, 313, 119, 0, 900, (char*)"AIR FLOW kg/h", mafArray); //Ladedruck
  }
}

void drawIgnitionView() {
  float lambda = getLambda(); // afr / 14.7f;

  tft.fillRect(0, 0, 480, 320 - 43, backColor);

  tft.setTextColor(foreColor);
  tft.setFont(Arial_12);
  tft.setCursor(3, 12);
  tft.print("Lambda ");
  tft.setCursor(3 + 93, 10);
  tft.setFont(Arial_14);
  tft.print(lambda, 2);
  //tft.print(" V");


  tft.setFont(Arial_12);
  tft.setCursor(326, 12);
  tft.print("Noise ");
  tft.setCursor(326 + 86, 10);
  if (knockVal > getEngineNoiseLimit()) tft.setTextColor(0xF800); //red value
  tft.setFont(Arial_14);
  tft.print(knockVal, 2);
  tft.print(" V");
  tft.setTextColor(foreColor);

  //drawBar(int x, int y, char *title, int vMin, int vMax, int vYellow, int vRed, float value)
  drawBar(10, 35, (char*)"IAA [*CA]", -4, 22, 16, 18, ign); //IAA = Ignition Advance Angle

  drawBar(109, 35, (char*)"KR1 [*CA]", 0, 18, 3, 5, knockRet1); //KRx = Knock Retard Angle Cylinder x
  drawBar(208, 35, (char*)"KR2 [*CA]", 0, 18, 3, 5, knockRet2); //KRx = Knock Retard Angle Cylinder x
  drawBar(307, 35, (char*)"KR3 [*CA]", 0, 18, 3, 5, knockRet3); //KRx = Knock Retard Angle Cylinder x
  drawBar(406, 35, (char*)"KR4 [*CA]", 0, 18, 3, 5, knockRet4); //KRx = Knock Retard Angle Cylinder x
}

void drawAccView() {
  //Aktuelle Geschwindigkeit
  oldspeed = speed;
  //gtrMeter(val, min, max, x , y , unit, crit, title, maxVal, decis)
  gtrMeter(oldspeed, 0, 260, 0, 33, (char*)"km/h", 255, (char*)"SPEED", rMAXspeed, 0, false);

  //Aktuelle Drehzahl
  oldrpm = rpm;
  gtrMeter(oldrpm, 0, 7000, 0, 156, (char*)"rpm", 6500, (char*)"ENGINE SPEED", rMAXrpm, 0, false);

  tft.fillRect(170, 41, 160, 120, backColor);

  //0-100 Time, mit Korrektur
  tft.setFont(Arial_14);
  tft.setTextColor(foreColor);
  tft.setCursor(170, 41);
  tft.print("0 - 100 km/h");

  tft.setFont(Arial_20);
  tft.setCursor(170, 62);
  tft.print(t100 / 1000.0f, 2); //t100 = Time in Millis
  tft.print(" sec");

  //100-200 Time, mit Korrektur
  tft.setFont(Arial_14);
  tft.setTextColor(foreColor);
  tft.setCursor(170, 106);
  tft.print("100 - 200 km/h");

  tft.setFont(Arial_20);
  tft.setCursor(170, 127);
  tft.print(t200 / 1000.0f, 2); //t200 = Time in Millis
  tft.print(" sec");

  //Abweichung
  tft.setFont(Arial_14);
  tft.setTextColor(foreColor);
  tft.setCursor(170, 195);
  tft.print("Tachoabweichung");
  tft.setFont(Arial_20);
  tft.fillRect(170, 216, 80, 40, backColor);
  tft.setCursor(170, 216);
  tft.print(tachoabw);
  tft.print(" %");

}

void drawFuelView() {
  //--------- First Row Y = 33
  gtrMeter(tankInhalt/*Liter im Tank*/, 0/*min*/, 52/*max*/, 0/*x*/, 33/*y*/, (char*)"L"/*unit*/, -1/*criticalV*/, (char*)"FUEL TANK"/*title*/, -1/*maxVal*/, 1/*decimalPlaces*/, false);

  if (!graphHold) {
    drawGraph(162, 33, 313, 119, 0, 120, (char*)"ACTUAL CONSUME l/h", consumeArray); //Verbrauch
  }
  int y = 160;
  tft.fillRect(0, y, 480, 120, backColor);

  tft.setFont(Arial_14);
  tft.setTextColor(foreColor);
  tft.setCursor(0, y);
  tft.print("Durchschnittsverbrauch");
  tft.setCursor(270, y);
  if (absolutConsume > 0 && strecke > 0)
    tft.print(absolutConsume / strecke * 100.0f, 1);
  else
    tft.print("0.0");
  tft.print(" L/100km");

  y += 25;
  tft.setCursor(0, y);
  tft.print("Absolutverbrauch");
  tft.setCursor(270, y);
  tft.print(absolutConsume, 1);
  tft.print(" L/Trip");

  float runningTime = getEngineRunningMinutes();

  y += 25;
  tft.setCursor(0, y);
  tft.print("Durchschnittsgeschwindigkeit");
  tft.setCursor(270, y);
  if (strecke > 0 && runningTime > 0) tft.print((float)strecke / (runningTime / 60.0f), 2);
  else tft.print("0.00");
  tft.print(" km/h");

  y += 25;
  tft.setCursor(0, y);
  tft.print("Fahrstrecke");
  tft.setCursor(270, y);
  tft.print(strecke, 2);
  tft.print(" km");

  y += 25;
  tft.setCursor(0, y);
  tft.print("Fahrzeit");
  tft.setCursor(270, y);
  tft.print(runningTime, 0);
  tft.print(" min");
}

void drawSettingsView() {
  tft.fillRect(120, 50, 185, 210, backColor);

  //skip boot logo (default = false)
  tft.setFont(Arial_14); tft.setTextColor(foreColor);
  tft.setCursor(5, 60);
  tft.print("Skip boot logo =  ");
  tft.setTextColor(0xFF4A);
  if(skipBoot) tft.print("true");
  else tft.print("false");
  
  //startup LCD brigthness (default = 255 = 100%)
  tft.setFont(Arial_14); tft.setTextColor(foreColor);
  tft.setCursor(5, 105);
  tft.print("Startup LCD brightness =  ");
  tft.setTextColor(0xFF4A);
  tft.print(startLcdBright / 2.55f, 0);
  tft.print(" %");

  //second LCD brigthness (default = 100 = 39%)
  tft.setFont(Arial_14); tft.setTextColor(foreColor);
  tft.setCursor(5, 150);
  tft.print("Second LCD brightness =  ");
  tft.setTextColor(0xFF4A);
  tft.print(secondLcdBright / 2.55f, 0);
  tft.print(" %");

  //third LCD brigthness (default = 50 = 20%)
  tft.setFont(Arial_14); tft.setTextColor(foreColor);
  tft.setCursor(5, 195);
  tft.print("Third LCD brightness =  ");
  tft.setTextColor(0xFF4A);
  tft.print(thirdLcdBright / 2.55f, 0);
  tft.print(" %");

  //play critical alarms (default = true)
  tft.setFont(Arial_14); tft.setTextColor(foreColor);
  tft.setCursor(5, 240);
  tft.print("Play alarms =  ");
  tft.setTextColor(0xFF4A);
  if(playAlarms) tft.print("true");
  else tft.print("false");

}
//-------------------------------------------------------------//
//--------------------------InfoPopUp--------------------------//
//-------------------------------------------------------------//
void drawInfo() {
  if (actualInfo.Prio >= oldInfo.Prio || millis() - infoWritten >= oldInfo.Dur) { //wenn neu info-prio höher als alte info-prio, oder alte Info abgelaufen
    if (actualView == HOME_VIEW) { //wenn homeView & neue Info // && theInfo != oldInfo
      oldInfo = actualInfo;
      if (actualInfo.enabled) {
        tft.setFont(Arial_20);
        int pxLen = tft.strPixelLen(actualInfo.Text);
        tft.fillRoundRect(230 - (pxLen / 2), 140, pxLen + 20, 40, 5, foreColor);
        tft.setTextColor(actualInfo.TextColor);
        tft.setFont(Arial_20);
        tft.setCursor(240 - (pxLen / 2), 150); //middle screen
        tft.print(actualInfo.Text);

        if (!actualInfo.firstDrawn) {
          infoWritten = millis();
          actualInfo.firstDrawn = true;
          //Serial.print("Info drawn at ");
          //Serial.println(infoWritten);
        }

        if (millis() - infoWritten >= actualInfo.Dur) {
          actualInfo.enabled = false;
          tft.fillRect(0, 140, 480, 40, backColor);
          //Serial.print("Info disabled at ");
          //Serial.println(millis());
        }
      }
    }
  }
}

void askInfo() {
  //Wichtige Info vorhanden? Dann zeige Info
  if (waschwasserLeer && !waschwasserLeerOk) {
    actualInfo = info((char*)"Waschwasser leer !", foreColor, 3, 500);
  }
}

//-------------------------------------------------------------//
//------------------------Calculations-------------------------//
//-------------------------------------------------------------//
float getConsum() {
  float injSize = 470.0f;  // [ccm/min] @ injPress
  float injPress = 300.0f; // [kPa]
  float newSize = 470.0f; // [ccm/min] @ injPress
  float injCal = 0.57; // injectors battery offset calibration value

  //calculate injectors flow rate
  if (rpm > 1000) {
    newSize = injSize * sqrt((injPress + (float)boost) / injPress);
  } else {
    newSize = injSize * sqrt((injPress - 70.0f) / injPress); // -70kPa Saugrogrdruck im Leerlauf
  }

  //injectors battery offset calibration
  if (vBatt > 14) {
    //0.72 @ 14V, 0.63 @ 15V
    injCal = map(vBatt, 14, 15, 0.72, 0.63);
  } else if (vBatt > 13) {
    //0.84 @ 13V, 0.72 @ 14V
    injCal = map(vBatt, 13, 14, 0.84, 0.72);
  } else if (vBatt >= 12) {
    //0.97 @ 12V, 0.84 @ 13V
    injCal = map(vBatt, 12, 13, 0.97, 0.84);
  } else if (vBatt < 12) {
    //1.13 @ 11V, 0.97 @ 12V
    injCal = map(vBatt, 11, 12, 1.13, 0.97);
  }

  float consum = (inj - injCal) * (float)rpm * newSize * 0.12f / 60000.0f;
  if (consum < 0) consum = 0.0f;

  //calculate absolute consume
  absolutConsume += consum / 3600000.0f * (float)(millis() - lastConsumeTime); //[l/h] / ms = [l/ms] * delta[ms] = l/deltaTime
  lastConsumeTime = millis();

  addValue(consumeArray, consum);

  return consum;
}

float getAFR() {
  float afrC = 14.7f;
  float rohBenzin = 0.759f; //[kg/l] @ 0°C
  rohBenzin = rohBenzin * (1.0f - (float)ambientTemp * 0.001f);// Wärmeausdehnung
  float kgBenzin = rohBenzin * consume;

  afrC = maf / kgBenzin;

  return afrC;
}

float getLambda() {
  //lookup table from https://www.lotustalk.com/attachments/oxygen-sensor-conversion-table-png.1030577/
  float lmbdaCalc = lambdaVal * 200.0f;

  float lambda = 0;

  if (lmbdaCalc >= 193) {
    lambda = 0.70f;
  } else if (lmbdaCalc >= 189) {
    lambda = map(lmbdaCalc, 189, 193, 74, 70) / 100.0f;
  } else if (lmbdaCalc >= 184) {
    lambda = map(lmbdaCalc, 184, 189, 78, 74) / 100.0f;
  } else if (lmbdaCalc >= 180) {
    lambda = map(lmbdaCalc, 180, 184, 81, 78) / 100.0f;
  } else if (lmbdaCalc >= 176) {
    lambda = map(lmbdaCalc, 176, 180, 85, 81) / 100.0f;
  } else if (lmbdaCalc >= 172) {
    lambda = map(lmbdaCalc, 172, 176, 88, 85) / 100.0f;
  } else if (lmbdaCalc >= 168) {
    lambda = map(lmbdaCalc, 168, 172, 92, 88) / 100.0f;
  } else if (lmbdaCalc >= 156) {
    lambda = map(lmbdaCalc, 156, 168, 96, 92) / 100.0f;
  } else if (lmbdaCalc >= 143) {
    lambda = map(lmbdaCalc, 143, 156, 98, 96) / 100.0f;
  } else if (lmbdaCalc >= 92) {
    lambda = map(lmbdaCalc, 92, 143, 100, 98) / 100.0f;
  } else if (lmbdaCalc >= 74) {
    lambda = map(lmbdaCalc, 74, 92, 101, 100) / 100.0f;
  } else if (lmbdaCalc >= 16) {
    lambda = map(lmbdaCalc, 16, 74, 104, 101) / 100.0f;
  } else if (lmbdaCalc >= 12) {
    lambda = map(lmbdaCalc, 12, 16, 106, 104) / 100.0f;
  } else if (lmbdaCalc >= 8) {
    lambda = map(lmbdaCalc, 8, 12, 110, 106) / 100.0f;
  } else if (lmbdaCalc >= 4) {
    lambda = map(lmbdaCalc, 4, 8, 115, 110) / 100.0f;
  } else if (lmbdaCalc >= 0) {
    lambda = map(lmbdaCalc, 0, 4, 120, 115) / 100.0f;
  }

  return lambda;
}

float getEngineRunningMinutes() {
  float runningTime;
  if (EngineStartTime == 0) runningTime = 0.0f;
  else runningTime = (float)(millis() - EngineStartTime) / 60000.0f;

  return runningTime;
}

float getEngineNoiseLimit() {
  float noiseLimit = 0;

  if (rpm > 5710) {
    noiseLimit = 2.61;
  } else if (rpm > 5328) {
    noiseLimit = map(rpm, 5328, 5710, 2.51, 2.61);
  } else if (rpm > 4947) {
    noiseLimit = map(rpm, 4947, 5328, 2.41, 2.51);
  } else if (rpm > 4566) {
    noiseLimit = map(rpm, 4566, 4947, 2.22, 2.41);
  } else if (rpm > 4183) {
    noiseLimit = map(rpm, 4183, 4566, 2.00, 2.22);
  } else if (rpm > 3802) {
    noiseLimit = map(rpm, 3802, 4183, 1.82, 2.00);
  } else if (rpm > 3421) {
    noiseLimit = map(rpm, 3421, 3802, 1.61, 1.82);
  } else if (rpm > 3039) {
    noiseLimit = map(rpm, 3039, 3421, 1.45, 1.61);
  } else if (rpm > 2657) {
    noiseLimit = map(rpm, 2657, 3039, 1.35, 1.45);
  } else if (rpm > 1894) {
    noiseLimit = map(rpm, 1894, 2657, 1.39, 1.35);
  } else if (rpm > 750) {
    noiseLimit = map(rpm, 750, 1894, 1.37, 1.39);
  }

  return noiseLimit;
}

void speedHandle() {
  actSpeed = speed;
  if (actSpeed > 0) {
    actSpeed = (actSpeed * (100 - tachoabw)) / 100;
  }

  if (EngineStartTime > 0) { //Motor Läuft, Strecke berechnen
    double span = millis() - lastSpeedTime; //Zeit zwischen zwei Geschwindigkeitsangaben
    odoMeter += (double)actSpeed / 3600000.0d * span;
    strecke += (double)actSpeed / 3600000.0d * span;
    lastSpeedTime = millis();
  }

  if (oldactSpeed - actSpeed >= 3) { //Fzg wird langsamer
    //Beschleunigungszeiten zurücksetzen
    t100Started = false;
    t200Started = false;
  }

  if (!gotT100) {
    if (t100Started) {
      //Uhr läuft
      if (actSpeed >= 100) {
        //Beschleunigung gestartet
        t100Stop = millis();
        t100 = t100Stop - t100Start;
        gotT100 = true;
      }
    } else {
      //Noch nicht gestartet
      if (oldactSpeed <= 0 && actSpeed >= 1) {
        //Beschleunigung gestartet
        t100Start = millis();
        t100Started = true;
      }
    }
  }

  if (!gotT200) {
    if (t200Started) {
      //Uhr läuft
      if (actSpeed >= 200) {
        //Beschleunigung gestartet
        t200Stop = millis();
        t200 = t200Stop - t200Start;
        gotT200 = true;
      }
    } else {
      //Noch nicht gestartet
      if (oldactSpeed <= 99 && actSpeed >= 100) {
        //Beschleunigung gestartet
        t200Start = millis();
        t200Started = true;
      }
    }
  }
  oldactSpeed = actSpeed;
}

//-------------------------------------------------------------//
//--------------------------Drawables--------------------------//
//-------------------------------------------------------------//
void drawGraph(int x, int y, int w, int h, int vmin, int vmax, char *title, int *values) {
  //int w = 313; //273 / 7 = 39
  //int h = 119; //80
  int tXsize;

  //Background
  //tft.writeRect(x, y, w, h, graph_bg);
  tft.fillRect(x, y, w, h, backColor); //for debug
  tft.fillRectVGradient(x + 2, y + 2, w - 4, 62, 0x3186, backColor);
  tft.fillRectVGradient(x + 2, y + 95, w - 4, 22, backColor, 0x3186);
  tft.drawRoundRect(x, y, w, h, 5, 0x5AEB);
  tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 5, 0x5AEB);

  //Title
  tft.setTextColor(foreColor);
  tft.setFont(Arial_12_Bold);
  tXsize = tft.strPixelLen(title);
  tft.setCursor(x + w / 2 - (tXsize / 2), y + 101);
  tft.print(title);

  //x and y axis
  tft.drawLine(x + 37, y + 7, x + 37, y + 96, 0x632D); //vertical grey line
  tft.drawLine(x + 38, y + 7, x + 38, y + 96, 0x632D); //vertical grey line
  tft.drawLine(x + 37, y + 96, x + w - 10, y + 96, 0x632D); //horizontal grey line
  tft.drawLine(x + 37, y + 97, x + w - 10, y + 97, 0x632D); //horizontal grey line
  //white segments
  tft.drawLine(x + 37, y + 12, x + 44, y + 12, foreColor); //white segment
  tft.drawLine(x + 37, y + 33, x + 44, y + 33, foreColor); //white segment
  tft.drawLine(x + 37, y + 54, x + 44, y + 54, foreColor); //white segment
  tft.drawLine(x + 37, y + 75, x + 44, y + 75, foreColor); //white segment
  tft.drawLine(x + 37, y + 96, x + 44, y + 96, foreColor); //white segment
  //grey segments
  tft.drawLine(x + 45, y + 12, x + w - 10, y + 12, 0x632D); //grey segment
  tft.drawLine(x + 45, y + 33, x + w - 10, y + 33, 0x632D); //grey segment
  tft.drawLine(x + 45, y + 54, x + w - 10, y + 54, 0x632D); //grey segment
  tft.drawLine(x + 45, y + 75, x + w - 10, y + 75, 0x632D); //grey segment
  tft.drawLine(x + 45, y + 96, x + w - 10, y + 96, 0x632D); //grey segment
  //segment vals
  tft.setTextColor(foreColor);
  tft.setFont(Arial_10);
  tXsize = tft.strPixelLen(intToChar(vmax));
  tft.setCursor(x + 34 - (tXsize), y + 8);
  tft.print(vmax);
  int valSegment = (float)(vmax - vmin) / 4.0f;
  tXsize = tft.strPixelLen(intToChar(vmin + 3 * valSegment));
  tft.setCursor(x + 34 - (tXsize), y + 8 + 1 * 21);
  tft.print(vmin + 3 * valSegment);
  tXsize = tft.strPixelLen(intToChar(vmin + 2 * valSegment));
  tft.setCursor(x + 34 - (tXsize), y + 8 + 2 * 21);
  tft.print(vmin + 2 * valSegment);
  tXsize = tft.strPixelLen(intToChar(vmin + 1 * valSegment));
  tft.setCursor(x + 34 - (tXsize), y + 8 + 3 * 21);
  tft.print(vmin + 1 * valSegment);
  tXsize = tft.strPixelLen(intToChar(vmin));
  tft.setCursor(x + 34 - (tXsize), y + 8 + 4 * 21);
  tft.print(vmin);



  //Graph
  int lineWidth = w - 45; //268
  int xOffset = x + 41;
  int yOffset = y + 96; //=132
  int _x = 0;
  int _y = 0;
  int _y2 = 0;
  float xIncrement = (float)lineWidth / (float)arrayLength; //abstand zwischen x-punkten
  float yDivisor = (float)(vmax - vmin) / 84.0f;
  for (int i = 0; i < arrayLength - 1; i++) {
    _x = lineWidth - xIncrement * (float)i + xOffset - xIncrement;
    _y = (float)values[i] / yDivisor;
    _y2 = (float)values[i + 1] / yDivisor;

    tft.drawLine(_x, yOffset - _y, _x - xIncrement, yOffset - _y2, 0x07E0);
    tft.drawLine(_x, yOffset - _y + 1, _x - xIncrement, yOffset - _y2 + 1, 0x07E0);
  }

}

void gtrMeter(float value, int vmin, int vmax, int x, int y, char *units, int vCritical, char *title, int maxVal, int decis, bool alarm) {
  int w = 154;
  int h = 119;

  //Hintergrund
  tft.writeRect(x, y, 154, 119, gauge_bg);

  //Segment Values
  float valSegment = (float)(vmax - vmin) / 8.0f;
  int tXsize;
  float segval;
  bool multipl = false;
  tft.setTextColor(foreColor);
  tft.setFont(Arial_11);

  //segval 8
  segval = vmax;
  if (segval > 1000) segval /= 100.0f, multipl = true;
  tXsize = tft.strPixelLen(floatToChar(segval, 0));
  tft.setCursor(x + 141 - tXsize / 2, y + 85);
  tft.print(segval, 0);

  //segval 6
  tft.setCursor(x + 125, y + 26);
  segval = valSegment * 6.0f + (float)vmin;
  if (multipl) segval /= 100.0f;
  tft.print(segval, 0);


  //segval 4
  segval = valSegment * 4.0f + (float)vmin;
  if (multipl) segval /= 100.0f;
  tXsize = tft.strPixelLen(floatToChar(segval, 0));
  tft.setCursor(x + 80 - tXsize / 2, y + 5);
  tft.print(segval, 0);

  //segval 2
  segval = valSegment * 2.0f + (float)vmin;
  if (multipl) segval /= 100.0f;
  tXsize = tft.strPixelLen(floatToChar(segval, 0));
  tft.setCursor(x + 35 - tXsize, y + 26);
  tft.print(segval, 0);

  //segval 0
  segval = vmin;
  if (multipl && segval != 0) segval /= 100.0f;
  tXsize = tft.strPixelLen(floatToChar(segval, 0));
  tft.setCursor(x + 16 - tXsize / 2, y + 85);
  tft.print(segval, 0);


  if (multipl) {
    tft.setCursor(x + 119, y + 5);
    tft.print("x100");
  }


  int v = map(value, vmin, vmax, 0, 180); // Map the value to an angle v
  int vC = map(vCritical, vmin, vmax, 0, 180); // Map the vCritical to an angle vC
  int vMax = map(maxVal, vmin, vmax, 0, 180); // Map the vMax to an angle vMax
  if (v < 0)v = 0;
  else if (v > 180) v = 180;

  //value-ring
  tft.drawValRing(x + 28, y + 31, 100, 51, val_ring, v);

  //max val pointer
  if (maxVal >= vmin) {
    int x1, y1;
    x1 = (float)x + (float)w / 2.0f + cos((((float)vMax - 180.0f) * PI) / 180.0f) * 56.0f;
    y1 = (float)y + (float)h - 37.0f + sin((((float)vMax - 180.0f) * PI) / 180.0f) * 56.0f;
    tft.fillCircle(x1, y1 - 1, 2, 0xFFE0); //0xFFE0 = gelb
  }


  // Calculate coords of centre of ring
  x += w / 2; y += h / 2;

  // Convert value to a string
  char buf[10];
  byte len = 2; if (value > 9) len = 3; if (value > 99) len = 4; if (value > 999) len = 5;
  dtostrf(value, len, decis, buf);

  if (v >= vC && vCritical >= 0) {
    tft.setTextColor(ILI9486_RED);
  }
  else tft.setTextColor(foreColor);

  // Print value
  tft.setFont(Arial_20);
  if (decis == 0)tXsize = tft.strPixelLen(intToChar(value)), tft.setCursor(x - (tXsize / 2) - 7, y + h / 2 - 45);
  else tXsize = tft.strPixelLen(buf), tft.setCursor(x - (tXsize / 2), y + h / 2 - 45);
  tft.print(buf);

  tft.setTextColor(foreColor);
  tft.setFont(Arial_12_Bold);
  tXsize = tft.strPixelLen(title);
  tft.setCursor(x - (tXsize / 2), y + h / 2 - 18);
  tft.print(title);

  tft.setTextColor(foreColor);
  tXsize = tft.strPixelLen(units);
  tft.setCursor(x - (tXsize / 2), y - h / 2 + 52);
  tft.print(units);

  if (v >= vC && vCritical >= 0 && alarm) {
    alarmThreadID = threads.addThread(playAlarm);    
  }else{
    if(alarmThreadID >= 0) threads.kill(alarmThreadID);
  }
}

void drawBar(int x, int y, char *title, int vMin, int vMax, int vYellow, int vRed, float value) {
  float graphHeight = 240.0f;
  float graphWidth = 65.0f;
  float deltaMinMax = abs(vMax - vMin);
  float stepWidth = (graphHeight - 26.0f) / deltaMinMax;
  float divisors = 6.0f;

  tft.drawLine(x + 23, y + 23, x + 23, y + graphHeight - 5, foreColor); //Y-Left-Line
  tft.drawLine(x + 23, y + graphHeight - 5, x + graphWidth, y + graphHeight - 5, foreColor); //X-Bottom-Line

  //Title
  tft.setTextColor(foreColor);
  tft.setFont(Arial_12_Bold);
  int tXsize = tft.strPixelLen(title);
  tft.setCursor(x + graphWidth / 2 - tXsize / 2, y);
  tft.print(title);

  //Scale values and divisors
  tft.setFont(Arial_10);
  for (int i = 0; i < divisors + 1; i++) {
    int scaleValue = (deltaMinMax / divisors * i) + vMin;
    tXsize = tft.strPixelLen(intToChar(scaleValue));
    tft.setCursor(x + 17 - tXsize, y + graphHeight - 10 - (i * (graphHeight - 28) / divisors));
    tft.print(scaleValue);

    tft.drawLine(x + 19, y + graphHeight - 5 - (i * (graphHeight - 28) / divisors), x + 22, y + graphHeight - 5 - (i * (graphHeight - 28) / divisors), foreColor); //Y-Divisor
  }

  //Bar
  tft.fillRect(x + 25, y + (graphHeight - 5) + -1 * ((value - vMin) * stepWidth), graphWidth - 25, (value - vMin) * stepWidth, 0x07E0); //green bar

  if (value >= vYellow) {
    tft.fillRect(x + 25, y + (graphHeight - 5) + -1 * ((value - vMin) * stepWidth), graphWidth - 25, ((value - vMin) * stepWidth) - ((vYellow - vMin) * stepWidth), 0xFFE0); //yellow bar
  }
  if (value >= vRed) {
    tft.fillRect(x + 25, y + (graphHeight - 5) + -1 * ((value - vMin) * stepWidth), graphWidth - 25, ((value - vMin) * stepWidth) - ((vRed - vMin) * stepWidth), 0xF800); //red bar
  }
}

//-------------------------------------------------------------//
//---------------------------Helpers---------------------------//
//-------------------------------------------------------------//
char* intToChar(int value) {
  itoa(value, bufInt, 10);
  return bufInt;
}

char* floatToChar(float value, int decis) {
  //if (decis < 1) decis = 1;
  byte len = 3 + decis - 1;
  if (value >= 10) len = 4 + decis - 1;
  if (value >= 100) len = 5 + decis - 1;
  if (value >= 1000) len = 6 + decis - 1;
  if (value >= 10000) len = 7 + decis - 1;
  if (value >= 100000) len = 8 + decis - 1;
  if (value < 0) len = 4 + decis - 1;
  if (value <= -10) len = 5 + decis - 1;
  if (value <= -100) len = 6 + decis - 1;
  if (value <= -1000) len = 7 + decis - 1;

  dtostrf(value, len, decis, bufFloat);
  return bufFloat;
}

void addValue(int *arr, int value) {
  for (int i = 39 - 1; i > 0; i--) {
    arr[i] = arr[i - 1];
  }
  arr[0] = value;
}

void bootAnimation() {
  tft.fillScreen(backColor);
  tft.setCursor(0, 0);
  tft.setFont(Arial_12);
  tft.setTextColor(foreColor);
  tft.println("Author: S. Balzer");
  tft.print("Version: ");
  tft.println(bcVersion);

  for (int i = 0; i < 255; i += 15) {
    tft.drawPicBrightness(480 / 2 - 263 / 2, 320 / 2 - 133 / 2, 263, 133, (uint16_t*)opc, i);
    tft.updateScreen();
  }
  delay(2000);
  //tone(pin, frequency, duration)
  tone(buzzer, 1650, 200);
  for (int i = 255; i > 0; i -= 30) {
    tft.drawPicBrightness(480 / 2 - 263 / 2, 320 / 2 - 133 / 2, 263, 133, (uint16_t*)opc, i);
    tft.updateScreen();
  }
}

void playAlarm() {
  while(1) {
    if(playAlarms){
      tone(buzzer, 2000, 150);
      threads.delay(500);
    }
  }
}

//-------------------------------------------------------------//
//------------------------OBD-Functions------------------------//
//-------------------------------------------------------------//
void tech2() {
  CAN_message_t can_MsgRx;

  while (1) { //Thread-Schleife
    if (!ecuConnected) { //wenn nicht verbunden, sende "7E0 01 20 0 0 0 0 0 0" bis "7E8 01 60 0 0 0 0 0 0" geantwortet wird
      //Serial.println("Nicht verbunden.");
      while (ecuConnected == false) {//Solange nicht verbunden-Schleife
        //Serial.println("Sende Verbindungsanfrage...");

        //        info thisActualInfo((char*)"Sende Verbindungsanfrage...", 0x0000, 3, 500);
        //        actualInfo = thisActualInfo;

        sendEcuData(0x01, 0x20, 0, 0, 0, 0, 0, 0);
        threads.delay(500);

        if (ecuConnected) {
          //Serial.println("Verbunden mit ECU.");

          //info thisActualInfo("Verbunden mit ECU.", foreColor, 1, 500);
          //actualInfo = thisActualInfo;

          threads.delay(15); //Warte 15ms, danach wird die Liste konfiguriert
          //sendingTime = 0;
        }
      }
    } else { //wenn verbunden
      if (listSet == false) { //wenn Datenliste noch nicht konfiguriert
        sendEcuData(0x10, 0x0C, 0xAA, 0x04, 0x10, 0x11, 0x12, 0x13); //Pakete 10, 11, 12, 13
        threads.delay(15); //Warte 15ms
        sendEcuData(0x21, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x00); //Pakete 14, 15, 16, 17, 18, 19
        listSet = true;

        //Starte Herzschlag / Verbindungsüberwachung
        heartBeatThreadID = threads.addThread(heartBeat);
      }
      else {
        while (ecuConnected) {
          threads.delay(500);
        }
      }
    }
  }
  threads.yield();
}

void handleData500Msg(const CAN_message_t &can_MsgRx) {
  if (can_MsgRx.id == CANID_DATAREPLY) { //Motordaten empfangen von 0x5E8
    for (int i = 0; i < 8; i++) {
      //Serial.write((uint8_t)can_MsgRx.buf[i]);
      Serial.print(can_MsgRx.buf[i]);
      if (i < 7)Serial.print(",");
    }
    Serial.println();

    switch (can_MsgRx.buf[0]) { //Datenpaketnummer
      case 0x03:
        isMilOn = can_MsgRx.buf[7];
        break;

      case 0x10:
        vBatt = can_MsgRx.buf[1] / 10.0f;
        rpm = ((can_MsgRx.buf[2] * 256.0f) + can_MsgRx.buf[3]) / 4.0f;
        speed = can_MsgRx.buf[7];
        speedHandle();

        if (rpm > rMAXrpm)rMAXrpm = rpm;
        if (rpm > 0 && EngineStartTime == 0) {
          EngineStartTime = millis();

          byte bytes[2];
          bytes[0] = (engineStarts + 1) & 0xFF;
          bytes[1] = ((engineStarts + 1) >> 8) & 0xFF;

          EEPROM.write(4, bytes[0]);
          EEPROM.write(3, bytes[1]);
        }
        if (speed > rMAXspeed)rMAXspeed = speed;

        break;

      case 0x11:
        iat = can_MsgRx.buf[4] - 40;
        ect = can_MsgRx.buf[2] - 40;
        ambientTemp = can_MsgRx.buf[5] - 40;

        if (iat > rMAXiat)rMAXiat = iat;
        if (ect > rMAXect)rMAXect = ect;
        break;

      case 0x12:
        maf = ((can_MsgRx.buf[2] * 256.0f) + can_MsgRx.buf[3]) / 100.0f * 3.6f;
        mafVolt = can_MsgRx.buf[1] / 51.0f;
        if (firstBoostValRec) firstBoostVal = can_MsgRx.buf[7], firstBoostValRec = false;
        boost = can_MsgRx.buf[7] - firstBoostVal;
        power = maf * 0.383;

        addValue(mafArray, maf);
        addValue(boostArray, boost);
        if (power > rMAXpower)rMAXpower = power;
        if (maf > rMAXmaf)rMAXmaf = maf;
        if (rpm <= 5)boost = 0;
        if (boost > rMAXboost)rMAXboost = boost;
        break;

      case 0x13:
        //Pedal Position
        break;

      case 0x14:
        //Throttle Position

        inj = can_MsgRx.buf[7] / 10.0f; //Injektor Pulsweite

        if (rpm <= 5)inj = 0;
        injAuslast = inj * rpm / 1200.0f; //Duty Cycle
        if (injAuslast > rMAXinjAuslast)rMAXinjAuslast = injAuslast;
        if (inj > rMAXinj)rMAXinj = inj;
        getConsum();
        break;

      case 0x15:
        tankInhalt = (can_MsgRx.buf[7] / 2.55f - can_MsgRx.buf[6] / 256.0f);
        if (tankInhalt <= 5.0f) {
          actualInfo = info((char*)"Kraftstoffstand niedrig!", ILI9486_RED, 3, 5000);
        }
        break;

      case 0x16:
        ign = (can_MsgRx.buf[2] - 36.0f) / 10.0f;

        if (ign > rMAXign)rMAXign = ign;
        break;

      case 0x17:
        //Klopfsesor-Spannung = (A*5 + B/51) / 10  [V]
        knockVal = (can_MsgRx.buf[1] * 5.0f + can_MsgRx.buf[2] / 51.0f) / 10.0f;
        break;

      case 0x18:
        //Klopfregelung
        knockRet1 = can_MsgRx.buf[1];
        knockRet2 = can_MsgRx.buf[2];
        knockRet3 = can_MsgRx.buf[3];
        knockRet4 = can_MsgRx.buf[4];
        break;

      case 0x19:
        //Lambdasonde
        sft = (can_MsgRx.buf[1] - 128.0f) / 1.28;
        lambdaVal = can_MsgRx.buf[3] / 51.0f;
        break;

      case 0x81:
        answered = true;
        //Fehlercodes
        if (can_MsgRx.buf[4] == 0xFF) {
          //Ende der Übertragung
        } else {
          fcs++; //Fehleranzahl zählen
          decodeFCs(can_MsgRx.buf, 0);
        }
        break;

      case 0xA9: //TODO test 0xA9
        answered = true;
        //Fehlercodes
        if (can_MsgRx.buf[4] == 0xFF) {
          //ende der Übertragung
        } else {
          fcs++; //Fehleranzahl zählen
          decodeFCs(can_MsgRx.buf, 1);
        }
        break;
    }
  } else if (can_MsgRx.id == CANID_REPLY) {
    if (can_MsgRx.buf[0] == 0x01) {
      ecuConnected = true;
    }
    if (can_MsgRx.buf[1] == 0x7E) {
      response = true;
    }
  }
}

void handleData33Msg(const CAN_message_t &can_MsgRx) {
  //Serial.println(can_MsgRx.id, HEX);
  //Serial.print(", ");
  //Serial.print(can_MsgRx.buf[0]);
  //Serial.print(can_MsgRx.buf[1]);
  //Serial.print(can_MsgRx.buf[2]);
  //Serial.print(can_MsgRx.buf[3]);
  //Serial.print(can_MsgRx.buf[4]);
  //Serial.print(can_MsgRx.buf[5]);
  //Serial.print(can_MsgRx.buf[6]);
  //Serial.println(can_MsgRx.buf[7]);
  if (can_MsgRx.id == 0x175) { //Fernbedienung
    if (can_MsgRx.buf[5] == 0x10 && can_MsgRx.buf[6] == 0x1F) { //UP
      //up pressed
      switch (actualView) {
        case HOME_VIEW:
          switchView(GRAPH_VIEW);
          break;
        case GRAPH_VIEW:
          switchView(MAX_VIEW);
          break;
        case MAX_VIEW:
          switchView(HOME_VIEW);
          break;
      }
    } else if (can_MsgRx.buf[5] == 0x20 && can_MsgRx.buf[6] == 0x01) { //DOWN
      //down pressed
      switch (actualView) {
        case HOME_VIEW:
          switchView(MAX_VIEW);
          break;
        case GRAPH_VIEW:
          switchView(HOME_VIEW);
          break;
        case MAX_VIEW:
          switchView(GRAPH_VIEW);
          break;
      }
    } else if (can_MsgRx.buf[5] == 0x30 && can_MsgRx.buf[6] == 0x00) { //ENTER
      //enter pressed
      if (actualView == HOME_VIEW) {
        if (waschwasserLeer) {
          waschwasserLeerOk = true;
        }
      } else if (actualView == MAX_VIEW) { //reset max vals
        rMAXect = 0;
        rMAXboost = 0;
        rMAXiat = 0;
        rMAXmaf = 0;
        rMAXign = 0;
        rMAXrpm = 0;
        rMAXinj = 0.0f;
        rMAXinjAuslast = 0;
        rMAXpower = 0;
        rMAXmoment = 0;

        drawMaxView();
      } else if (actualView == GRAPH_VIEW) { //reset max vals
        graphHold = true;
      }
    }
  } else if (can_MsgRx.id == 0x445) { //Außentemp
    aat = can_MsgRx.buf[1] / 2.0f - 40.0f;
  } else if (can_MsgRx.id == 0x430) { //Info-Signale
    // 430 62  FF  FF  0 0 0 0 0 33 R-Gang drin, alles frei (Byte02 = Nähe)
    if (can_MsgRx.buf[0] == 0x62) { //R-Gang drin
      //Radio leiser machen
      //175  0 0 0 0 0 2 0 1F  33 - Befehl "Radio leiser (Fernbedienung)"
      if (!rGang) {
        sendBus33Data(0x175, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x1F);
        threads.delay(100);
        sendBus33Data(0x175, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x1F);
        threads.delay(100);
        sendBus33Data(0x175, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x1F);
        threads.delay(100);
        sendBus33Data(0x175, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x1F);
        threads.delay(100);
        sendBus33Data(0x175, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x1F);
        threads.delay(100);
        sendBus33Data(0x175, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x1F);
        threads.delay(100);
        sendBus33Data(0x175, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x1F);
        threads.delay(100);
        sendBus33Data(0x175, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x1F);
        threads.delay(100);
        sendBus33Data(0x175, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x1F);
        threads.delay(100);
        sendBus33Data(0x175, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x1F);
      }
      distanceBehind = can_MsgRx.buf[2]; //255 = max distance, 0 = min distance
      lastRGang = millis();
      rGang = true;
    } else if (can_MsgRx.buf[0] == 0x22) {
      //R-Gang raus
      rOut = true;
    }
  }
}

//95Msg
//  if (can_MsgRx.id == 0x206) {      //Fernbedienung
//    if(can_MsgRx.buf[0] == 0x08 && can_MsgRx.buf[1] == 0x83){
//      //up or down?
//      if(can_MsgRx.buf[2] == 0xFF){
//        //up pressed
//        switch (actualView){
//          case HOME_VIEW:
//            actualView = GRAPH_VIEW;
//            switchView();
//            break;
//          case GRAPH_VIEW:
//            actualView = MAX_VIEW;
//            switchView();
//            break;
//          case MAX_VIEW:
//            actualView = HOME_VIEW;
//            switchView();
//            break;
//        }
//      }else if(can_MsgRx.buf[2] == 0x01){
//        //down pressed
//        switch (actualView){
//          case HOME_VIEW:
//            actualView = MAX_VIEW;
//            switchView();
//            break;
//          case GRAPH_VIEW:
//            actualView = HOME_VIEW;
//            switchView();
//            break;
//          case MAX_VIEW:
//            actualView = GRAPH_VIEW;
//            switchView();
//            break;
//        }
//      }
//    }else if(can_MsgRx.buf[0] == 0x01 && can_MsgRx.buf[1] == 0x84){
//      //enter pressed
//      if(actualView == HOME_VIEW){
//        if(waschwasserLeer){
//          waschwasserLeerOk = true;
//        }
//      }else if (actualView == MAX_VIEW){ //reset max vals
//        rMAXect = 0;
//        rMAXboost = 0;
//        rMAXiat = 0;
//        rMAXmaf = 0;
//        rMAXign = 0;
//        rMAXrpm = 0;
//        rMAXinj = 0.0f;
//        rMAXinjAuslast = 0;
//        rMAXpower = 0;
//        rMAXmoment = 0;
//
//        drawMaxView();
//      }
//    }
//  }else if(can_MsgRx.id == 0x2B0){  //check control message
//    if(can_MsgRx.buf[0] == 0x46 && can_MsgRx.buf[1] == 0x0B){
//      //Waschwasser leer
//      waschwasserLeer = true;
//    }else if(can_MsgRx.buf[0] == 0x46 && can_MsgRx.buf[0] == 0x04){
//      //Waschwasser voll
//      waschwasserLeer = false;
//    }
//  }else if(can_MsgRx.id == 0x683){  //Außentemp
//    aat = can_MsgRx.buf[2] / 2.0f - 40.0f;
//  }else if(can_MsgRx.id == 0x4E8){  //Info-Signale
//    // 4E8 46  F 0 0 0 0 4   95 - Signal "R-Gang drin"
//    if(can_MsgRx.buf[0] == 0x46 && can_MsgRx.buf[1] == 0x0F && can_MsgRx.buf[6] == 0x05){
//      //R-Gang drin
//      //Radio leiser machen
//      // 206 8 93  FF          95 - Befehl "Radio leiser"
//      // 206 8 93  1           95 - Befehl "Radio lauter"
//      sendBus33Data(0x206, 0x08, 0x93, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00);
//      lastRGang = millis();
//      rGang = true;
//    }
//  }

void heartBeat() {
  int heartBeatTimeOut = 419; //alle 419ms Herzschlag senden
  threads.delay(heartBeatTimeOut);
  while (ecuConnected) {
    sendEcuData(0x01, 0x3E, 0, 0, 0, 0, 0, 0);
    response = false;
    threads.delay(heartBeatTimeOut);

    if (response == false) { //alles zurücksetzen
      noResponseCount++;
      //Serial.println("Keine Antwort von ECU zum " + (String)noResponseCount + ". mal!");
      if (noResponseCount >= 6) {
        //Serial.println("Verbindung getrennt!");

        ect = 0;
        boost = 0.0;
        iat = 0;
        maf = 0;
        ign = 0;
        sft = 0;
        fcs = 0;
        rpm = 0;
        inj = 0;
        vBatt = 0;
        injAuslast = 0;
        power = 0;
        listSet = false;
        ecuConnected = false;
        threads.kill(heartBeatThreadID);
        break;
      }
    } else { //prog laufen lassen
      noResponseCount = 0;
    }
    threads.yield();
  }
}

void sendEcuData(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4, uint8_t byte5, uint8_t byte6, uint8_t byte7) {
  CAN_message_t TxMsg;
  TxMsg.buf[0] = byte0;
  TxMsg.buf[1] = byte1;
  TxMsg.buf[2] = byte2;
  TxMsg.buf[3] = byte3;
  TxMsg.buf[4] = byte4;
  TxMsg.buf[5] = byte5;
  TxMsg.buf[6] = byte6;
  TxMsg.buf[7] = byte7;
  TxMsg.flags.extended = 0;
  TxMsg.flags.remote = 0;
  TxMsg.flags.overrun  = 0;
  TxMsg.flags.reserved = 0;
  TxMsg.len = 8;
  TxMsg.id = CANID_REQUEST;

  //send request
  if (!can500.write(TxMsg)) {
    //Serial.println("Senden fehlgeschlagen");
    actualInfo = info((char*)"ECU nicht verbunden !", ILI9486_RED, 3, 5000);
  }
}

void sendBus33Data(uint16_t id, uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4, uint8_t byte5, uint8_t byte6, uint8_t byte7) {
  CAN_message_t TxMsg;
  TxMsg.buf[0] = byte0;
  TxMsg.buf[1] = byte1;
  TxMsg.buf[2] = byte2;
  TxMsg.buf[3] = byte3;
  TxMsg.buf[4] = byte4;
  TxMsg.buf[5] = byte5;
  TxMsg.buf[6] = byte6;
  TxMsg.buf[7] = byte7;
  TxMsg.flags.extended = 0;
  TxMsg.flags.remote = 0;
  TxMsg.flags.overrun  = 0;
  TxMsg.flags.reserved = 0;
  TxMsg.len = 8;
  TxMsg.id = id;

  //send request
  if (!can33.write(TxMsg)) {
    //Serial.println("Senden fehlgeschlagen");
  }
}

void clearFCs() {
  fcs = 0;
  sendEcuData(0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
  if (actualView == FC_VIEW) {
    //clear screen
    tft.fillRect(0, 35, 320, 240, backColor);
    tft.setCursor(0, 38);
    tft.setFont(Arial_12);
    tft.setTextColor(foreColor);
    tft.println("Loesche Fehlercodes...");

    threads.delay(2000); //Warte 2sec

    tft.fillRect(0, 35, 320, 240, backColor);
    tft.setCursor(0, 38);
    tft.setFont(Arial_12);
    tft.setTextColor(foreColor);

    //request DTCs
    sendEcuData(0x03, 0xA9, 0x81, 0x12, 0x00, 0x00, 0x00, 0x00);
    int requestTime = millis();
    answered = false;

    //recive with timeout
    CAN_message_t can_MsgRx;
    while ((int)millis() < requestTime + 1000);
    //Serial.println("answered = " + (String)answered);
    if (fcs <= 0) { //wenn keine Fehlercodes, Info anzeigen
      if (actualView == FC_VIEW) {
        tft.println("Keine Fehlercodes");
      }
    }
    if (actualView == FC_VIEW && answered == false) {
      tft.println("Keine Rueckmeldung von ECU");
    }
  }
}

void requestFCs() {
  // kill tech2- and heartbeat-thread
  threads.kill(tech2ThreadID);
  threads.kill(heartBeatThreadID);

  threads.delay(25);

  // Disconnect Datamonitor
  sendEcuData(0x02, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
  threads.delay(50);

  //request DTCs
  fcs = 0;
  sendEcuData(0x03, 0xA9, 0x81, 0x12, 0x00, 0x00, 0x00, 0x00);
  answered = false;
  threads.delay(1000);

  if (fcs <= 0) { //wenn keine Fehlercodes, Info anzeigen
    if (actualView == FC_VIEW) {
      tft.println("Keine Fehlercodes");
    }
  }
  if (actualView == FC_VIEW && answered == false) {
    tft.println("Keine Rueckmeldung von ECU");
  }
}

void decodeFCs(const byte buff[], const int stat) {
  if (actualView == FC_VIEW) {
    tft.print(intToChar(fcs)); //Fehlernummer (1. , 2. , 3.)
    tft.print(". ");           //Fehlernummer (1. , 2. , 3.)

    byte firstByte = (byte)buff[1];
    byte secondByte = (byte)buff[2];

    byte fSystem = firstByte >> 6;           //P, C, B, U
    byte specific = (firstByte >> 4) & 0x03; // 0 or 1
    byte subSystem = firstByte & 0x0f;       // 0 to 7
    byte thirdNum = secondByte >> 4;
    byte fourthNum = secondByte & 0x0f;

    char desc[3];
    sprintf(desc, "%02x", (byte)buff[3]); //descriptor

    switch (fSystem) {
      case 0:
        tft.print('P');
        break;
      case 1:
        tft.print('C');
        break;
      case 2:
        tft.print('B');
        break;
      case 3:
        tft.print('U');
        break;
      default:
        tft.print('-');
        break;
    } //P

    tft.print(specific, DEC);  //0
    tft.print(subSystem, HEX); //4
    tft.print(thirdNum, HEX);  //0
    tft.print(fourthNum, HEX); //0
    tft.print("-");            //-
    tft.print(desc);           //5

    if (fSystem == 0) { //Power train
      switch (subSystem) {
        case 0:
          tft.print(" Sensor Umgebungslufttemperatur");
          break;
        case 1: //LMM, IAT, ECT, TPS, O2S
          switch (thirdNum) {
            case 0:
              tft.print(" Luftmassenmesser");
              break;
            case 1:
              if (fourthNum == 0) {
                tft.print(" Sensor Ansauglufttemperatur");
              }
              else if (fourthNum == 5) {
                tft.print(" Sensor Kuehlmitteltemperatur");
              }
              break;
            case 2:
              tft.print(" Pedalpositionsgeber A");
              break;
            case 3:
              tft.print(" Lambdasonde");
              break;
            case 4:
              tft.print(" Lambdasonde Heizkreis");
              break;
            case 7:
              tft.print(" Kraftstoffkorrektur");
              break;
          }
          break;

        case 2: //INJ, RPM, APP, FP, BPS, BPV,
          switch (thirdNum) {
            case 0:
              tft.print(" Einspritzventil ");
              tft.print(fourthNum, DEC);
              break;
            case 1:
              tft.print(" Motordrehzahl zu groß");
              break;
            case 2:
              tft.print(" Pedalpositionsgeber B");
              break;
            case 3:
              if (fourthNum == 0) {
                tft.print(" Kraftstoffpumpe Primaerkreis");
              }
              else if (fourthNum == 5) {
                tft.print(" Ladedruckgeber");
              }
              break;
            case 4:
              tft.print(" Ladedruckregelventil");
              break;
          }
        case 3:
          if (thirdNum == 0 && fourthNum == 0) { //P0300
            tft.print(" Fehlzuendung mehrere Zylinder");
          } else {
            switch (thirdNum) {
              case 0:
                tft.print(" Fehlzuendung Zylinder ");
                tft.print(fourthNum, DEC);
                break;
              case 1:
                tft.print(" Fehlzuendung bei leerem Tank");
                break;
              case 2:
                tft.print(" Klopfsensor");
                break;
              case 3:
                tft.print(" Kurbelwellensensor");
                break;
              case 4:
                tft.print(" Nockenwellensensor");
                break;
            }
          }
          break;

        case 4:
          if (thirdNum == 4)
            tft.print(" Tankentlueftungssystem");
          else if (thirdNum == 6)
            tft.print(" Kraftstoffstandsensor");
          break;

        case 5:
          switch (thirdNum) {
            case 0:
              tft.print(" Fahrzeuggeschwindigkeitssignal");
              break;
            case 2:
              tft.print(" Oeldruckschalter");
              break;
            case 3:
              tft.print(" Klimaanlagendruck");
              break;
            case 7:
              tft.print(" Bremslichtschalter");
              break;
          }
          break;

        case 6:
          switch (thirdNum) {
            case 0:
              tft.print(" Klopfsensor Schaltkreis");
              break;
            case 2:
              tft.print(" Generator Regelung");
              break;
            case 5:
              tft.print(" MIL Fehler");
              break;
          }
          break;

        case 7:
          tft.print(" Kupplungsschalter");
          break;

        default:
          tft.print(" UNKNWN");
          break;
      }
    }
    tft.println();
  }
}
