  /*    
 * A demo for Wio Terminal to simulate mouse by buttons.
 * Such as Mouse Up, Mouse Down, Mouse Left, Mouse Right,
 * Click the left mouse button, Click the right mouse button, 
 * Up roll, Down roll and etc.
 * Copyright (c) 2020 seeed technology co., ltd.  
 * Author      : weihong.cai (weihong.cai@seeed.cc)  
 * Create Time : July 2020 
 * Change Log  :  
 *
 * The MIT License (MIT) 
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublifoodly
 * cense, and/or sell
 * copies of the Software, and to permit persons to whom the Software istm
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *     
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS INcommInterface
 * THE SOFTWARE.
 */
/*
 * Copyright (c) 2021 Isao Hara
 * 
 */
#include "KeyMouse.h"


TFT_eSPI tft = TFT_eSPI();            // Invoke custom library
TFT_eSprite spr = TFT_eSprite(&tft);  // Sprite 

/*----------------define the button pins---------------------------*/ 
const int upButton          = WIO_5S_UP;
const int downButton        = WIO_5S_DOWN;
const int leftButton        = WIO_5S_LEFT;
const int rightButton       = WIO_5S_RIGHT;
const int pushButton        = WIO_5S_PRESS;

const int mouseBttonRight   = BUTTON_1;
const int mouseBttonMiddle  = BUTTON_2;
const int mouseButtonLeft   = BUTTON_3;

// output range of X or Y movement; affects movement speed          
int g_range = 20;

// for mode
enum mode_name{
  MOUSE_MODE=0, KEYIN_MODE, KEYIN_JP_MODE,
  KEYPAD_MODE, KEYPAD_JP_MODE,
  CONFIG_MODE
};
auto menu_list = str_list();
int state_config = 0;
int selected_mode = MOUSE_MODE;


#if BLE_SERVER
/*----------- BLE Service Callbacks ------------------*/
// BLE Service
BLEServer *pServer = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

//
// Callback for BLE connection
class BleServerConnection: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer)    { deviceConnected = true; };
  void onDisconnect(BLEServer* pServer) { deviceConnected = false; }
};

//
// Callback for event
class BleKeyEventCallback: public BLECharacteristicCallbacks {
  // Data comming...
  void onWrite(BLECharacteristic *pCharacteristic){
    rxValue = pCharacteristic->getValue();
    int len = rxValue.length();
      
    if(len >= 3){
      char *buf = (char *)rxValue.c_str();
      procMultiBytesEvent(buf, len);
       
    } else if (len == 1){ // Keyboard
      unsigned char ch=rxValue[0];
      if (PressSpecialJpKey(ch) == 0){  KeyPress(ch); }
      responseDelay = 20;
    }
    return;
  }
  
  //
  void onRead(BLECharacteristic *pCharacteristic){
    return;
  }
};

//
void setup_BLE_server()
{
  //-------- Initialize BLE for remote keyboard
  BLEDevice::init("Remote Keyboard Service");
 
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new BleServerConnection());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // Create a BLE Characteristic
  // Trasmit Event
  pTxChara = pService->createCharacteristic(CHARACTERISTIC_UUID_TX,
                   BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ );
  pTxChara->setAccessPermissions(GATT_PERM_READ);      
  pTxChara->addDescriptor(new BLE2902());
 
  /// Receive Event
  pRxChara = pService->createCharacteristic(CHARACTERISTIC_UUID_RX,
                   BLECharacteristic::PROPERTY_WRITE);
  pRxChara->setAccessPermissions(GATT_PERM_READ | GATT_PERM_WRITE);           
  pRxChara->setCallbacks(new BleKeyEventCallback());
  
  // Start the service
  pService->start();
 
  // Start advertising
  pServer->getAdvertising()->start();
}
#endif

/************** Hardware Events ****************************/
// INPUT_PULLUP Mode
bool isPressed(int x){
  return (x == LOW);
}

/// for 5 states joystick of WioTerminal
enum button_state{
  NONE_STATE=-1, 
  UP_STATE=0x41, DOWN_STATE, RIGHT_STATE, LEFT_STATE,
  PUSH_STATE=0x51
};

