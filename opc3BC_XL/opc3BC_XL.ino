/*
   Autor: Sebastian Balzer
   Version: 2.7 XL + PC-Out
   Date: 29.07.2022

   Hardware: Teensy 4.0, Waveshare CAN Board SN65HVD230, Waveshare 4inch RPi LCD (C) ILI9486 with XPT2046 Touch, LM2596 DC/DC Converter - set 5.0 Volt out
   Car: Opel Astra H opc
   Interface 1: HSCAN 500 kBaud (Pin 6 and 14 on OBD connector)
   Interface 2: MSCAN 33.3 kBaud (Pin 1 and 5(GND) on OBD connector)
*/
const char bcVersion[] = "2.7 XL";

// Language select - only uncomment one
//#define DE //Deutsch
#define EN //English
//#define FR //France
//#define RO //Romanian

// Uncomment if you have a Waveshare 4inch RPi LCD (C) ILI9486 display installed
// Comment out if you have a Waveshare 4inch RPi LCD (A) ILI9486 display installed
#define TYPE_C_DISPLAY

// Sends data over serial to PC
//#define PCOUT

// Take Screenshot to Serial Monitor with command "<sh>"
//#define SHTS

#include "SPI.h"
#include <XPT2046_Touchscreen.h>
#include <FlexCAN_T4.h>
#include "TeensyThreads.h"
#include <EEPROM.h>
#include "ILI9486_t3n.h"
#include "ili9486_t3n_font_Arial.h"
#include "ili9486_t3n_font_ArialBold.h"
#include "graphics.c"
#include "bcButton.h"
#include "info.h"
#include "strings.h"

// Threads
uint8_t tech2ThreadID;
uint8_t displayDataThreadID;
uint8_t testerPresentThreadID;
uint8_t spiThreadID;
uint8_t alarmThreadID;

// Touchscreen config
#define CS_PIN 7
XPT2046_Touchscreen ts(CS_PIN);
#define TIRQ_PIN 2
// This is calibration data for the raw touch data to the screen coordinates
#ifdef TYPE_C_DISPLAY
#define TS_MINX 3800
#define TS_MINY 190
#define TS_MAXX 120
#define TS_MAXY 3800
#else
#define TS_MINX 3500
#define TS_MINY 3800
#define TS_MAXX 190
#define TS_MAXY 200
#endif

// TFT config
#define TFT_DC 9
#define TFT_CS 10
#define TFT_RST 8
#define TFT_MOSI 11
#define TFT_SCLK 13
#define TFT_MISO 12
ILI9486_t3n tft = ILI9486_t3n(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);

// Can 500 config
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> can500;
const uint32_t canBaud500 = (uint32_t)500000;

// Can 33.3 config
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can33;
const uint32_t canBaud33 = (uint32_t)33333;

// Buzzer pin
const uint8_t buzzer = 4;

// Definition of CAN-IDs
#define CANID_REPLY 0x7E8
#define CANID_REQUEST 0x7E0
#define CANID_DATAREPLY 0x5E8

// Display Views
#define HOME_VIEW 0
#define FC_VIEW 1
#define MAX_VIEW 2
#define ACC_VIEW 3
#define GRAPH_VIEW 4
#define IGNITION_VIEW 5
#define FUEL_VIEW 6
#define SETT_VIEW 7
uint8_t actualView;

// Styles
const uint16_t backColor = ILI9486_BLACK;
const uint16_t foreColor = ILI9486_WHITE;

// Motordaten-Variablen
int ect = 0;
float vBatt = 0.0f;
int boost = 0;
int iat = 0;
int ambientTemp = 0;
float maf = 0.0f;
float mafVolt = 0.0f;
float ign = 0;
int sft = 0;
float lambdaVolt = 0.0f;
int fcs = 0;
int rpm = 0;
int speed = 0;
float inj = 0.0f;
bool isMilOn = false;
float aat = 0.0f; // Außentemp. (CAN 33,3kbps)
int distanceBehind = 0;
float knockVal = 0.0f;
byte knockRet1;
byte knockRet2;
byte knockRet3;
byte knockRet4;

// berechnete Daten
float absolutConsume;
int injUt = 0;
int power = 0;
float moment = 0;
float consume = 0.0f;
//float afr = 0.0f;
bool waschwasserLeer = false;
bool rGang = false;
bool rOut = false;
long lastRGang = 0;
uint64_t EngineStartTime = 0;
uint64_t lastSpeedTime = 0;
uint64_t lastConsumeTime = 0;
float strecke = 0;
float odoMeter = 0;
float tankInhalt = 0;

// Alte Motordaten-Variablen
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
int oldinjUt = 0;
bool oldisMilOn = true;
float oldconsume = 0.0f;
//float oldafr = 0.0f;
float oldaat;

// max-reached-vals
int rMAXect = 0;
int rMAXboost = 0;
int rMAXiat = 0;
int rMAXmaf = 0;
float rMAXign = 0.0f;
int rMAXrpm = 0;
int rMAXspeed = 0;
float rMAXinj = 0.0f;
int rMAXinjUt = 0;
int rMAXpower = 0;
int rMAXmoment = 0;
int rMAXconsume = 0;
//float rMAXafr = 0.0f;

// Beschleunigungszeiten Variablen
uint64_t t100 = 0;
uint64_t t200 = 0;
uint64_t t100Start = 0;
uint64_t t200Start = 0;
uint64_t t100Stop = 0;
uint64_t t200Stop = 0;
bool t100Started = false;
bool t200Started = false;
bool gotT100 = false;
bool gotT200 = false;
int16_t tachoabw = 0;
uint16_t oldactSpeed;
uint16_t actSpeed;

// Programm-Variablen
const int arrayLength = 39;
int boostArray[arrayLength];
int mafArray[arrayLength];
int consumeArray[arrayLength];
bool ecuConnected = false;
bool listSet = false;
bool response = false;
uint16_t noResponseCount = 0;
info actualInfo;
info oldInfo;
uint64_t infoWritten;
bool answered = false;
char bufInt[12];   // global variable created because malloc cannot be used here
char bufFloat[12]; // global variable created because malloc cannot be used here
bool dispBufReady = false;
bool graphHold = false;
bool firstBoostValRec = true;
int firstBoostVal = 100; // atmosperic pressure, will be corrected by first value received
uint32_t engineStarts = 0;
char disabledInfos[10][35];

// Settings
uint16_t lcdBright = 255;
uint16_t maxLcdBright = 255;
uint16_t minLcdBright = 50;
bool skipBoot = false;
bool playAlarms = true;
bool handleRVolume = true;

// Buttons
const uint8_t maxVisibleBtns = 10;
Button *visibleBtns[maxVisibleBtns];
Button btnDTCs = Button(0, 285, 84, 34, (char *)"DTCs", backColor, styleBtn);
Button btnGraph = Button(90, 285, 84, 34, (char *)"Graph", backColor, styleBtn);
Button btnMax = Button(180, 285, 84, 34, (char *)"MaxV", backColor, styleBtn);
Button btnAcc = Button(270, 285, 84, 34, (char *)"AccM", backColor, styleBtn);
Button btnFuel = Button(360, 285, 84, 34, (char *)"Fuel", backColor, styleBtn);
Button btnResetAcc = Button(90, 285, 84, 34, (char *)"Reset", backColor, styleBtn);
Button btnBack = Button(0, 0, 84, 34, (char *)"Back", backColor, styleBtn);
Button btnHome = Button(0, 285, 84, 34, (char *)"Home", backColor, styleBtn);
Button btnClrFCs = Button(90, 0, 84, 34, (char *)"Clear", backColor, styleBtn);
Button btnResetMax = Button(90, 285, 84, 34, (char *)"Reset", backColor, styleBtn);
Button btnPlus = Button(345, 213, 30, 30, (char *)"+", backColor, styleBtnSmall);
Button btnMinus = Button(390, 213, 30, 30, (char *)"-", backColor, styleBtnSmall);
Button btnHold = Button(90, 285, 84, 34, (char *)"Hold", backColor, styleBtn);
Button btnIgnition = Button(180, 285, 84, 34, (char *)"Ign", backColor, styleBtn);
Button btnSave = Button(90, 285, 84, 34, (char *)"Save", backColor, styleBtn);
//--- Header buttons (x-area is from 130 to 350 px)
Button btnSettings = Button(290, 0, 30, 30, (char *)"", backColor, settingsBtn);
//--- Settings Buttons
//- skip boot logo
Button setBtnSwitch1 = Button(395, 55, 30, 30, (char *)"S", backColor, styleBtnSmall);
//- max LCD brigthness (default = 255 = 100%)
Button setBtnPlus1 = Button(395, 100, 30, 30, (char *)"+", backColor, styleBtnSmall);
Button setBtnMinus1 = Button(440, 100, 30, 30, (char *)"-", backColor, styleBtnSmall);
//- min LCD brigthness (default = 100 = 39%)
Button setBtnPlus2 = Button(395, 145, 30, 30, (char *)"+", backColor, styleBtnSmall);
Button setBtnMinus2 = Button(440, 145, 30, 30, (char *)"-", backColor, styleBtnSmall);
//- play alarms
Button setBtnSwitch2 = Button(395, 235, 30, 30, (char *)"S", backColor, styleBtnSmall);
//- vol adj in R
Button setBtnSwitch3 = Button(395, 190, 30, 30, (char *)"S", backColor, styleBtnSmall);

