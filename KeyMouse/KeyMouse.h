/*
 * Copyright(C) 2021 Isao Hara, All rights reserved.
 *  License: The MIT License
 */
#define ACCELOMATER 0
#define BLE_SERVER  1
#define HID_PROJECT 1

#include <map>
#include <string>

#include <TFT_eSPI.h>                 // Hardware-specific library
#include <SPI.h>

#if BLE_SERVER
#include "rpcBLEDevice.h"
#include <BLE2902.h>
#endif

#if ACCELOMATER
#include"LIS3DHTR.h" 
#endif

#include "Seeed_FS.h"
#include "SD/Seeed_SD.h"

#if HID_PROJECT
#  define   HID_CUSTOM_LAYOUT
#  define   LAYOUT_JAPANESE
#  include  "HID-Project.h"
#else
#  include "Keyboard.h"
#  include "Mouse.h"
#  define KeyboardKeycode(n)  (n)
#endif

#define MAX_NUM_LIST    6

#define str_list  std::vector<std::string>
#define str_map   std::map<std::string, std::string>

#if BLE_SERVER
/*** BLE ****/
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

/*--- BLE Service ----*/
BLECharacteristic * pTxChara;
BLECharacteristic * pRxChara;
#endif

// read buffer for TxCaracterristic
std::string rxValue;

// response delay of the mouse, in ms
int responseDelay;

/*----- for Mouse ------------------*/
/// State of stick
int upState;
int downState;
int rightState;
int leftState;
int pushState;

/// State of button
int buttonLeftState;
int buttonMiddleState;
int buttonRightState;

/*----------------define the button pins---------------------------*/ 
// the time record paramas
unsigned long _currentMillis;
unsigned long _previousMillis;

/*------------- Functions -------------------*/
// a delay function uses millis()
void My_delay(int Time);

// Display message 
void showInfoMessage(String  msg, int pos);
void showTitle(String  msg, int fl);
void showMenu(int item);
void drawTextMsg(String msg, int y, int flag, int fsize);

/* --- for Keyboard ---*/
int PressSpecialJpKey(unsigned char ch);
int CheckFuncKey(char *buf, int size);
void KeyPress2(int key_val);
void KeyPress(unsigned char ch);
int CheckMouseVal(char *buf, int len);
void procMultiBytesEvent(char *buf, int len);