int g_joystick_state = NONE_STATE;
int get_joystick_state()
{
  if (  isPressed(leftState) ) {
    return LEFT_STATE;
  }else if (  isPressed(downState) ) {
   return DOWN_STATE;
  }else if (  isPressed(upState) ) {
    return UP_STATE;
  }else if (  isPressed(rightState) ) {
    return RIGHT_STATE; 
  }else  if ( isPressed(pushState) ) {
     return PUSH_STATE;
  }else{
     return NONE_STATE;
  }
}

//  for Upper buttons
#define NONE_BUTTON     0x00
#define LEFT_BUTTON     0x01
#define MIDDLE_BUTTON   0x02
#define RIGHT_BUTTON    0x04
#define LEFT_MIDDLE_BUTTON  (LEFT_BUTTON | MIDDLE_BUTTON)

uint8_t g_button_state = 0;
void get_button_state()
{
  g_button_state = NONE_BUTTON;
  if (isPressed(buttonLeftState)  ){ g_button_state = g_button_state | LEFT_BUTTON; }
  if (isPressed(buttonMiddleState)){ g_button_state = g_button_state | MIDDLE_BUTTON; }
  if (isPressed(buttonRightState) ){ g_button_state = g_button_state | RIGHT_BUTTON; }
  return;
}

/* Default messages */
static const char *ble_state_str[] = { "[[ BLE Disconnected ]]", "[[ BLE Connected ]]" };

//// for SD card message file
char msg_file[] = "message.txt"; 
int ready_sd = 0;
auto msg_item_list = str_list();
auto msg_list = str_list();

char msg_jp_file[] = "message_jp.txt"; 
auto msg_jp_item_list = str_list();
auto msg_jp_list = str_list();

#define KEYPAD_RAW 4
#define KEYPAD_COLUMN 8
static const char key_buttons[KEYPAD_RAW][KEYPAD_COLUMN]= {
  {'@','A','B','C','D','E', 'F','G'},
  {'H','I','J','K','L','M', 'N','O'},
  {'P','Q','R','S','T','U', 'V','W'},
  {'X','Y','Z','[','\\',']','^','_'}
};

int selected_node = 0;
int nlist = 0;
int selected_page = 0;
int npage = 1;

int sel_x = 0;
int sel_y = 0;

#if ACCELOMATER
/// for 3-axis acceleromater inside WioTerminal
LIS3DHTR<TwoWire> accelerometer;
#endif

std::map<char, char> E2J = {
    {'"', '@'}, {'&', '^'}, {'\'', '&'}, {'(', '*'}, { ')', '('}, 
    {'=', '_'}, {'^', '='}, {'~',  '+'}, {'@', '['}, { '`', '{'},
    {'[', ']'}, {'{', '}'}, {'+',  ':'}, {':', '\''}, {'*', '"'},
    {']', '\\'},{'}', '|'}, {'\\', 0x89},{'|', 0x88}, {'_', 0x87}};

std::map<int, KeyboardKeycode> KeyArrow = { { UP_STATE, KEY_UP_ARROW },
    { DOWN_STATE, KEY_DOWN_ARROW }, { LEFT_STATE, KEY_LEFT_ARROW },
    { RIGHT_STATE, KEY_RIGHT_ARROW }};

std::map<char, KeyboardKeycode> KeyPad ={
    {'e',KEY_ENTER}, {'u',KEY_UP_ARROW}, {'d',KEY_DOWN_ARROW},
    {'l',KEY_LEFT_ARROW},{'r',KEY_RIGHT_ARROW},{'b',KEY_BACKSPACE}};  
    