void setup()
{
  analogWrite(3, 0); // Disp. LED off

  Serial.begin(115200);

  // init TFT
  tft.begin();
  tft.fillScreen(backColor);
  tft.useFrameBuffer(1);
  tft.setRotation(3);

  // init CAN500
  can500.begin();
  can500.setBaudRate(canBaud500);
  for (int i = 0; i < 10; i++)
  {
    can500.setMB(FLEXCAN_MAILBOX(i), RX, STD);
  }
  can500.enableFIFO();
  can500.enableFIFOInterrupt();
  can500.setFIFOFilter(REJECT_ALL);
  can500.setFIFOFilter(0, CANID_DATAREPLY, STD);
  can500.setFIFOFilter(1, CANID_REPLY, STD);
  can500.onReceive(handleData500Msg);

  // init CAN33
  can33.begin();
  can33.setBaudRate(canBaud33);
  for (int i = 0; i < 10; i++)
  {
    can33.setMB(FLEXCAN_MAILBOX(i), RX, STD);
  }
  can33.enableFIFO();
  can33.enableFIFOInterrupt();
  can33.setFIFOFilter(REJECT_ALL);
  can33.setFIFOFilter(0, 0x175, STD);
  can33.setFIFOFilter(1, 0x430, STD);
  can33.setFIFOFilter(2, 0x445, STD);
  can33.setFIFOFilter(3, 0x626, STD);
  can33.setFIFOFilter(4, 0x235, STD);
  can33.setFIFOFilter(5, 0x305, STD);
  // CAN-IDs for check-control
  can33.setFIFOFilter(6, 0x340, STD);
  can33.setFIFOFilter(7, 0x350, STD);
  can33.setFIFOFilter(8, 0x360, STD);
  can33.onReceive(handleData33Msg);

  // init touch
  ts.setRotation(3);
  ts.begin();

  if (EEPROM.read(5) < 2)
    skipBoot = EEPROM.read(5);
  playAlarms = EEPROM.read(6);
  maxLcdBright = EEPROM.read(7);
  minLcdBright = EEPROM.read(8);
  handleRVolume = EEPROM.read(9);

  if (maxLcdBright < 1)
    maxLcdBright = 255;

  analogWrite(3, maxLcdBright); // Disp. LED on
  if (!skipBoot)
    bootAnimation(); // show bootlogo

  // initialize EEPROM to 1st start - for a reminder to check fluid levels every 100 engine starts
  //   byte bytes[2];
  //   bytes[0] = (engineStarts + 1) & 0xFF;
  //   bytes[1] = ((engineStarts + 1) >> 8) & 0xFF;
  //   EEPROM.write(4, bytes[0]);
  //   EEPROM.write(3, bytes[1]);

  // init Homeview
  tft.fillScreen(backColor);
  switchView(HOME_VIEW);

  // start threads
  spiThreadID = threads.addThread(spiThread);
  displayDataThreadID = threads.addThread(displayData);

  // start communication with ECU
  tech2ThreadID = threads.addThread(tech2);

  engineStarts = EEPROM.read(3) * 256 + EEPROM.read(4);

  if (engineStarts % 100 == 0)
  { // check fluid levels every 100 engine starts
    actualInfo = info(strCheckFl, ILI9486_RED, 5, 10000, fluids);
  }
}

void loop(void)
{
  can500.events();
  can33.events();

#ifdef SHTS // Screenshot to Serial Monitor
  receiveFromSerial();
#endif
}

void spiThread()
{
  while (1)
  {
    if (dispBufReady)
    {
      tft.updateScreen();
    }

    checkTouch();

    threads.yield();
  }
}

void checkTouch()
{
  if (ts.touched())
  {
    // Serial.println("Touched");
    TS_Point p = ts.getPoint();
    // Serial.print((String)p.x + ", " + (String)p.y); //for Calibration
    p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
    p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
    // Serial.println("XYZ = " + (String)p.x + ", " + (String)p.y + ", " + (String)p.z); //for Calibration

    tft.setFont(Arial_20);
    int pxLen = tft.strPixelLen("Saved!");

    if (actualInfo.isTouched(p.x, p.y))
      addDisInfo(actualInfo.Text);

    if (btnDTCs.isTouched(p.x, p.y))
    {
      switchView(FC_VIEW);
    }
    else if (btnGraph.isTouched(p.x, p.y))
    {
      switchView(GRAPH_VIEW);
    }
    else if (btnMax.isTouched(p.x, p.y))
    {
      switchView(MAX_VIEW);
    }
    else if (btnAcc.isTouched(p.x, p.y))
    {
      switchView(ACC_VIEW);
    }
    else if (btnFuel.isTouched(p.x, p.y))
    {
      switchView(FUEL_VIEW);
    }
    else if (btnSettings.isTouched(p.x, p.y))
    {
      switchView(SETT_VIEW);
    }
    else if (btnClrFCs.isTouched(p.x, p.y))
    {
      clearFCs();
      fcs = 0;
      threads.delay(250); // avoids bouncing
    }
    else if (btnBack.isTouched(p.x, p.y) || btnHome.isTouched(p.x, p.y))
    {
      if (actualView == FC_VIEW)
      {
        ecuConnected = false;
        listSet = false;
        tech2ThreadID = threads.addThread(tech2);
      }
      graphHold = false;
      switchView(HOME_VIEW);
    }
    else if (btnHold.isTouched(p.x, p.y))
    {
      graphHold = !graphHold;
      threads.delay(250); // avoids bouncing
    }
    else if (btnIgnition.isTouched(p.x, p.y))
    {
      graphHold = false;
      switchView(IGNITION_VIEW);
    }
    else if (btnResetMax.isTouched(p.x, p.y))
    {
      rMAXect = 0;
      rMAXboost = 0;
      rMAXiat = 0;
      rMAXmaf = 0;
      rMAXign = 0;
      rMAXrpm = 0;
      rMAXspeed = 0;
      rMAXinj = 0.0f;
      rMAXinjUt = 0;
      rMAXpower = 0;
      rMAXmoment = 0;

      drawMaxView();
      threads.delay(250); // avoids bouncing
    }
    else if (btnResetAcc.isTouched(p.x, p.y))
    {
      // Beschleunigungszeiten zurücksetzen
      t100Started = false;
      gotT100 = false;
      t100 = 0;
      t200Started = false;
      gotT200 = false;
      t200 = 0;
    }
    else if (btnPlus.isTouched(p.x, p.y))
    { // tachoabw
      tachoabw++;
      threads.delay(250); // avoids bouncing
    }
    else if (btnMinus.isTouched(p.x, p.y))
    { // tachoabw
      tachoabw--;
      threads.delay(250); // avoids bouncing
    }
    else if (btnSave.isTouched(p.x, p.y))
    {
      // save Settings
      byte bytes[5];
      bytes[0] = skipBoot;
      bytes[1] = playAlarms;
      bytes[2] = maxLcdBright;
      bytes[3] = minLcdBright;
      bytes[4] = handleRVolume;

      EEPROM.write(5, bytes[0]);
      EEPROM.write(6, bytes[1]);
      EEPROM.write(7, bytes[2]);
      EEPROM.write(8, bytes[3]);
      EEPROM.write(9, bytes[4]); // handleRVolume

      // show info "saved"
      tft.fillRoundRect(230 - (pxLen / 2), 0, pxLen + 20, 40, 5, 0xCFF9);
      tft.setTextColor(backColor);
      tft.setFont(Arial_20);
      tft.setCursor(240 - (pxLen / 2), 10);
      tft.print("Saved!");
    }
    else if (setBtnSwitch1.isTouched(p.x, p.y))
    {
      skipBoot = !skipBoot;
      threads.delay(250);                                                    // avoids bouncing
      tft.fillRoundRect(230 - (pxLen / 2), 0, pxLen + 20, 40, 5, backColor); // discard info "saved"
    }
    else if (setBtnSwitch2.isTouched(p.x, p.y))
    {
      playAlarms = !playAlarms;
      threads.delay(250);                                                    // avoids bouncing
      tft.fillRoundRect(230 - (pxLen / 2), 0, pxLen + 20, 40, 5, backColor); // discard info "saved"
    }
    else if (setBtnSwitch3.isTouched(p.x, p.y))
    {
      handleRVolume = !handleRVolume;
      threads.delay(250);                                                    // avoids bouncing
      tft.fillRoundRect(230 - (pxLen / 2), 0, pxLen + 20, 40, 5, backColor); // discard info "saved"
    }
    else if (setBtnPlus1.isTouched(p.x, p.y))
    {
      if (maxLcdBright < 255)
      {
        if (lcdBright == maxLcdBright)
          lcdBright++;
        maxLcdBright++;
        tft.fillRoundRect(230 - (pxLen / 2), 0, pxLen + 20, 40, 5, backColor); // discard info "saved"
      }
    }
    else if (setBtnMinus1.isTouched(p.x, p.y))
    {
      if (maxLcdBright > 1)
      {
        if (lcdBright == maxLcdBright)
          lcdBright--;
        maxLcdBright--;
        tft.fillRoundRect(230 - (pxLen / 2), 0, pxLen + 20, 40, 5, backColor); // discard info "saved"
      }
    }
    else if (setBtnPlus2.isTouched(p.x, p.y))
    {
      if (minLcdBright < 255)
      {
        if (lcdBright == minLcdBright)
          lcdBright++;
        minLcdBright++;
        tft.fillRoundRect(230 - (pxLen / 2), 0, pxLen + 20, 40, 5, backColor); // discard info "saved"
      }
    }
    else if (setBtnMinus2.isTouched(p.x, p.y))
    {
      if (minLcdBright > 1)
      {
        if (lcdBright == minLcdBright)
          lcdBright--;
        minLcdBright--;
        tft.fillRoundRect(230 - (pxLen / 2), 0, pxLen + 20, 40, 5, backColor); // discard info "saved"
      }
    }

    analogWrite(3, lcdBright); // Disp. LED
  }
}

void displayData()
{
  while (1)
  {
    consume = getConsum();
    if (consume > rMAXconsume)
      rMAXconsume = consume;
    //afr = getAFR();

    // r-gang-abfrage
    if (handleRVolume)
      if ((millis() - lastRGang >= 3000 && rGang) || (rOut && rGang))
      { // R-Gang länger als 3 Sec aus
        // Musik wieder etwas lauter machen
        for (int i = 0; i < 6; i++)
        {
          sendBus33Data(0x175, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01);
          if (i < 5)
            threads.delay(100);
        }
        rGang = false;
        rOut = false;
      }

    dispBufReady = false;
    if (actualView == HOME_VIEW)
    {
      tft.fillRect(0, 140, 480, 40, backColor);
      drawHomeView();
      drawInfo();
    }
    else if (actualView == GRAPH_VIEW)
    {
      drawGraphView();
    }
    else if (actualView == IGNITION_VIEW)
    {
      drawIgnitionView();
    }
    else if (actualView == FC_VIEW)
    {
      // Fehlercodes anzeigen
      threads.delay(500);
    }
    else if (actualView == MAX_VIEW)
    {
      drawMaxView();
      threads.delay(500);
    }
    else if (actualView == ACC_VIEW)
    {
      drawAccView();
      threads.delay(20);
    }
    else if (actualView == FUEL_VIEW)
    {
      drawFuelView();
    }
    else if (actualView == SETT_VIEW)
    {
      drawSettingsView();
    }
    dispBufReady = true;

    threads.yield();
  }
}