/////////////////////////////// Initialize
void setup() {
  // initialize the buttons' inputs:
  pinMode(upButton,         INPUT);
  pinMode(downButton,       INPUT);
  pinMode(leftButton,       INPUT);
  pinMode(rightButton,      INPUT);
  pinMode(pushButton,       INPUT);
  
  pinMode(mouseButtonLeft,  INPUT);
  pinMode(mouseBttonMiddle, INPUT);
  pinMode(mouseBttonRight,  INPUT);
  
  // Initialize Display
  tft.begin();
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
 
  // Create Sprite
   spr.fillSprite(TFT_BLACK);
   //spr.createSprite(TFT_HEIGHT, TFT_WIDTH);
   spr.createSprite(280, 200);

  // Menu, 
  menu_list.push_back("Mouse mode");
  menu_list.push_back("KeyIn mode");
  menu_list.push_back("KeyIn(JP) mode");
  menu_list.push_back("KeyPad mode");
  menu_list.push_back("KeyPad(JP) mode");
  
  // initialize mouse control:
  Mouse.begin();

  // initalize Keyboard
  Keyboard.begin();

#if BLE_SERVER
  //-------- Initialize BLE for remote keyboard
  setup_BLE_server();
#endif

#if ACCELOMATER
  //------ Setup 3-axis accelerometer
  accelerometer.begin(Wire1);
  accelerometer.setOutputDataRate(LIS3DHTR_DATARATE_25HZ); 
  accelerometer.setFullScaleRange(LIS3DHTR_RANGE_2G);
#endif

  // SD Card
  ready_sd = checkSdCard(3);
  readMessageFile(msg_file, msg_list, msg_item_list);
  //readMessageFile(msg_jp_file, msg_jp_list, msg_jp_item_list);

}

///////////////////////////////// Main Loop
void loop() {
  char msg[25]="--------";  /// Message buffer
  
  updateButtonEvents();

  // calculate the movement distance based on the button states:
  int xDistance = (leftState - rightState);
  int yDistance = (upState   - downState);

  g_joystick_state = get_joystick_state();
  char ch;

  get_button_state();
  responseDelay = 5;

  /////----- for all mode 
  if (g_button_state == (LEFT_BUTTON | RIGHT_BUTTON) )
  {
    showMenu(CONFIG_MODE);
    state_config = CONFIG_MODE;
    responseDelay = 100;
  }

  //// Mode Operations
  if (state_config == MOUSE_MODE){
    /*------------------Mouse Move--------------------------------------*/
    // if X or Y is non-zero, move:
    if( g_button_state == LEFT_BUTTON  ){
      if(ArrowKeyPress() == 0){ ArrowFuncPress(); }
      responseDelay = 150;
    }else if( g_button_state == LEFT_MIDDLE_BUTTON ){
      ArrowFuncPress();
      responseDelay = 150;
    }else{
      Mouse.move(xDistance * g_range, yDistance * g_range, 0);
      procMouseButton();
    }

    /*----- Show message --------*/
    showTitle("Mouse & keyboard:", 0);
    flushDisplay();
    sprintf(msg, "Sp:%d, Key:%s(%d) %d, %d\n", 
        g_range, rxValue.c_str(), rxValue.size(),
        g_button_state, responseDelay);
    showInfoMessage(msg, 0); 

  } else if (state_config  == KEYIN_MODE 
          || state_config  == KEYIN_JP_MODE ) {  
    //// KeyIn Mode (disable joystick mouse)
    responseDelay = 0;

    if( g_button_state == LEFT_BUTTON  ){
      ArrowKeyPress();
    }else if(g_button_state == LEFT_MIDDLE_BUTTON ){
      ArrowFuncPress();
    }else {
      switch(g_joystick_state){
        case PUSH_STATE:
          outputString(msg_list[selected_node]);
          break;
        default:
          selected_node = selectListItem(selected_node, yDistance, nlist);
          break;
      }
    }
    /*------- Display ----*/
    showTitle("Keyboard Mode", 0);
    showMenuItems(msg_item_list, selected_node);
    flushDisplay();
    if (ready_sd == 0){   sprintf(msg, " No SD card");        }
    else{                 sprintf(msg, "File: %s", msg_file); }
    showInfoMessage(msg, 0);
 
    responseDelay += 120;
    
  } else if (state_config == KEYPAD_MODE 
          || state_config == KEYPAD_JP_MODE){
    ////--- KeyPad mode (simple keyboard, ASCII code table)
    if(g_button_state == LEFT_BUTTON ){
      if(ArrowKeyPress() == 0){
        ArrowFuncPress();
      }
    }else{
      switch(g_joystick_state){
        case PUSH_STATE:
          ch = key_buttons[sel_y][sel_x];
          if (g_button_state == MIDDLE_BUTTON){ ch -= 0x20; }
          else if (g_button_state == RIGHT_BUTTON){ ch += 0x20; } 
          if( state_config  == KEYPAD_JP_MODE && E2J[ch]){ ch = E2J[ch]; }
          Keyboard.write(ch);
          break;
 
        default:
          if(g_button_state == LEFT_MIDDLE_BUTTON ){ ArrowFuncPress(); }
          else{
            sel_x = selectListItem(sel_x, xDistance, KEYPAD_COLUMN);
            sel_y = selectListItem(sel_y, yDistance, KEYPAD_RAW);
          }
          break;
      }
    }
    //--- Display
    showTitle("Keytype Mode", 0);
    displayKeypad(sel_x, sel_y);
    flushDisplay();

    responseDelay = 120;
    
  } else if (state_config == CONFIG_MODE) {
    //// Configuration Mode
    switch(g_joystick_state){
        case PUSH_STATE:
          state_config = selected_mode;
          break;
        default:
          selected_mode = selectListItem(selected_mode, yDistance, menu_list.size());
          break;
    } 

    // show Mouse Menu
    showMenu(selected_mode);
    sprintf(msg, "Selected: %d , %d\n", selected_mode, yDistance);
    showInfoMessage(msg, 0);
    responseDelay = 120;
  }
  
#if BLE_SERVER
  /*---------- BLE connection -------*/
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
      delay(500);                  // give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // restart advertising
      oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
     oldDeviceConnected = deviceConnected;
  }
#endif
  /*-----------------------------------------------------------------*/ 
  // a delay so the mouse doesn't move too fast:
  My_delay(responseDelay);
}


/*------------- Functions -------------------*/
// a delay function uses millis()
void My_delay(int Time)
{
  while((_currentMillis - _previousMillis) <= Time)
  {
      _currentMillis = millis();
  }
  _previousMillis = _currentMillis; 
}

/// Set display fonts
void setFontSize(int fsize)
{
  switch(fsize)
  {
    case 9:
     spr.setFreeFont(&FreeSerif9pt7b);
     break;
    case 12:
     spr.setFreeFont(&FreeSerif12pt7b);
     break;
    case 18:
     spr.setFreeFont(&FreeSerif18pt7b);
     break; 
    case 24:
     spr.setFreeFont(&FreeSerif24pt7b);
     break; 
  }
  return;
}

// Show message 
void showInfoMessage(String  msg, int pos)
{
  spr.fillSprite(TFT_BLACK);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  setFontSize(9);
  spr.drawString(msg, 20, pos*20);
  spr.pushSprite(10, 220);
}

void showTitle(String  msg, int fl)
{
  // Show Title Message
  spr.fillSprite(TFT_BLACK);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.setFreeFont(&FreeSansBoldOblique12pt7b);
  spr.drawString(msg,10 ,5); 
  if (fl == 1){
    spr.pushSprite(0, 0);
  }
#if BLE_SERVER
  drawTextMsg(ble_state_str[deviceConnected], 0, 0, 9);
#endif
  return;
}

void flushDisplay()
{
   spr.pushSprite(0, 0);
}
 
void showMenu(int item)
{
  showTitle("Menu", 0);
  showMenuItems(menu_list, item);
  flushDisplay();
}

int selectListItem(int sel, int dist, int n_list){;
  return min(max(sel+dist, 0), n_list -1);
}

void showMenuItems(std::vector<std::string> menu, int item)
{
  int n_menu=menu.size();
  int n_start = max(item - MAX_NUM_LIST +1, 0);
  // show Mouse Menu
  setFontSize(9);
  for(int i=0; i < n_menu && i < MAX_NUM_LIST;i++){
    drawTextMsg(menu[i+n_start].c_str(), i+1, (item == (i+n_start)), 0);
 }
 return;
}


void drawTextMsg(String msg, int y, int flag, int fsize)
{
  if (flag){
    spr.setTextColor(TFT_BLACK, TFT_WHITE);
  }else{
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
  }
  setFontSize(fsize);
  spr.drawString(msg, 20, 40+y*20);
}