void switchView(int view)
{
  actualView = view;

  threads.kill(displayDataThreadID);
  tft.fillScreen(backColor);

  if (actualView == HOME_VIEW)
  {
    clearBtns();

    addButton(&btnDTCs);
    addButton(&btnGraph);
    addButton(&btnMax);
    addButton(&btnAcc);
    addButton(&btnFuel);
    addButton(&btnSettings);

    drawHomeView();
  }
  else if (actualView == GRAPH_VIEW)
  {
    clearBtns();

    addButton(&btnSettings);
    addButton(&btnHome);
    addButton(&btnHold);
    addButton(&btnIgnition);

    drawGraphView();
  }
  else if (actualView == IGNITION_VIEW)
  {
    clearBtns();

    addButton(&btnHome);
    addButton(&btnHold);

    drawGraphView();
  }
  else if (actualView == FC_VIEW)
  {
    tft.writeRect(420, 2, 60, 23, (uint16_t *)opc23px);

    clearBtns();

    addButton(&btnBack);
    addButton(&btnClrFCs);

    tft.setCursor(0, 38);
    tft.setFont(Arial_12);
    tft.setTextColor(foreColor);
    tft.updateScreen();

    requestFCs();
  }
  else if (actualView == MAX_VIEW)
  {
    tft.writeRect(420, 2, 60, 23, (uint16_t *)opc23px);
    tft.setCursor(0, 0);
    tft.setFont(Arial_20);
    tft.setTextColor(foreColor);
    tft.print(F(strMaxVals));

    clearBtns();

    addButton(&btnHome);
    addButton(&btnResetMax);

    drawMaxView();
  }
  else if (actualView == ACC_VIEW)
  {
    tft.writeRect(420, 2, 60, 23, (uint16_t *)opc23px);
    tft.setCursor(0, 0);
    tft.setFont(Arial_20);
    tft.setTextColor(foreColor);
    tft.print(F(strAccMon));

    clearBtns();

    addButton(&btnHome);
    addButton(&btnResetAcc);
    addButton(&btnMinus);
    addButton(&btnPlus);

    drawAccView();
  }
  else if (actualView == FUEL_VIEW)
  {
    tft.writeRect(420, 2, 60, 23, (uint16_t *)opc23px);
    tft.setCursor(0, 0);
    tft.setFont(Arial_20);
    tft.setTextColor(foreColor);
    tft.print(F(strFuelMon));

    clearBtns();

    addButton(&btnHome);

    drawFuelView();
  }
  else if (actualView == SETT_VIEW)
  {
    tft.writeRect(420, 2, 60, 23, (uint16_t *)opc23px);
    tft.setCursor(0, 0);
    tft.setFont(Arial_20);
    tft.setTextColor(foreColor);
    tft.print(F(strSettings));

    clearBtns();

    addButton(&btnHome);
    addButton(&btnSave);
    addButton(&setBtnSwitch1);
    addButton(&setBtnPlus1);
    addButton(&setBtnMinus1);
    addButton(&setBtnPlus2);
    addButton(&setBtnMinus2);
    addButton(&setBtnSwitch2);
    addButton(&setBtnSwitch3);

    drawSettingsView();
  }

  threads.delay(250); // avoids bouncing
  displayDataThreadID = threads.addThread(displayData);
}