void drawTextMsg2(String msg, int x, int y, int flag)
{
  int x_step=24;
  int y_step=24;
  if (flag){
    spr.setTextColor(TFT_BLACK, TFT_WHITE);
  }else{
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
  }
  spr.drawString(msg, 40+x*x_step, 42+y*y_step);
}

void displayKeypad(int sel_x, int sel_y)
{
  char ch;
  setFontSize(12);
  for(int j=0; j< KEYPAD_RAW; j++){
    for(int i=0; i< KEYPAD_COLUMN; i++){
      ch=key_buttons[j][i];
      if (g_button_state == MIDDLE_BUTTON){
        ch -= 0x20;
      }else if (g_button_state == RIGHT_BUTTON){
        ch += 0x20;
      }
      if(ch <0x20 || ch>0x7e){ ch=' '; }     
      drawTextMsg2(String((char)ch), i, j+2, ((sel_x==i) && (sel_y==j)));
    }
  }
  return;
}
/*---- for Mouse Mode ----*/
void updateButtonEvents()
{
  // read the button state:
  upState            = digitalRead(upButton);
  downState          = digitalRead(downButton);
  rightState         = digitalRead(rightButton);
  leftState          = digitalRead(leftButton);
  pushState          = digitalRead(pushButton);
  buttonMiddleState  = digitalRead(mouseBttonMiddle);
  buttonRightState   = digitalRead(mouseBttonRight);
  buttonLeftState    = digitalRead(mouseButtonLeft);
   
  return;
}

void procMouseButton()
{
  /*-------------Mouse Config Button  Click--------------------------*/
  if(g_button_state == MIDDLE_BUTTON && g_joystick_state == PUSH_STATE){
    if(g_range == 20){ g_range = 2; }else{ g_range = 20; }
    responseDelay=250;
    return;
  }
    
  /*-------------Mouse Button Left Click---------------------------*/
  if (g_button_state == LEFT_MIDDLE_BUTTON )
  {
    Mouse.release(MOUSE_LEFT);  
    if ( g_joystick_state == UP_STATE ){           // Scroll Up
      Mouse.move(0, 0, 1);
    }else if ( g_joystick_state == DOWN_STATE ) {  // Scroll Down
      Mouse.move(0, 0, -1);
    }
    responseDelay = 100;
    return;
    
  } else {
    // if the mouse button left is pressed:
    if (g_button_state == NONE_BUTTON && g_joystick_state == PUSH_STATE )
    {
      if (!Mouse.isPressed(MOUSE_LEFT)) { Mouse.press(MOUSE_LEFT); }
      responseDelay = 100;
      return;
    } else { 
      if (Mouse.isPressed(MOUSE_LEFT))  { Mouse.release(MOUSE_LEFT); }
    }
  }
  
  /*-------------Mouse Button Right Click-----------------------------*/
  // if the mouse button right is pressed:
  if (g_button_state == RIGHT_BUTTON)
  {
    if (!Mouse.isPressed(MOUSE_RIGHT)) { Mouse.press(MOUSE_RIGHT); }
    responseDelay=100;
  } else {
    if (Mouse.isPressed(MOUSE_RIGHT))  { Mouse.release(MOUSE_RIGHT); }
  }

  return;  
}

/* --- for Keyboard Mode ---*/
////------ read SD card ----------------
int checkSdCard(int timeout)
{
  int count=0;
  while(1){
    if(!SD.begin(SDCARD_SS_PIN, SDCARD_SPI)){
      delay(1000);
      count += 1;
      if (count > timeout){ break; }
    }else{
      return 1;   
    }
  }
  return 0;
}


void parseMessageEntry(std::string item_buf, str_list& list, str_list& item_list)
{
  auto separator2 = std::string("\t");
  auto pos2 = item_buf.find(separator2, 0);
  if(pos2 == std::string::npos){ 
    list.push_back(item_buf);
    item_list.push_back(item_buf);
  }else{
    item_list.push_back(item_buf.substr(0, pos2));
    list.push_back(item_buf.substr(pos2+1));
  }
  return;
}

void readMessageFile(char *fname, str_list& list, str_list& item_list)
{
  std::string msg_buf;
  if (ready_sd){
    File myFile = SD.open(fname, FILE_READ);
    if (myFile){
      while(myFile.available()){
        msg_buf += myFile.read();
      }
    }

    // parse string
    if (msg_buf.length() > 0){
      auto separator = std::string("\n");
      auto separator_length = separator.length();
      auto offset = std::string::size_type(0);
      nlist = 0;
      
      while (1) {
        auto pos = msg_buf.find(separator, offset);
        if (pos == std::string::npos) {
          if(msg_buf.substr(offset).length() > 0){
            parseMessageEntry(msg_buf.substr(offset), list, item_list);
            nlist += 1;
          }
          break;
        }
        parseMessageEntry(msg_buf.substr(offset, pos - offset), list, item_list);
        offset = pos + separator_length;
        nlist += 1;
      }
    }
  }
  return;
} 
////------ read SD card :END----------------
///--- Joystick event process
int ArrowKeyPress()
{
  if (KeyArrow[g_joystick_state]){
     Keyboard.write(KeyArrow[g_joystick_state]);
     return 1;
  }
  return 0;
}

int ArrowFuncPress(){
  switch(g_joystick_state){
    case PUSH_STATE:
      Keyboard.write(KEY_ENTER);
      break;
    case LEFT_STATE:
      Keyboard.write(KEY_BACKSPACE);
      break;
    case RIGHT_STATE:
      outputString(" ");
      break;
    default:
      return 0;
  }
  return 1;
}

////----- Key Press
/*
 * Check Special Key:
 * -- JP keylayout--
 *   HID_KEYBOARD_INTERNATIONAL1|MOD_LEFT_SHIFT,   // 135(0x87) - Unused
 *   HID_KEYBOARD_INTERNATIONAL3|MOD_LEFT_SHIFT,   // 136(0x88) - Unused
 *   HID_KEYBOARD_INTERNATIONAL3,                  // 137(0x89) - Unused
 *   KEY_LANG5,                                    // 138(0x8a) - Unused
 *   KEY_TILDE|MOD_LEFT_ALT,                       // 139(0x8b) - Unused
 */
int PressSpecialJpKey(unsigned char ch)
{
  uint8_t chval = ch;
  if(ch >= 0x87 && ch <= 0x8b){
    Keyboard.write(ch);
    return 1;
  }
  return 0;
}

/*
 * Function key
 */
int CheckFuncKey(char *buf, int size)
{
  if(size < 3)  return -1;
  if (buf[0] != 0x1b or buf[1] != 0x5b) return -1;
  if(size == 3){
    switch(buf[2]){
      case 0x41:  // ^
         return KEY_UP_ARROW;
      case 0x42:  // v
         return KEY_DOWN_ARROW;
      case 0x43: //  ->
         return KEY_RIGHT_ARROW;
      case 0x44:  // <-
         return KEY_LEFT_ARROW;
      default:
         return -1;
    }
  }else if(size == 4){
    if(buf[3] == 0x7e){
      switch(buf[2]){
        case 0x31:  // Home
          return KEY_HOME;
        case 0x32:  // Ins
          return KEY_INSERT;
        case 0x33: //  End
           return KEY_END;
        case 0x34:  // Up
           return KEY_PAGE_UP;
        case 0x35:  // DOWN
           return KEY_PAGE_DOWN;
        default:
           return -1;
      }
    }
  }else{
    if(buf[4] == 0x7e){
      if (buf[2] == 0x31){
        switch(buf[3]){
          case 0x31:
            return KEY_F1;
          case 0x32:
            return KEY_F2;
          case 0x33:
            return KEY_F3;
          case 0x34:
            return KEY_F4;
          case 0x35:
            return KEY_F5;
          case 0x36:
            return KEY_F6;
          case 0x37:
            return KEY_F7;
          case 0x38:
            return KEY_F8;
          default:
            return -1;
        }
      }
      if(buf[2] == 0x32){     
        switch(buf[3]){
          case 0x30:
            return KEY_F9;
          case 0x31:
            return KEY_F10;
          case 0x32:
            return KEY_F11;
          case 0x33:
            return KEY_F12;
          default:
            return -1;
        }
      }
    }
  }
  return -1;
}