//-------------------------------------------------------------//
//----------------------------Views----------------------------//
//-------------------------------------------------------------//
void drawHeadLine()
{
  // clear head
  tft.fillRect(0, 0, 129, 33, backColor);
  tft.fillRect(351, 0, 129, 33, backColor);

  // separator1
  tft.drawLine(128, 0, 128, 30, 0xFF4A);
  tft.drawLine(129, 0, 129, 30, 0xFF4A);
  // separator2
  tft.drawLine(351, 0, 351, 30, 0xFF4A);
  tft.drawLine(352, 0, 352, 30, 0xFF4A);

  // DEBUG OIL-LEVEL VALUE
  //   tft.setTextColor(0x87F0);
  //   tft.setFont(Arial_14);
  //   tft.setCursor(137, 8);
  //   tft.print(engOil);
  // END DEBUG OIL-LEVEL

  tft.setTextColor(foreColor);
  tft.setFont(Arial_12);
  tft.setCursor(3, 9);

  if (!rGang)
  { // Wenn R-Gang nicht drin ist, zeige Batteriespannung
    // battery voltage
    tft.print("BATT ");
    tft.setFont(Arial_14);
    tft.setCursor(3 + 46, 8);
    tft.print(vBatt);
    tft.print(" V");
  }
  else
  { // Wenn R-Gang drin ist, zeige Abstand
    // distance behind
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

  // Außentemperatur
  tft.setFont(Arial_12);
  tft.setCursor(360, 9);
  tft.print("OUT ");
  tft.setFont(Arial_14);
  tft.setCursor(412, 8);
  tft.print(aat, 1);
  tft.print(" *C");
}

void drawMaxView()
{
  tft.fillRect(215, 35, 50, 250, 0x0000); // backColor

  tft.setFont(Arial_12);
  tft.setTextColor(foreColor);

  int y = 36;
  tft.setCursor(2, y);
  tft.print(F(strCoolant));
  tft.setCursor(215, y);
  tft.print(rMAXect);
  tft.setCursor(270, y);
  tft.print("*C");

  y += 21;
  tft.setCursor(2, y);
  tft.print(F(strBoost));
  tft.setCursor(215, y);
  tft.print(rMAXboost);
  tft.setCursor(270, y);
  tft.print("kPa");

  y += 21;
  tft.setCursor(2, y);
  tft.println(strIAT);
  tft.setCursor(215, y);
  tft.println(rMAXiat);
  tft.setCursor(270, y);
  tft.println("*C");

  y += 21;
  tft.setCursor(2, y);
  tft.println(strRPM);
  tft.setCursor(215, y);
  tft.println(rMAXrpm);
  tft.setCursor(270, y);
  tft.println(strRpmUnit);

  y += 21;
  tft.setCursor(2, y);
  tft.println(strIgnTime);
  tft.setCursor(215, y);
  tft.println(rMAXign);
  tft.setCursor(270, y);
  tft.println(strIgnTimeUnit);

  y += 21;
  tft.setCursor(2, y);
  tft.println(strMaf);
  tft.setCursor(215, y);
  tft.println(rMAXmaf);
  tft.setCursor(270, y);
  tft.println("kg / h");

  y += 21;
  tft.setCursor(2, y);
  tft.println(strInjUt);
  tft.setCursor(215, y);
  tft.println(rMAXinjUt);
  tft.setCursor(270, y);
  tft.println("%");

  y += 21;
  tft.setCursor(2, y);
  tft.println(strEngStarts);
  tft.setCursor(215, y);
  tft.println(engineStarts);
  tft.setCursor(270, y);
  tft.println("Starts");

  y += 21;
  tft.setCursor(2, y);
  tft.println(strPower);
  tft.setCursor(215, y);
  tft.println(rMAXpower);
  tft.setCursor(270, y);
  tft.println("PS");

  y += 21;
  tft.setCursor(2, y);
  tft.println(strTorque);
  tft.setCursor(215, y);
  tft.println(rMAXmoment);
  tft.setCursor(270, y);
  tft.println("Nm");

  y += 21;
  tft.setCursor(2, y);
  tft.println(strConsume);
  tft.setCursor(215, y);
  tft.println(rMAXconsume);
  tft.setCursor(270, y);
  tft.println("l / h");

  y += 21;
  tft.setCursor(2, y);
  tft.println(strSpeed);
  tft.setCursor(215, y);
  tft.println(rMAXspeed);
  tft.setCursor(270, y);
  tft.println("km / h");
}

void drawHomeView()
{
  drawHeadLine();

  //--------- First Row Y = 33
  oldect = ect;
  // gtrMeter(val, min, max, x , y , unit, crit, title, maxVal, decis, alarm)
  gtrMeter(oldect, 40, 120, 0, 33, (char *)"*C", 110, (char *)"WATER TEMP", rMAXect, 0, true); // Motortemp

  oldboost = boost;
  gtrMeter(oldboost, 0, 200, 162, 33, (char *)"kPa", 150, (char *)"BOOST", rMAXboost, 0, false); // Ladedruck

  oldmaf = maf;
  gtrMeter(oldmaf, 0, 900, 324, 33, (char *)"kg/h", 900, (char *)"AIR FLOW", rMAXmaf, 0, false); // Luftmasse

  //--------- Second Row Y = 156
  oldiat = iat;
  gtrMeter(oldiat, 0, 80, 0, 156, (char *)"*C", 75, (char *)"BOOST TEMP", rMAXiat, 0, true); // Ladelufttemp

  //  oldrpm = rpm;
  //  gtrMeter(oldrpm, 0, 8000, 162, 156, (char*)"rpm", 6500, (char*)"ENGINE SPEED", rMAXrpm, 0, true);
  float lambda = getLambda(); // afr / 14.7f;
  gtrMeter(lambda, 0.5, 1.5, 162, 156, (char *)"x", 1.2, (char *)"LAMBDA", 3, 2, false);

  oldinjUt = injUt;
  gtrMeter(oldinjUt, 0, 100, 324, 156, (char *)"%", 93, (char *)"DUTY CYCLE", rMAXinjUt, 0, true);
}

void drawGraphView()
{
  drawHeadLine();

  //--------- First Row Y = 33
  oldect = ect;
  gtrMeter(oldect, 40, 120, 0, 33, (char *)"*C", 110, (char *)"WATER TEMP", rMAXect, 0, true); // Motortemp

  if (!graphHold)
  {
    // drawGraph(int x, int y, int w, int h, int vmin, int vmax, char *title,        int *values)
    drawGraph(162, 33, 313, 119, 0, 160, (char *)"BOOST kPa", boostArray); // Ladedruck
  }

  //--------- Second Row Y = 156
  oldiat = iat;
  gtrMeter(oldiat, 0, 80, 0, 156, (char *)"*C", 70, (char *)"BOOST TEMP", rMAXiat, 0, true); // Ladelufttemp

  if (!graphHold)
  {
    drawGraph(162, 156, 313, 119, 0, 900, (char *)"AIR FLOW kg/h", mafArray); // Ladedruck
  }
}

void drawIgnitionView()
{
  float lambda = getLambda(); // afr / 14.7f;

  tft.fillRect(0, 0, 480, 320 - 43, backColor);

  tft.setTextColor(foreColor);
  tft.setFont(Arial_12);
  tft.setCursor(3, 12);
  tft.print("Lambda ");
  tft.setCursor(3 + 93, 10);
  tft.setFont(Arial_14);
  tft.print(lambda, 2);

  tft.setFont(Arial_12);
  tft.setCursor(326, 12);
  tft.print("Noise ");
  tft.setCursor(326 + 86, 10);
  if (knockVal > getEngineNoiseLimit())
    tft.setTextColor(0xF800); // red value
  tft.setFont(Arial_14);
  tft.print(knockVal, 2);
  tft.print(" V");
  tft.setTextColor(foreColor);

  // drawBar(int x, int y, char *title, int vMin, int vMax, int vYellow, int vRed, float value)
  drawBar(10, 35, (char *)"IAA [*CA]", -4, 22, 16, 18, ign); // IAA = Ignition Advance Angle

  drawBar(109, 35, (char *)"KR1 [*CA]", 0, 18, 3, 5, knockRet1); // KRx = Knock Retard Angle Cylinder x
  drawBar(208, 35, (char *)"KR2 [*CA]", 0, 18, 3, 5, knockRet2); // KRx = Knock Retard Angle Cylinder x
  drawBar(307, 35, (char *)"KR3 [*CA]", 0, 18, 3, 5, knockRet3); // KRx = Knock Retard Angle Cylinder x
  drawBar(406, 35, (char *)"KR4 [*CA]", 0, 18, 3, 5, knockRet4); // KRx = Knock Retard Angle Cylinder x
}

void drawAccView()
{
  // Aktuelle Geschwindigkeit
  oldspeed = speed;
  // gtrMeter(val, min, max, x , y , unit, crit, title, maxVal, decis)
  gtrMeter(oldspeed, 0, 260, 0, 33, (char *)"km/h", 255, (char *)"SPEED", rMAXspeed, 0, false);

  // Aktuelle Drehzahl
  oldrpm = rpm;
  gtrMeter(oldrpm, 0, 7000, 0, 156, (char *)"rpm", 6500, (char *)"ENGINE SPEED", rMAXrpm, 0, false);

  tft.fillRect(170, 41, 160, 120, backColor);

  // 0-100 Time, mit Korrektur
  tft.setFont(Arial_14);
  tft.setTextColor(foreColor);
  tft.setCursor(170, 41);
  tft.print("0 - 100 km/h");

  tft.setFont(Arial_20);
  tft.setCursor(170, 62);
  tft.print(t100 / 1000.0f, 2); // t100 = Time in Millis
  tft.print(" sec");

  // 100-200 Time, mit Korrektur
  tft.setFont(Arial_14);
  tft.setTextColor(foreColor);
  tft.setCursor(170, 106);
  tft.print("100 - 200 km/h");

  tft.setFont(Arial_20);
  tft.setCursor(170, 127);
  tft.print(t200 / 1000.0f, 2); // t200 = Time in Millis
  tft.print(" sec");

  // Abweichung
  tft.setFont(Arial_14);
  tft.setTextColor(foreColor);
  tft.setCursor(170, 195);
  tft.print(F(strOdoDev));
  tft.setFont(Arial_20);
  tft.fillRect(170, 216, 80, 40, backColor);
  tft.setCursor(170, 216);
  tft.print(tachoabw);
  tft.print(" %");
}

void drawFuelView()
{
  //--------- First Row Y = 33
  gtrMeter(tankInhalt /*Liter im Tank*/, 0 /*min*/, 52 /*max*/, 0 /*x*/, 33 /*y*/, (char *)"L" /*unit*/, 10 /*criticalV*/, (char *)"FUEL TANK" /*title*/, -1 /*maxVal*/, 1 /*decimalPlaces*/, false);

  if (!graphHold)
  {
    drawGraph(162, 33, 313, 119, 0, 120, (char *)"ACTUAL CONSUME l/h", consumeArray); // Verbrauch
  }
  uint16_t y = 160;
  tft.fillRect(0, y, 480, 120, backColor);

  tft.setFont(Arial_14);
  tft.setTextColor(foreColor);
  tft.setCursor(0, y);
  tft.print(F(strAvCons));
  tft.setCursor(270, y);
  if (absolutConsume > 0 && strecke > 0)
    tft.print(absolutConsume / strecke * 100.0f, 1);
  else
    tft.print("0.0");
  tft.print(" L/100km");

  y += 25;
  tft.setCursor(0, y);
  tft.print(F(strAbsCons));
  tft.setCursor(270, y);
  tft.print(absolutConsume, 1);
  tft.print(" L/Trip");

  float runningTime = getEngineRunningMinutes();

  y += 25;
  tft.setCursor(0, y);
  tft.print(F(strAvSpeed));
  tft.setCursor(270, y);
  if (strecke > 0 && runningTime > 0)
    tft.print((float)strecke / (runningTime / 60.0f), 2);
  else
    tft.print("0.00");
  tft.print(" km/h");

  y += 25;
  tft.setCursor(0, y);
  tft.print(F(strDrivDist));
  tft.setCursor(270, y);
  tft.print(strecke, 2);
  tft.print(" km");

  y += 25;
  tft.setCursor(0, y);
  tft.print(F(strDrivTime));
  tft.setCursor(270, y);
  tft.print(runningTime, 0);
  tft.print(" min");
}

void drawSettingsView()
{
  tft.fillRect(120, 50, 270, 0 + 210, backColor);

  // skip boot logo default = false
  tft.setFont(Arial_14);
  tft.setTextColor(foreColor);
  tft.setCursor(5, 60);
  tft.print(F(strSkipLogo));
  tft.print("  ");
  tft.setTextColor(0xFF4A);
  if (skipBoot)
    tft.print("true");
  else
    tft.print("false");

  // startup LCD brigthness default = 255 = 100%
  tft.setFont(Arial_14);
  tft.setTextColor(foreColor);
  tft.setCursor(5, 105);
  tft.print(F(strMaxLcdBr));
  tft.print("  ");
  tft.setTextColor(0xFF4A);
  tft.print(maxLcdBright / 2.55f, 0);
  tft.print(" %");

  // second LCD brigthness default = 100 = 39%
  tft.setFont(Arial_14);
  tft.setTextColor(foreColor);
  tft.setCursor(5, 150);
  tft.print(F(strMinLcdBr));
  tft.print("  ");
  tft.setTextColor(0xFF4A);
  tft.print(minLcdBright / 2.55f, 0);
  tft.print(" %");

  // reduce volume in R-gear (default = true)
  tft.setFont(Arial_14);
  tft.setTextColor(foreColor);
  tft.setCursor(5, 195);
  tft.print(F(strhandleRVolume));
  tft.print("  ");
  tft.setTextColor(0xFF4A);
  if (handleRVolume)
    tft.print("true");
  else
    tft.print("false");

  // play critical alarms (default = true)
  tft.setFont(Arial_14);
  tft.setTextColor(foreColor);
  tft.setCursor(5, 240);
  tft.print(F(strPlayAlarms));
  tft.print("  ");
  tft.setTextColor(0xFF4A);
  if (playAlarms)
    tft.print("true");
  else
    tft.print("false");
}

//-------------------------------------------------------------//
//--------------------------InfoPopUp--------------------------//
//-------------------------------------------------------------//
void drawInfo()
{
  for (uint8_t i = 0; i < 10; i++)
  {
    if (strcmp(disabledInfos[i], actualInfo.Text) == 0)
    {
      return;
    }
  }

  if (actualInfo.Prio >= oldInfo.Prio || millis() - infoWritten >= oldInfo.Dur)
  { // wenn neu info-prio höher als alte info-prio, oder alte Info abgelaufen
    if (actualView == HOME_VIEW)
    { // draw infos only at homeview
      oldInfo = actualInfo;
      if (actualInfo.enabled)
      {
        uint16_t xOffset = 0;
        if (actualInfo.Img != NULL)
        {
          xOffset = 35;
        }

        tft.setFont(Arial_20);
        int pxLen = tft.strPixelLen(actualInfo.Text);
        tft.fillRoundRect(230 - ((pxLen + xOffset) / 2), 140, (pxLen + xOffset) + 20, 40, 5, foreColor);
        tft.drawRoundRect(230 - ((pxLen + xOffset) / 2) + 2, 140 + 2, (pxLen + xOffset) + 20 - 4, 40 - 4, 3, actualInfo.TextColor);

        actualInfo.X = 230 - ((pxLen + xOffset) / 2);
        actualInfo.Y = 140;
        actualInfo.W = (pxLen + xOffset) + 20;
        actualInfo.H = 40;

        tft.setTextColor(actualInfo.TextColor);
        tft.setFont(Arial_20);
        tft.setCursor(240 - ((pxLen + xOffset) / 2) + xOffset, 150); // middle screen
        tft.print(actualInfo.Text);

        if (actualInfo.Img != NULL)
        {
          tft.writeRect(240 - ((pxLen + xOffset) / 2), 145, 30, 30, actualInfo.Img);
        }

        if (!actualInfo.firstDrawn)
        {
          infoWritten = millis();
          actualInfo.firstDrawn = true;
        }

        if (millis() - infoWritten >= actualInfo.Dur)
        {
          actualInfo.enabled = false;
          tft.fillRect(0, 140, 480, 40, backColor);
        }
      }
    }
  }
}

//-------------------------------------------------------------//
//------------------------Calculations-------------------------//
//-------------------------------------------------------------//
float getConsum()
{
  float injSize = 470.0f;           // [ccm/min] @ referencePressure
  float referencePressure = 300.0f; // [kPa]
  float newSize = 470.0f;           // [ccm/min] @ realPressure
  float realPressure = 320.0f;      // [kPa]
  float injCal = 0.57;              // injectors battery offset calibration value

  // calculate injectors flow rate
  newSize = injSize * sqrt(realPressure / referencePressure);

  // injectors battery offset calibration
  if (vBatt > 14)
  {
    // 0.72 @ 14V, 0.63 @ 15V
    injCal = map(vBatt, 14, 15, 0.72, 0.63);
  }
  else if (vBatt > 13)
  {
    // 0.84 @ 13V, 0.72 @ 14V
    injCal = map(vBatt, 13, 14, 0.84, 0.72);
  }
  else if (vBatt >= 12)
  {
    // 0.97 @ 12V, 0.84 @ 13V
    injCal = map(vBatt, 12, 13, 0.97, 0.84);
  }
  else if (vBatt < 12)
  {
    // 1.13 @ 11V, 0.97 @ 12V
    injCal = map(vBatt, 11, 12, 1.13, 0.97);
  }

  float consum = (inj - injCal) * (float)rpm * newSize * 0.12f / 60000.0f;
  if (consum < 0)
    consum = 0.0f;

  // calculate absolute consume
  absolutConsume += consum / 3600000.0f * (float)(millis() - lastConsumeTime); //[l/h] / ms = [l/ms] * delta[ms] = l/deltaTime
  lastConsumeTime = millis();

  addValue(consumeArray, consum);

  return consum;
}

// float getAFR()
// {
//   float afrC = 14.7f;
//   float rohBenzin = 0.759f;                                     //[kg/l] @ 0°C
//   rohBenzin = rohBenzin * (1.0f - (float)ambientTemp * 0.001f); // Wärmeausdehnung
//   float kgBenzin = rohBenzin * consume;

//   afrC = maf / kgBenzin;

//   return afrC;
// }

float getLambda()
{
  float milliVoltMap[3][8] = {
      {1000, 980, 945, 830, 100, 40, 25, 20}, // AGT 500°C
      {930, 910, 865, 740, 130, 55, 35, 25},  // AGT 750°C
      {875, 850, 790, 660, 150, 65, 45, 30}}; // AGT 900°C

  int tempIndex;

  if (maf >= 650)
  { // AGT 900°C
    tempIndex = 2;
  }
  else if (maf >= 250)
  { // AGT 750°C
    tempIndex = 1;
  }
  else
  { // AGT 500°C
    tempIndex = 0;
  }

  float lambdaMilliVolt = lambdaVolt * 1000.0f;

  if (lambdaMilliVolt >= milliVoltMap[tempIndex][0])
  {
    return 0.70f;
  }
  else if (lambdaMilliVolt >= milliVoltMap[tempIndex][1])
  {
    return map(lambdaMilliVolt, milliVoltMap[tempIndex][1], milliVoltMap[tempIndex][0], 70, 80) / 100.0f;
  }
  else if (lambdaMilliVolt >= milliVoltMap[tempIndex][2])
  {
    return map(lambdaMilliVolt, milliVoltMap[tempIndex][2], milliVoltMap[tempIndex][1], 80, 90) / 100.0f;
  }
  else if (lambdaMilliVolt >= milliVoltMap[tempIndex][3])
  {
    return map(lambdaMilliVolt, milliVoltMap[tempIndex][3], milliVoltMap[tempIndex][2], 90, 100) / 100.0f;
  }
  else if (lambdaMilliVolt >= milliVoltMap[tempIndex][4])
  {
    return 1.0f;
  }
  else if (lambdaMilliVolt >= milliVoltMap[tempIndex][5])
  {
    return map(lambdaMilliVolt, milliVoltMap[tempIndex][5], milliVoltMap[tempIndex][4], 100, 110) / 100.0f;
  }
  else if (lambdaMilliVolt >= milliVoltMap[tempIndex][6])
  {
    return map(lambdaMilliVolt, milliVoltMap[tempIndex][6], milliVoltMap[tempIndex][5], 110, 120) / 100.0f;
  }
  else if (lambdaMilliVolt >= milliVoltMap[tempIndex][7])
  {
    return map(lambdaMilliVolt, milliVoltMap[tempIndex][7], milliVoltMap[tempIndex][6], 120, 129) / 100.0f;
  }
  else
  {
    return 1.30f;
  }
}

float getEngineRunningMinutes()
{
  float runningTime;
  if (EngineStartTime == 0)
    runningTime = 0.0f;
  else
    runningTime = (float)(millis() - EngineStartTime) / 60000.0f;

  return runningTime;
}

float getEngineNoiseLimit()
{
  float noiseLimit = 0;

  if (rpm > 5710)
  {
    noiseLimit = 2.61;
  }
  else if (rpm > 5328)
  {
    noiseLimit = map(rpm, 5328, 5710, 2.51, 2.61);
  }
  else if (rpm > 4947)
  {
    noiseLimit = map(rpm, 4947, 5328, 2.41, 2.51);
  }
  else if (rpm > 4566)
  {
    noiseLimit = map(rpm, 4566, 4947, 2.22, 2.41);
  }
  else if (rpm > 4183)
  {
    noiseLimit = map(rpm, 4183, 4566, 2.00, 2.22);
  }
  else if (rpm > 3802)
  {
    noiseLimit = map(rpm, 3802, 4183, 1.82, 2.00);
  }
  else if (rpm > 3421)
  {
    noiseLimit = map(rpm, 3421, 3802, 1.61, 1.82);
  }
  else if (rpm > 3039)
  {
    noiseLimit = map(rpm, 3039, 3421, 1.45, 1.61);
  }
  else if (rpm > 2657)
  {
    noiseLimit = map(rpm, 2657, 3039, 1.35, 1.45);
  }
  else if (rpm > 1894)
  {
    noiseLimit = map(rpm, 1894, 2657, 1.39, 1.35);
  }
  else if (rpm > 750)
  {
    noiseLimit = map(rpm, 750, 1894, 1.37, 1.39);
  }

  return noiseLimit;
}

void speedHandle()
{
  actSpeed = speed;
  if (actSpeed > 0)
  {
    actSpeed = (actSpeed * (100 - tachoabw)) / 100;
  }

  if (EngineStartTime > 0)
  {                                        // Motor Läuft, Strecke berechnen
    float span = millis() - lastSpeedTime; // Zeit zwischen zwei Geschwindigkeitsangaben
    odoMeter += (float)actSpeed / 3600000.0f * span;
    strecke += (float)actSpeed / 3600000.0f * span;
    lastSpeedTime = millis();
  }

  if (oldactSpeed - actSpeed >= 3)
  { // Fzg wird langsamer
    // Beschleunigungszeiten zurücksetzen
    t100Started = false;
    t200Started = false;
  }

  if (!gotT100)
  {
    if (t100Started)
    {
      // Uhr läuft
      if (actSpeed >= 100)
      {
        // Beschleunigung gestartet
        t100Stop = millis();
        t100 = t100Stop - t100Start;
        gotT100 = true;
      }
    }
    else
    {
      // Noch nicht gestartet
      if (oldactSpeed <= 0 && actSpeed >= 1)
      {
        // Beschleunigung gestartet
        t100Start = millis();
        t100Started = true;
      }
    }
  }

  if (!gotT200)
  {
    if (t200Started)
    {
      // Uhr läuft
      if (actSpeed >= 200)
      {
        // Beschleunigung gestartet
        t200Stop = millis();
        t200 = t200Stop - t200Start;
        gotT200 = true;
      }
    }
    else
    {
      // Noch nicht gestartet
      if (oldactSpeed <= 99 && actSpeed >= 100)
      {
        // Beschleunigung gestartet
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
void drawGraph(int x, int y, int w, int h, int vmin, int vmax, char *title, int *values)
{
  int tXsize;

  // Background
  // tft.writeRect(x, y, w, h, graph_bg);
  tft.fillRect(x, y, w, h, backColor); // for debug
  tft.fillRectVGradient(x + 2, y + 2, w - 4, 62, 0x3186, backColor);
  tft.fillRectVGradient(x + 2, y + 95, w - 4, 22, backColor, 0x3186);
  tft.drawRoundRect(x, y, w, h, 5, 0x5AEB);
  tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 5, 0x5AEB);

  // Title
  tft.setTextColor(foreColor);
  tft.setFont(Arial_12_Bold);
  tXsize = tft.strPixelLen(title);
  tft.setCursor(x + w / 2 - (tXsize / 2), y + 101);
  tft.print(title);

  // x and y axis
  tft.drawLine(x + 37, y + 7, x + 37, y + 96, 0x632D);      // vertical grey line
  tft.drawLine(x + 38, y + 7, x + 38, y + 96, 0x632D);      // vertical grey line
  tft.drawLine(x + 37, y + 96, x + w - 10, y + 96, 0x632D); // horizontal grey line
  tft.drawLine(x + 37, y + 97, x + w - 10, y + 97, 0x632D); // horizontal grey line

  // white segments
  tft.drawLine(x + 37, y + 12, x + 44, y + 12, foreColor); // white segment
  tft.drawLine(x + 37, y + 33, x + 44, y + 33, foreColor); // white segment
  tft.drawLine(x + 37, y + 54, x + 44, y + 54, foreColor); // white segment
  tft.drawLine(x + 37, y + 75, x + 44, y + 75, foreColor); // white segment
  tft.drawLine(x + 37, y + 96, x + 44, y + 96, foreColor); // white segment

  // grey segments
  tft.drawLine(x + 45, y + 12, x + w - 10, y + 12, 0x632D); // grey segment
  tft.drawLine(x + 45, y + 33, x + w - 10, y + 33, 0x632D); // grey segment
  tft.drawLine(x + 45, y + 54, x + w - 10, y + 54, 0x632D); // grey segment
  tft.drawLine(x + 45, y + 75, x + w - 10, y + 75, 0x632D); // grey segment
  tft.drawLine(x + 45, y + 96, x + w - 10, y + 96, 0x632D); // grey segment

  // segment vals
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

  // Graph
  int lineWidth = w - 45; // 268
  int xOffset = x + 41;
  int yOffset = y + 96; //=132
  int _x = 0;
  int _y = 0;
  int _y2 = 0;
  float xIncrement = (float)lineWidth / (float)arrayLength; // abstand zwischen x-punkten
  float yDivisor = (float)(vmax - vmin) / 84.0f;
  for (int i = 0; i < arrayLength - 1; i++)
  {
    _x = lineWidth - xIncrement * (float)i + xOffset - xIncrement;
    _y = (float)values[i] / yDivisor;
    _y2 = (float)values[i + 1] / yDivisor;

    tft.drawLine(_x, yOffset - _y, _x - xIncrement, yOffset - _y2, 0x07E0);
    tft.drawLine(_x, yOffset - _y + 1, _x - xIncrement, yOffset - _y2 + 1, 0x07E0);
  }
}

/**
   Summary:
     draws a gauge looks like Nissan GT-R digital gauges
     width = 154 and height = 119 pixels
   Parameter:
     value:
       the actual value to show on gauge
     vmin:
       the minimal value (start of gauge)
     vmax:
       the maximal value (end of gauge)
     x, y:
       the position of left top gauge corner on screen
     units:
       the unit of value as string
     vCritical:
       the start of critical value area (red marked on gauge and red drawn value if reached)
     title:
       the titel of shown value
     maxVal:
       the start alarm value
     decis:
       number of decimal places vor value
     alarm:
       bool play alarm sound if maxVal is reached
*/
void gtrMeter(float value, float vmin, float vmax, int x, int y, char *units, float vCritical, char *title, int maxVal, int decis, bool alarm)
{
  int w = 154;
  int h = 119;
  int segValDecis = decis > 0 ? 1 : 0;

  // Hintergrund
  // tft.writeRect(x, y, 154, 119, gauge_bg); //with red crit marker
  tft.writeRect(x, y, 154, 119, gauge_bg_nr); // without red crit marker

  // Segment Values
  float valSegment = (float)(vmax - vmin) / 8.0f;
  int tXsize;
  float segval;
  bool multipl = false;
  tft.setTextColor(foreColor);
  tft.setFont(Arial_11);

  // segval 8
  segval = vmax;
  if (segval > 1000)
    segval /= 100.0f, multipl = true;
  tXsize = tft.strPixelLen(floatToChar(segval, segValDecis));
  tft.setCursor(x + 141 - tXsize / 2, y + 85);
  tft.print(segval, segValDecis);

  // segval 6
  tft.setCursor(x + 125, y + 26);
  segval = valSegment * 6.0f + (float)vmin;
  if (multipl)
    segval /= 100.0f;
  tft.print(segval, segValDecis);

  // segval 4
  segval = valSegment * 4.0f + (float)vmin;
  if (multipl)
    segval /= 100.0f;
  tXsize = tft.strPixelLen(floatToChar(segval, segValDecis));
  tft.setCursor(x + 80 - tXsize / 2, y + 5);
  tft.print(segval, segValDecis);

  // segval 2
  segval = valSegment * 2.0f + (float)vmin;
  if (multipl)
    segval /= 100.0f;
  tXsize = tft.strPixelLen(floatToChar(segval, segValDecis));
  tft.setCursor(x + 35 - tXsize, y + 26);
  tft.print(segval, segValDecis);

  // segval 0
  segval = vmin;
  if (multipl && segval != 0)
    segval /= 100.0f;
  tXsize = tft.strPixelLen(floatToChar(segval, segValDecis));
  tft.setCursor(x + 16 - tXsize / 2, y + 85);
  tft.print(segval, segValDecis);

  if (multipl)
  {
    tft.setCursor(x + 119, y + 5);
    tft.print("x100");
  }

  int v = map(value, vmin, vmax, 0, 180);      // Map the value to an angle v
  int vC = map(vCritical, vmin, vmax, 0, 180); // Map the vCritical to an angle vC
  int vMax = map(maxVal, vmin, vmax, 0, 180);  // Map the vMax to an angle vMax

  // reduce angles to visible area
  if (v < 0)
    v = 0;
  else if (v > 180)
    v = 180;

  // draw red critical area marker
  float startAngle = (vC <= 0 ? 3 : vC) - 180;
  float endAngle = -3;

  if (title == (char *)"FUEL TANK")
  {
    startAngle = -177;
    endAngle = vC - 180;
  }

  if (startAngle > endAngle)
    startAngle -= 360;
  float steps = 0.2;
  int x2, y2;
  float xc, yc, xc2, yc2;
  xc = (float)x + (float)w / 2.0f;
  yc = (float)y + (float)h - 37.0f;
  for (float i = 0.0f + steps; i < endAngle - startAngle - steps; i += steps)
  {
    // prevent drawing of full circle
    if (vCritical >= vmax)
      break;

    // skip at white lines
    if ((startAngle + i >= 17 - 180 && startAngle + i <= 23 - 180) ||
        (startAngle + i >= 40 - 180 && startAngle + i <= 47 - 180) ||
        (startAngle + i >= 63 - 180 && startAngle + i <= 69 - 180) ||
        (startAngle + i >= 87.5 - 180.0f && startAngle + i <= 92.5 - 180.0f) ||
        (startAngle + i >= 111 - 180 && startAngle + i <= 117 - 180) ||
        (startAngle + i >= 133.5 - 180.0f && startAngle + i <= 140 - 180) ||
        (startAngle + i >= 157.5 - 180.0f && startAngle + i <= 163 - 180))
      continue;

    // get start and end coordinates
    x2 = (xc + 1) + cos((((float)startAngle + i) * PI) / 180.0f) * 58.0f;
    y2 = yc + sin((((float)startAngle + i) * PI) / 180.0f) * 58.0f;
    xc2 = (xc + 1) + cos((((float)startAngle + i) * PI) / 180.0f) * 55.0f;
    yc2 = yc + sin((((float)startAngle + i) * PI) / 180.0f) * 55.0f;

    tft.drawLine(xc2, yc2, x2, y2, ILI9486_RED);
    tft.drawPixel(x2, y2, 0xD800);   // make smooth border outside
    tft.drawPixel(xc2, yc2, 0xC000); // make smooth border inside
  }

  // draw value-ring
  tft.drawValRing(x + 28, y + 31, 100, 51, val_ring, v);

  // draw max val pointer
  if (maxVal >= vmin && maxVal <= vmax)
  {
    int x1, y1;
    x1 = xc + cos((((float)vMax - 180.0f) * PI) / 180.0f) * 56.0f;
    y1 = yc + sin((((float)vMax - 180.0f) * PI) / 180.0f) * 56.0f;
    tft.fillCircle(x1, y1 - 1, 2, 0xFFE0); // 0xFFE0 = gelb
  }

  // Calculate coords of ring centre
  x += w / 2;
  y += h / 2;

  // Convert value to a string
  char buf[10];
  byte len = 2;
  if (value > 9)
    len = 3;
  if (value > 99)
    len = 4;
  if (value > 999)
    len = 5;
  dtostrf(value, len, decis, buf);

  if (v >= vC && vCritical >= 0 && title != (char *)"FUEL TANK")
  {
    tft.setTextColor(ILI9486_RED);
  }
  else
    tft.setTextColor(foreColor);

  // Print value
  tft.setFont(Arial_20);
  if (decis == 0)
    tXsize = tft.strPixelLen(intToChar(value)), tft.setCursor(x - (tXsize / 2) - 7, y + h / 2 - 45);
  else
    tXsize = tft.strPixelLen(buf), tft.setCursor(x - (tXsize / 2), y + h / 2 - 45);
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

  // play alarm
  if (v >= vC && vCritical >= 0 && alarm)
  {
    alarmThreadID = threads.addThread(playAlarm);
  }
  else
  {
    if (threads.getState(alarmThreadID) == threads.RUNNING)
    {
      threads.kill(alarmThreadID);
    }
  }
}

void drawBar(int x, int y, char *title, int vMin, int vMax, int vYellow, int vRed, float value)
{
  float graphHeight = 240.0f;
  float graphWidth = 65.0f;
  float signedDelta = vMax - vMin;
  float deltaMinMax = (signedDelta > 0) ? signedDelta : -signedDelta;
  float stepWidth = (graphHeight - 26.0f) / deltaMinMax;
  float divisors = 6.0f;

  tft.drawLine(x + 23, y + 23, x + 23, y + graphHeight - 5, foreColor);                      // Y-Left-Line
  tft.drawLine(x + 23, y + graphHeight - 5, x + graphWidth, y + graphHeight - 5, foreColor); // X-Bottom-Line

  // Title
  tft.setTextColor(foreColor);
  tft.setFont(Arial_12_Bold);
  int tXsize = tft.strPixelLen(title);
  tft.setCursor(x + graphWidth / 2 - tXsize / 2, y);
  tft.print(title);

  // Scale values and divisors
  tft.setFont(Arial_10);
  for (int i = 0; i < divisors + 1; i++)
  {
    int scaleValue = (deltaMinMax / divisors * i) + vMin;
    tXsize = tft.strPixelLen(intToChar(scaleValue));
    tft.setCursor(x + 17 - tXsize, y + graphHeight - 10 - (i * (graphHeight - 28) / divisors));
    tft.print(scaleValue);

    tft.drawLine(x + 19, y + graphHeight - 5 - (i * (graphHeight - 28) / divisors), x + 22, y + graphHeight - 5 - (i * (graphHeight - 28) / divisors), foreColor); // Y-Divisor
  }

  // Bar
  tft.fillRect(x + 25, y + (graphHeight - 5) + -1 * ((value - vMin) * stepWidth), graphWidth - 25, (value - vMin) * stepWidth, 0x07E0); // green bar

  if (value >= vYellow)
  {
    tft.fillRect(x + 25, y + (graphHeight - 5) + -1 * ((value - vMin) * stepWidth), graphWidth - 25, ((value - vMin) * stepWidth) - ((vYellow - vMin) * stepWidth), 0xFFE0); // yellow bar
  }
  if (value >= vRed)
  {
    tft.fillRect(x + 25, y + (graphHeight - 5) + -1 * ((value - vMin) * stepWidth), graphWidth - 25, ((value - vMin) * stepWidth) - ((vRed - vMin) * stepWidth), 0xF800); // red bar
  }
}

//-------------------------------------------------------------//
//---------------------------Helpers---------------------------//
//-------------------------------------------------------------//
char *intToChar(int value)
{
  itoa(value, bufInt, 10);
  return bufInt;
}

char *floatToChar(float value, int decis)
{
  byte len = 3 + decis - 1;
  if (value >= 10)
    len = 4 + decis - 1;
  if (value >= 100)
    len = 5 + decis - 1;
  if (value >= 1000)
    len = 6 + decis - 1;
  if (value >= 10000)
    len = 7 + decis - 1;
  if (value >= 100000)
    len = 8 + decis - 1;
  if (value < 0)
    len = 4 + decis - 1;
  if (value <= -10)
    len = 5 + decis - 1;
  if (value <= -100)
    len = 6 + decis - 1;
  if (value <= -1000)
    len = 7 + decis - 1;

  dtostrf(value, len, decis, bufFloat);
  return bufFloat;
}

void addValue(int *arr, int value)
{
  for (int i = 39 - 1; i > 0; i--)
  {
    arr[i] = arr[i - 1];
  }
  arr[0] = value;
}

void addDisInfo(char *disText)
{
  for (int i = 10 - 1; i > 0; i--)
  {
    for (int c = 0; c < 35; c++)
    {
      disabledInfos[i][c] = disabledInfos[i - 1][c];
    }
  }

  for (int c = 0; c < 35; c++)
  {
    disabledInfos[0][c] = disText[c];
  }
}

void clearBtns()
{
  for (int i = 0; i < maxVisibleBtns; i++)
  {
    if (visibleBtns[i] != NULL)
    {
      visibleBtns[i]->makeInvisible();
    }
  }
}

void addButton(Button *btn)
{
  for (int i = maxVisibleBtns; i > 0; i--)
  {
    visibleBtns[i] = visibleBtns[i - 1];
  }

  visibleBtns[0] = btn;

  btn->drawButton(true, tft);
}

void bootAnimation()
{
  tft.fillScreen(backColor);
  tft.setCursor(0, 0);
  tft.setFont(Arial_12);
  tft.setTextColor(foreColor);
  tft.println("Author: S. Balzer");
  tft.print("Version: ");
  tft.println(bcVersion);

  for (int i = 0; i < 255; i += 15)
  {
    tft.drawPicBrightness(480 / 2 - 263 / 2, 320 / 2 - 133 / 2, 263, 133, (uint16_t *)opc, i);
    tft.updateScreen();
  }

  delay(800);

  // tone(pin, frequency, duration)
  tone(buzzer, 1650, 200);
  for (int i = 255; i > 0; i -= 30)
  {
    tft.drawPicBrightness(480 / 2 - 263 / 2, 320 / 2 - 133 / 2, 263, 133, (uint16_t *)opc, i);
    tft.updateScreen();
  }
}

void playAlarm()
{
  while (1)
  {
    if (playAlarms)
    {
      tone(buzzer, 2000, 150);
      threads.delay(500);
    }
  }
}

void screenshotToConsole()
{
  uint16_t buf[480];
  uint16_t w = tft.width();
  uint16_t h = tft.height();

  Serial.println();
  Serial.print(F("// Image Size     : "));
  Serial.print(w);
  Serial.print(F(" x "));
  Serial.print(h);
  Serial.println(F(" pixels"));
  Serial.print(F("const unsigned short screenShot["));
  Serial.print(w * h);
  Serial.println(F("]={"));

  // line by line
  for (uint16_t iy = 0; iy < h; iy++)
  {
    tft.readRect(0, iy, w, 1, buf);

    for (uint16_t ix = 0; ix < w; ix++)
    {
      Serial.print(F("0x"));
      Serial.print(buf[ix], HEX);
      if (ix < w - 1)
        Serial.print(F(", "));
    }

    if (iy < h - 1)
      Serial.print(F(","));
    Serial.println();
    delay(2);
  }

  Serial.println(F("}"));
}

byte ReadSerialMonitorString(char *sString)
{
  byte nCount = 0;

  if (Serial.available() > 0)
  {
    Serial.setTimeout(50);
    nCount = Serial.readBytes(sString, 5);
  }
  sString[nCount] = 0; // String terminator

  return nCount;
}

char sString[5]; // command from Serial
void receiveFromSerial()
{
  // Check for command from Serial
  if (ReadSerialMonitorString(sString) > 0)
  {
    if (strstr_P(sString, PSTR("<sh>")) != NULL)
    { // strstr_P keeps sString in flash; PSTR avoid ram using
      screenshotToConsole();
    }
    else
    {
      // undefined
      Serial.print("unknown command: ");
      Serial.println(sString);
    }
  }
}
//-------------------------------------------------------------//
//------------------------OBD-Functions------------------------//
//-------------------------------------------------------------//
void tech2()
{
  CAN_message_t can_MsgRx;

  while (1)
  { // Thread-Schleife
    if (!ecuConnected)
    { // wenn nicht verbunden, sende "7E0 01 20 0 0 0 0 0 0" bis "7E8 01 60 0 0 0 0 0 0" geantwortet wird
      while (ecuConnected == false)
      { // Solange nicht verbunden-Schleife
        sendEcuData(0x01, 0x20, 0, 0, 0, 0, 0, 0);

        threads.delay(500);

        if (ecuConnected)
        {
          threads.delay(15); // Warte 15ms, danach wird die Liste konfiguriert
        }
      }
    }
    else
    { // wenn verbunden
      if (listSet == false)
      {                                                              // wenn Datenliste noch nicht konfiguriert
        sendEcuData(0x10, 0x0C, 0xAA, 0x04, 0x10, 0x11, 0x12, 0x13); // Pakete 10, 11, 12, 13
        threads.delay(15);                                           // Warte 15ms
        sendEcuData(0x21, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x00); // Pakete 14, 15, 16, 17, 18, 19
        listSet = true;

        testerPresentThreadID = threads.addThread(testerPresent);
      }
      else
      {
        while (ecuConnected)
        {
          threads.delay(500);
        }
      }
    }
  }

  threads.yield();
}

void handleData500Msg(const CAN_message_t &can_MsgRx)
{
  if (can_MsgRx.id == CANID_DATAREPLY)
  { // Motordaten empfangen von 0x5E8

#ifdef PCOUT
    for (int i = 0; i < 8; i++)
    {
      // Serial.write((uint8_t)can_MsgRx.buf[i]);
      Serial.print(can_MsgRx.buf[i]);
      if (i < 7)
        Serial.print(",");
    }
    Serial.println();
#endif

    switch (can_MsgRx.buf[0])
    { // Datenpaketnummer
    case 0x03:
      isMilOn = can_MsgRx.buf[7];
      break;

    case 0x10:
      vBatt = can_MsgRx.buf[1] / 10.0f;
      rpm = ((can_MsgRx.buf[2] * 256.0f) + can_MsgRx.buf[3]) / 4.0f;
      speed = can_MsgRx.buf[7];
      speedHandle();

      if (rpm > rMAXrpm)
        rMAXrpm = rpm;
      if (rpm > 0 && EngineStartTime == 0)
      {
        EngineStartTime = millis();

        byte bytes[2];
        bytes[0] = (engineStarts + 1) & 0xFF;
        bytes[1] = ((engineStarts + 1) >> 8) & 0xFF;

        EEPROM.write(4, bytes[0]);
        EEPROM.write(3, bytes[1]);
      }
      if (speed > rMAXspeed)
        rMAXspeed = speed;

      break;

    case 0x11:
      iat = can_MsgRx.buf[4] - 40;
      ect = can_MsgRx.buf[2] - 40;
      ambientTemp = can_MsgRx.buf[5] - 40;

      if (iat > rMAXiat)
        rMAXiat = iat;
      if (ect > rMAXect)
        rMAXect = ect;
      break;

    case 0x12:
      maf = ((can_MsgRx.buf[2] * 256.0f) + can_MsgRx.buf[3]) / 100.0f * 3.6f;
      mafVolt = can_MsgRx.buf[1] / 51.0f;
      if (firstBoostValRec)
        firstBoostVal = can_MsgRx.buf[7], firstBoostValRec = false;
      if (can_MsgRx.buf[7] > firstBoostVal)
        boost = can_MsgRx.buf[7] - firstBoostVal;
      else
        boost = 0;

      power = maf * 0.383f;
      moment = ((float)power / 1.36f * 1000.0f) / (2.0f * 3.1415926f * rpm / 60.0f);

      addValue(mafArray, maf);
      addValue(boostArray, boost);
      if (power > rMAXpower)
        rMAXpower = power;
      if (moment > rMAXmoment)
        rMAXmoment = moment;
      if (maf > rMAXmaf)
        rMAXmaf = maf;
      if (rpm <= 5)
        boost = 0;
      if (boost > rMAXboost)
        rMAXboost = boost;
      break;

    case 0x13:
      // Pedal Position
      break;

    case 0x14:
      // Throttle Position

      inj = can_MsgRx.buf[7] / 10.0f; // Injektor Pulsweite

      if (rpm <= 5)
        inj = 0;
      injUt = inj * rpm / 1200.0f; // Duty Cycle
      if (injUt > rMAXinjUt)
        rMAXinjUt = injUt;
      if (inj > rMAXinj)
        rMAXinj = inj;
      getConsum();
      break;

    case 0x15:
      tankInhalt = (can_MsgRx.buf[7] / 2.55f - can_MsgRx.buf[6] / 256.0f);
      if (tankInhalt <= 5.0f)
      {
        actualInfo = info(strLowFuel, ILI9486_RED, 3, 5000, fuel);
      }
      break;

    case 0x16:
      ign = (can_MsgRx.buf[2] - 36.0f) / 10.0f;

      if (ign > rMAXign)
        rMAXign = ign;
      break;

    case 0x17:
      // Klopfsesor-Spannung = (A*5 + B/51) / 10  [V]
      knockVal = (can_MsgRx.buf[1] * 5.0f + can_MsgRx.buf[2] / 51.0f) / 10.0f;
      break;

    case 0x18:
      // Klopfregelung
      knockRet1 = can_MsgRx.buf[1];
      knockRet2 = can_MsgRx.buf[2];
      knockRet3 = can_MsgRx.buf[3];
      knockRet4 = can_MsgRx.buf[4];
      break;

    case 0x19:
      // Lambdasonde
      sft = (can_MsgRx.buf[1] - 128.0f) / 1.28;
      lambdaVolt = can_MsgRx.buf[3] / 51.0f;
      break;

    case 0x81:
      // Fehlercodes
      if (can_MsgRx.buf[4] == 0xFF)
      {
        // Ende der Übertragung
        answered = true;
      }
      else
      {
        fcs++; // Fehleranzahl zählen
        decodeFCs(can_MsgRx.buf, 0);
      }
      break;

    case 0xA9: // TODO test 0xA9
      // Fehlercodes
      if (can_MsgRx.buf[4] == 0xFF)
      {
        answered = true;
        // ende der Übertragung
      }
      else
      {
        fcs++; // Fehleranzahl zählen
        decodeFCs(can_MsgRx.buf, 1);
      }
      break;
    }
  }
  else if (can_MsgRx.id == CANID_REPLY)
  {
    if (can_MsgRx.buf[0] == 0x01)
    {
      ecuConnected = true;
    }
    if (can_MsgRx.buf[1] == 0x7E)
    {
      response = true;
    }
  }
}

void handleData33Msg(const CAN_message_t &can_MsgRx)
{
  if (can_MsgRx.id == 0x175)
  { // Lenkradfernbedienung
    if (can_MsgRx.buf[5] == 0x10 && can_MsgRx.buf[6] == 0x1F)
    { // UP
      // up pressed
      switch (actualView)
      {
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
    }
    else if (can_MsgRx.buf[5] == 0x20 && can_MsgRx.buf[6] == 0x01)
    { // DOWN
      // down pressed
      switch (actualView)
      {
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
    }
    else if (can_MsgRx.buf[5] == 0x30 && can_MsgRx.buf[6] == 0x00)
    { // ENTER
      // enter pressed
      if (actualView == MAX_VIEW)
      { // reset max vals
        rMAXect = 0;
        rMAXboost = 0;
        rMAXiat = 0;
        rMAXmaf = 0;
        rMAXign = 0;
        rMAXrpm = 0;
        rMAXinj = 0.0f;
        rMAXinjUt = 0;
        rMAXpower = 0;
        rMAXmoment = 0;

        drawMaxView();
      }
      else if (actualView == GRAPH_VIEW)
      { // reset max vals
        graphHold = true;
      }
    }
  }
  else if (can_MsgRx.id == 0x235)
  {
    // light signals
    if (can_MsgRx.buf[1] != 0x3A)
    {
      // Parking light off
      lcdBright = maxLcdBright;
    }
    else
    {
      // Parking light on
      lcdBright = minLcdBright;
    }
    analogWrite(3, lcdBright); // LCD LED
  }
  else if (can_MsgRx.id == 0x340)
  { // UEC - Scheinwerfer
    // Sort by priority in ascending order
    if (bitRead(can_MsgRx.buf[0], 2))
      actualInfo = info(strHBR, ILI9486_RED, 6, 5000, lights);
    if (bitRead(can_MsgRx.buf[0], 3))
      actualInfo = info(strHBL, ILI9486_RED, 6, 5000, lights);

    if (bitRead(can_MsgRx.buf[1], 6))
      actualInfo = info(strPLR, ILI9486_RED, 10, 5000, lights);
    if (bitRead(can_MsgRx.buf[1], 7))
      actualInfo = info(strPLL, ILI9486_RED, 10, 5000, lights);

    if (bitRead(can_MsgRx.buf[0], 0))
      actualInfo = info(strLBR, ILI9486_RED, 11, 5000, lights);
    if (bitRead(can_MsgRx.buf[0], 1))
      actualInfo = info(strLBL, ILI9486_RED, 11, 5000, lights);
  }
  else if (can_MsgRx.id == 0x350)
  { // Flüssigkeiten?
    // Sort by priority in ascending order
    if (bitRead(can_MsgRx.buf[0], 7))
      actualInfo = info(strWashLev, ILI9486_RED, 1, 5000, wash);
    if (bitRead(can_MsgRx.buf[1], 3))
      actualInfo = info(strCoolLev, ILI9486_RED, 15, 5000, coolant);
    if (bitRead(can_MsgRx.buf[1], 5))
      actualInfo = info(strBrFlE, ILI9486_RED, 17, 5000, brfl);

    // TODO Get Engine Oil (is a level sensor)
  }
  else if (can_MsgRx.id == 0x360)
  { // REC - Rücklichter
    // Sort by priority in ascending order
    if (bitRead(can_MsgRx.buf[1], 0))
      actualInfo = info(strRFL, ILI9486_RED, 5, 5000, lights);

    if (bitRead(can_MsgRx.buf[0], 2))
      actualInfo = info(strRevLR, ILI9486_RED, 7, 5000, lights);
    if (bitRead(can_MsgRx.buf[0], 3))
      actualInfo = info(strRevLL, ILI9486_RED, 7, 5000, lights);

    if (bitRead(can_MsgRx.buf[0], 4))
      actualInfo = info(strTSRR, ILI9486_RED, 8, 5000, lights);
    if (bitRead(can_MsgRx.buf[0], 5))
      actualInfo = info(strTSRL, ILI9486_RED, 8, 5000, lights);

    if (bitRead(can_MsgRx.buf[0], 0))
      actualInfo = info(strRLR, ILI9486_RED, 12, 5000, lights);
    if (bitRead(can_MsgRx.buf[0], 1))
      actualInfo = info(strRLL, ILI9486_RED, 12, 5000, lights);

    if (bitRead(can_MsgRx.buf[0], 6))
      actualInfo = info(strBLR, ILI9486_RED, 13, 5000, lights);
    if (bitRead(can_MsgRx.buf[0], 7))
      actualInfo = info(strBLL, ILI9486_RED, 13, 5000, lights);
  }
  else if (can_MsgRx.id == 0x445)
  { // Außentemp
    aat = can_MsgRx.buf[1] / 2.0f - 40.0f;
  }
  else if (can_MsgRx.id == 0x430)
  { // Info-Signale
    if (can_MsgRx.buf[0] == 0x62)
    { // R-Gang drin
      // Radio leiser machen
      if (!rGang && handleRVolume)
      {
        for (int i = 0; i < 10; i++)
        {
          sendBus33Data(0x175, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x1F);
          if (i < 9)
            threads.delay(100);
        }
      }
      distanceBehind = can_MsgRx.buf[2]; // 255 = max distance, 0 = min distance
      lastRGang = millis();
      rGang = true;
    }
    else if (can_MsgRx.buf[0] == 0x22)
    {
      // R-Gang raus
      rOut = true;
    }
  }
}

void testerPresent()
{
  int testerPresentTimeOut = 419; // alle 419ms Herzschlag senden
  threads.delay(testerPresentTimeOut);
  while (ecuConnected)
  {
    sendEcuData(0x01, 0x3E, 0, 0, 0, 0, 0, 0);
    response = false;
    threads.delay(testerPresentTimeOut);

    if (response == false)
    { // alles zurücksetzen
      noResponseCount++;
      if (noResponseCount >= 6)
      {
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
        injUt = 0;
        power = 0;
        listSet = false;
        ecuConnected = false;
        threads.kill(testerPresentThreadID);
        break;
      }
    }
    else
    { // prog laufen lassen
      noResponseCount = 0;
    }
    threads.yield();
  }
}

void sendEcuData(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4, uint8_t byte5, uint8_t byte6, uint8_t byte7)
{
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
  TxMsg.flags.overrun = 0;
  TxMsg.flags.reserved = 0;
  TxMsg.len = 8;
  TxMsg.id = CANID_REQUEST;

  // send request
  if (!can500.write(TxMsg))
  {
    actualInfo = info(strNoEcu, ILI9486_RED, 3, 5000);
  }
}

void sendBus33Data(uint16_t id, uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4, uint8_t byte5, uint8_t byte6, uint8_t byte7)
{
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
  TxMsg.flags.overrun = 0;
  TxMsg.flags.reserved = 0;
  TxMsg.len = 8;
  TxMsg.id = id;

  // send request
  if (!can33.write(TxMsg))
  {
    // Serial.println("Senden fehlgeschlagen");
  }
}

void clearFCs()
{
  fcs = 0;
  sendEcuData(0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

  if (actualView == FC_VIEW)
  {
    // clear screen
    tft.fillRect(0, 35, 480, 285, backColor);
    tft.setCursor(0, 38);
    tft.setFont(Arial_12);
    tft.setTextColor(foreColor);
    tft.println(strDelDTCs);
    tft.updateScreen();

    threads.delay(2000); // Warte 2sec

    tft.fillRect(0, 35, 480, 285, backColor);
    tft.setCursor(0, 38);
    tft.setFont(Arial_12);
    tft.setTextColor(foreColor);

    // request DTCs
    sendEcuData(0x03, 0xA9, 0x81, 0x12, 0x00, 0x00, 0x00, 0x00);

    unsigned long requestTime = millis();
    answered = false;

    // recive with timeout
    while (millis() - requestTime < 1500 && !answered)
      ;

    if (actualView == FC_VIEW && !answered)
    {
      tft.println(strNoResp);
    }
    else if (actualView == FC_VIEW && fcs <= 0 && answered)
    { // wenn keine Fehlercodes, Info anzeigen
      tft.println(strNoDTCs);
    }
  }
}

void requestFCs()
{
  // kill tech2- and testerPresent-thread
  threads.kill(tech2ThreadID);
  threads.kill(testerPresentThreadID);

  threads.delay(50);

  // Disconnect Datamonitor
  sendEcuData(0x02, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
  threads.delay(50);

  // request DTCs
  fcs = 0;
  sendEcuData(0x03, 0xA9, 0x81, 0x12, 0x00, 0x00, 0x00, 0x00);

  unsigned long requestTime = millis();
  answered = false;

  // recive with timeout
  while (millis() - requestTime < 3000 && !answered)
  {
    can500.events();
  }

  if (actualView == FC_VIEW && !answered)
  {
    tft.println(strNoResp);
  }
  else if (actualView == FC_VIEW && fcs <= 0 && answered)
  { // wenn keine Fehlercodes, Info anzeigen
    tft.println(strNoDTCs);
  }
}

void decodeFCs(const byte buff[], const int stat)
{
  if (actualView == FC_VIEW)
  {
    tft.print(intToChar(fcs)); // Fehlernummer (1. , 2. , 3.)
    tft.print(". ");           // Fehlernummer (1. , 2. , 3.)

    byte firstByte = (byte)buff[1];
    byte secondByte = (byte)buff[2];

    byte fSystem = firstByte >> 6;           // P, C, B, U
    byte specific = (firstByte >> 4) & 0x03; // 0 or 1
    byte subSystem = firstByte & 0x0f;       // 0 to 7
    byte thirdNum = secondByte >> 4;
    byte fourthNum = secondByte & 0x0f;

    char desc[3];
    sprintf(desc, "%02x", (byte)buff[3]); // descriptor (symptom)

    switch (fSystem)
    {
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
    } // P

    tft.print(specific, DEC);  // 0
    tft.print(subSystem, HEX); // 4
    tft.print(thirdNum, HEX);  // 0
    tft.print(fourthNum, HEX); // 0
    tft.print("-");            //-
    tft.print(desc);           // 5
    tft.print(" ");
    if (fSystem == 0 && specific == 0)
    { // P (Power train)
      switch (subSystem)
      {
      case 0:
        tft.print(F(strP00xx));
        break;
      case 1: // LMM, IAT, ECT, TPS, O2S
        switch (thirdNum)
        {
        case 0:
          tft.print(F(strP010x));
          break;
        case 1:
          if (fourthNum == 0)
          {
            tft.print(F(strP0110));
          }
          else if (fourthNum == 5)
          {
            tft.print(F(strP0115));
          }
          break;
        case 2:
          tft.print(F(strP012x));
          break;
        case 3:
          tft.print(F(strP013x));
          break;
        case 4:
          tft.print(F(strP014x));
          break;
        case 7:
          tft.print(F(strP017x));
          break;
        }
        break;

      case 2: // INJ, RPM, APP, FP, BPS, BPV,
        switch (thirdNum)
        {
        case 0:
          tft.print(F(strP020x));
          tft.print(" ");
          tft.print(fourthNum, DEC);
          break;
        case 1:
          tft.print(F(strP021x));
          break;
        case 2:
          tft.print(F(strP022x));
          break;
        case 3:
          if (fourthNum == 0)
          {
            tft.print(F(strP0230));
          }
          else if (fourthNum == 5)
          {
            tft.print(F(strP0235));
          }
          break;
        case 4:
          tft.print(F(strP024x));
          break;
        }
        break;
      case 3:
        if (thirdNum == 0 && fourthNum == 0)
        { // P0300
          tft.print(F(strP0300));
        }
        else
        {
          switch (thirdNum)
          {
          case 0:
            tft.print(F(strP030x));
            tft.print(" ");
            tft.print(fourthNum, DEC);
            break;
          case 1:
            tft.print(F(strP031x));
            break;
          case 2:
            tft.print(F(strP032x));
            break;
          case 3:
            tft.print(F(strP033x));
            break;
          case 4:
            tft.print(F(strP034x));
            break;
          }
        }
        break;

      case 4:
        if (thirdNum == 4)
          tft.print(F(strP044x));
        else if (thirdNum == 6)
          tft.print(F(strP046x));
        break;

      case 5:
        switch (thirdNum)
        {
        case 0:
          tft.print(F(strP050x));
          break;
        case 2:
          tft.print(F(strP052x));
          break;
        case 3:
          tft.print(F(strP053x));
          break;
        case 7:
          tft.print(F(strP057x));
          break;
        }
        break;

      case 6:
        switch (thirdNum)
        {
        case 0:
          tft.print(F(strP060x));
          break;
        case 2:
          tft.print(F(strP062x));
          break;
        case 5:
          tft.print(F(strP065x));
          break;
        }
        break;

      case 7:
        tft.print(F(strP07xx));
        break;

      default:
        tft.print(F(strUnknwn));
        break;
      }
    }
    else
    {
      tft.print(F(strUnknwn));
    }
    tft.println();
  }
}