/*
 * Send key stroke to USB port
 */
void KeyPress2(int key_val)
{
  Keyboard.press(KeyboardKeycode(key_val));
  delay(10);
  Keyboard.releaseAll();
  responseDelay = 20;
}

/*
 *  Send key stroke to USB port
 *  
 */
void KeyPress(unsigned char ch)
{
    switch(ch){
      case '\r':
        Keyboard.write(KEY_RETURN);
        break;
      case 0x09:
        Keyboard.write(KEY_TAB);
        break;    
      case 0x1b:
        Keyboard.write(KEY_ESC);
        break;
      case 0x08:
        Keyboard.write(KEY_BACKSPACE);
        break;
      case 0x7f:
        Keyboard.write(KEY_DELETE);
        break;

      default:
        if (ch > 0 && ch < 0x1b){
          Keyboard.press(KEY_LEFT_CTRL);
          Keyboard.press(ch + 0x60);
          delay(20);
          Keyboard.releaseAll();
        }else{
          Keyboard.write(ch);
        }
        break;
    }
}
///----------------


/*
 * Check mouse event value
 */
int CheckMouseVal(char *buf, int len)
{
  if (len == 8){
    if (buf[0] == 0x1b && buf[1] == 0x5b && buf[2] == 0x6d && buf[7] == 0x7e){
      return 1; 
    }
  }
  return 0;
}

/*
 * output string
 */
void outputString(std::string buf)
{
  char ch;
  for(int i=0; i < buf.length(); i++){
    ch = buf[i];
    if( state_config == KEYIN_JP_MODE){
      if (E2J[buf[i]]){ ch = E2J[buf[i]]; }
    }
    Keyboard.write(ch);
  }
  responseDelay = 100;
  return;
}

#define P_ZERO  -128  
/*
 * Process key-event.
 */
void procMultiBytesEvent(char *buf, int len)
{
  int key_val = CheckFuncKey(buf, len);

  if (key_val > 0){
    KeyPress2(key_val);
    //responseDelay = 20;
          
  }else{
    if (CheckMouseVal(buf, len) == 1){ /// Mouse Event
      int x, y, w, b;
      b = buf[3];
      x = (int)buf[4] + P_ZERO;
      y = (int)buf[5] + P_ZERO;
      w = (int)buf[6] + P_ZERO;
          
      if( b == 0 ){ 
        if (x == P_ZERO && y == P_ZERO && w == P_ZERO){
          Mouse.release(MOUSE_ALL);
        }else{
          Mouse.move(x, y, w);
        }
      }else{
        int btn = MOUSE_LEFT;
        if (b == 2){  btn = MOUSE_RIGHT; }
        if (x == P_ZERO && y == P_ZERO && w == P_ZERO){
          if (Mouse.isPressed(btn) ){
            Mouse.release(btn);
          }else{
            Mouse.press(btn);
          }
        }else{
          Mouse.move(x, y, w);
        }
      }
      responseDelay = 10;
    }
  }
  return;
}

#if ACCELOMATER
//---- for 3-axis acceleromater
float rad2deg(float v){
  return v * 180/M_PI;
}
float conv_normal(float v){
  if(v > M_PI){
    return v - M_PI*2;
  }else if (v < -M_PI){
    return v + M_PI*2;
  }else{
    return v;
  }
}

float correct_pitch(float pitch, float acc[3])
{
  if(acc[2] > 0){
    if (pitch > 0) return (pitch - M_PI);
    else return (M_PI + pitch);
  }
  return -pitch;
}

void get_angle_from_acc(float *roll, float *pitch)
{
  float r,p;
  float acc[3];
  accelerometer.getAcceleration(&acc[0], &acc[1], &acc[2]);
  p = atan2( -acc[0], sqrt(acc[1]*acc[1]+acc[2]*acc[2])); 
  r = atan2( acc[1], acc[2]);
  *roll = rad2deg(conv_normal(r+M_PI));
  *pitch = rad2deg( correct_pitch(p, acc) );
  return;
}
#endif
/***** End of file ****/
