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

#define FOODLY 0

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
int range = 10;

// for mode
int state_config = 0;
int selected_mode = MOUSE_MODE;

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


/************** Hardware Events ****************************/
bool isPressed(int x){
  return (x == LOW);
}

enum button_state{
  NONE_STATE=-1, LEFT_STATE=0, RIGHT_STATE, DOWN_STATE, UP_STATE, PUSH_STATE
};

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

#define LEFT_BUTTON     0x01
#define MIDDLE_BUTTON   0x02
#define RIGHT_BUTTON    0x04
unsigned char get_button_state()
{
  unsigned char res = 0;
  if (  isPressed(buttonLeftState) ) { res = res | LEFT_BUTTON; }
  if (  isPressed(buttonMiddleState) ) {res = res | MIDDLE_BUTTON; }
  if (  isPressed(buttonRightState) ) { res = res | RIGHT_BUTTON; }
  return res;
}

/* messages */
static const char *ble_state_str[] = { "[[ BLE Disconnected ]]", "[[ BLE Connected ]]" };

#if FOODLY
/*--------- for foodly mode ------------*/
int foodly_rl_mode = 0;
int foodly_rot_mode = 0;
int arm_control_mode = 0;
int key_pressed = 0;
int foodly_menu_mode=0;

enum motion{
  M_LEFT = 0, M_RIGHT, M_BACK, M_FRONT,  M_DOWN, M_UP,
  ROLL_M,     ROLL_P,  PITCH_M, PITCH_P, YAW_M,  YAW_P,
  DISHER, END_MOTION
};

static char arm_motion[]= { 'a', 'f', 's', 'd', 'z', 'x',       // left move
                            'q', 'w', 'e', 'r', 'Q', 'W', 't',  // left ratate, disher
                            'h', 'l', 'j', 'k', ',', '.',       // right move
                            'o', 'p', 'u', 'i', 'U', 'I', 'y'}; // right rotate, disher

char get_command(int v)
{
  return arm_motion[foodly_rl_mode * END_MOTION + v];
}

int get_motion_index(int idx)
{
  int value = -1;

  switch(idx){
    case M_LEFT:
    case M_RIGHT:
       value = idx + foodly_rot_mode * 6;
       break;
    case M_BACK:
    case M_FRONT:
       value = idx + arm_control_mode * 2 + foodly_rot_mode * 6;
       break;
  }
  return value;
}

int toggle_value(int *v){
  *v = (*v + 1) % 2;
}

#endif

char msg_file[] = "message.txt"; 
int ready_sd = 0;
auto msg_item_list = std::vector<std::string>();
auto msg_list = std::vector<std::string>();
auto menu_list = std::vector<std::string>();
auto command_list = std::vector<std::string>();

int selected_node = 0;
int nlist = 0;  

LIS3DHTR<TwoWire> lis;

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
   spr.createSprite(240, 200);

  // SD Card
  ready_sd = checkSdCard(3);
  readMessageFile(msg_file);
  if (nlist > 7) { nlist=7; }

  // Menu
  menu_list.push_back("Mouse mode");
  menu_list.push_back("Keyboard mode");
  
#if FOODLY
  menu_list.push_back("Foodly mode");

  command_list.push_back("=");
  command_list.push_back("H");
  command_list.push_back("K");
  command_list.push_back("C");
  command_list.push_back("<");
  command_list.push_back(">");
  command_list.push_back(" ");
#endif
  // initialize mouse control:
  Mouse.begin();

  // initalize Keyboard
  //BootKeyboard.begin();
  Keyboard.begin();
  
  // Initialize BLE for remote keyboard
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

  //
  lis.begin(Wire1);
  lis.setOutputDataRate(LIS3DHTR_DATARATE_25HZ); 
  lis.setFullScaleRange(LIS3DHTR_RANGE_2G);
  
}
//----- Key

///////////////////////////////// Main Loop
void loop() {
  char msg[25]="--------";  /// Message buffer
  
  updateButtonEvents();

  // calculate the movement distance based on the button states:
  int xDistance = (leftState - rightState) * range;
  int yDistance = (upState   - downState)  * range;

  unsigned char button_state = get_button_state();
  int joystick_state = get_joystick_state();

  responseDelay = 5;

  //// Mode Operations
  if (state_config == MOUSE_MODE){
    /*------------------Mouse Move--------------------------------------*/
    // if X or Y is non-zero, move:
    if ( ( (xDistance != 0) || (yDistance != 0)) ) {
      Mouse.move(xDistance, yDistance, 0);
    }
    
    procMouseButton();
    
    /*----- Show message --------*/
    showTitle("Mouse & keyboard:", 0);
    
    drawTextMsg(ble_state_str[deviceConnected], 0, 0, 9);
    flushDisplay();
    
    sprintf(msg, "Mouse speed: %d, Keyin: %s(%d)\n", range, rxValue.c_str(), rxValue.size());
    showInfoMessage(msg, 0); 
    
  } else if (state_config  == KEYBOARD_MODE) { 
    //// Keyboard Mode (disable joystick mouse)
    responseDelay = 0;

    if(button_state == LEFT_BUTTON  ){
      switch(joystick_state){
        case UP_STATE:
          //KeyPress(KEY_UP_ARROW);
          Keyboard.write(KEY_UP_ARROW);
          break;
        case DOWN_STATE:
          //KeyPress(KEY_DOWN_ARROW);
          Keyboard.write(KEY_DOWN_ARROW);
          break;
        case LEFT_STATE:
          //KeyPress(KEY_LEFT_ARROW);
          Keyboard.write(KEY_LEFT_ARROW);
          break;
        case RIGHT_STATE:
          //KeyPress(KEY_RIGHT_ARROW);
          Keyboard.write(KEY_RIGHT_ARROW);
          break;          
      }

    }else {
      switch(joystick_state){
        case PUSH_STATE:
           if(msg_list.size() > selected_node){
             outputString(msg_list[selected_node]+"\n");
           }else{
             outputString("\n");
           }
           break;
           
        case LEFT_STATE:
           KeyPress2(0x08);
           break;
           
        case RIGHT_STATE:
           outputString(" ");
           break;
           
        case UP_STATE:
        case DOWN_STATE:
            selected_node += yDistance/2;
            if (selected_node < 0)    { selected_node = 0;    }
            if (selected_node > nlist){ selected_node = nlist; }
           break;
      }
    }

    /*------- Display ----*/
    showTitle("Keyboard Mode", 0);

    drawTextMsg(ble_state_str[deviceConnected], 0, 0, 9);
    
    // show string list
    showMenuItems(msg_item_list, selected_node);
    drawTextMsg("=[CR]=", nlist+1,  (selected_node == nlist), 0);
    flushDisplay();

    if (ready_sd == 0){
      sprintf(msg, " No SD card");
    }else{
      sprintf(msg, "File: %s", msg_file);
    }
    showInfoMessage(msg,0);
    responseDelay += 120;

#if FOODLY   
  } else if (state_config  == FOODLY_MODE) {
    //// Foodly Control with keyinput.
     char *rl_mode_str[] = {"Left", "Right" };
     char *arm_ctrl_str[] = { "[X-Y]", "[Y-X]" };
     char *rot_mode_str[] = { " Move", " Rot" };

    //-------- Change Mode 
    ///-- 3-Axis Sensor
    float acc[3], roll, pitch;

    
    
    lis.getAcceleration(&acc[0], &acc[1], &acc[2]);
    get_angle_from_acc(acc, &roll, &pitch);

    if (arm_control_mode == 0 && pitch > 70){
      arm_control_mode = 1;
    }else if (arm_control_mode == 1 && pitch < 30){
      arm_control_mode = 0;
    }

    //--- Button
    responseDelay = 50;
    if ( button_state == LEFT_BUTTON ) {
      if (foodly_rl_mode == 0){ toggle_value(&foodly_rot_mode); }
      foodly_rl_mode = 0;
      
    } else if (button_state == RIGHT_BUTTON ) {
      if (foodly_rl_mode == 1){ toggle_value(&foodly_rot_mode); }
      foodly_rl_mode = 1;
      
    } else if (button_state == MIDDLE_BUTTON ) {
      toggle_value(&foodly_menu_mode);   
    }else{
      responseDelay = 5;
    }

    char key = 0;
 
    //------ Display
    showTitle("Foodly Mode", 0);
    
    drawTextMsg(rl_mode_str[foodly_rl_mode], 0, 1, 9);
    drawTextMsg2(rot_mode_str[foodly_rot_mode], 4, 0, foodly_rot_mode);
    drawTextMsg2(arm_ctrl_str[arm_control_mode], 8, 0, arm_control_mode);
      
    if (foodly_menu_mode == 1){
      int cmd_list_len = command_list.size();
      showMenuItems2(command_list, selected_node, 2, 10);

      switch(joystick_state){
        case PUSH_STATE:
           if(command_list.size() > selected_node){
             if(command_list[selected_node] == "="){
               key = get_command(DISHER);
             }else{
               outputString(command_list[selected_node]);
             }
           }
           break;
        case LEFT_STATE:
        case RIGHT_STATE:
           selected_node += xDistance/2;
           if (selected_node < 0)    { selected_node = 0;    }
           if (selected_node >= cmd_list_len){ selected_node = cmd_list_len-1; }
           responseDelay = 100;
           break;           
        case UP_STATE:
        case DOWN_STATE:
           selected_node += yDistance/2;
           if (selected_node < 0)    { selected_node = 0;    }
           if (selected_node >= cmd_list_len){ selected_node = cmd_list_len-1; }
           responseDelay = 100;
           break;
           
        default:
           key_pressed = 0;
           break;
      }

    }else{
      if (joystick_state == NONE_STATE){
        key_pressed = 0;
      }else if(joystick_state == PUSH_STATE){
        key = ' ';
        drawTextMsg("Record Current Pos", 3, 0, 0); 
      }else{ 
        key = get_command(get_motion_index(joystick_state));
      }

   }

    ///  Press keyboard
    if (key > 0 && key_pressed == 0){
      KeyPress2(key);
      key_pressed = 1;
    }

    //-------------- Flush display
    flushDisplay();

    if (key > 64){
      sprintf(msg, "Mode: %d === %c ===", foodly_rl_mode, key);
    }else{
      sprintf(msg, "Roll:%3.1f, Pitch: %3.1f", roll, pitch);
    }
    showInfoMessage(msg,0);

    responseDelay += 100;
#endif

  } else if (state_config == CONFIG_MODE) {
    //// Configuration Mode
    selected_node = 0;
    if (  isPressed(pushState) ) {
      state_config = selected_mode;
      Mouse.release(MOUSE_LEFT);
    }else{
      selected_mode += yDistance/2;
      if (selected_mode < MOUSE_MODE)   { selected_mode = MOUSE_MODE;    }
      if (selected_mode >= CONFIG_MODE){ selected_mode = CONFIG_MODE -1; }
    }
    // show Mouse Menu
    showMenu(selected_mode);
    sprintf(msg, "Selected: %d , %d\n", selected_mode, yDistance);
    showInfoMessage(msg, 0);
    responseDelay = 120;
  }
  
  if (isPressed(buttonLeftState) && isPressed(pushState) ) {
    showMenu(CONFIG_MODE);
    state_config = CONFIG_MODE;
    responseDelay = 100;
  }
  
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

void showMenuItems(std::vector<std::string> menu, int item)
{
  int n_menu=menu.size();
  // show Mouse Menu
  setFontSize(9);
  for(int i=0; i < n_menu && i < 7;i++){
    drawTextMsg(menu[i].c_str(), i+1, (item == i), 0);
 }
 return;
}

void showMenuItems2(std::vector<std::string> menu, int item, int start_y, int max_w)
{
  int n_menu=menu.size();
  setFontSize(9);
  int x = 0;
  int y = start_y;
  for(int i=0; i < n_menu ;i++){
    drawTextMsg2(menu[i].c_str(), x , y, (item == i));
    x += menu[i].length()+1;
    if (x > max_w){
      x = 0;
      y += 1;
    }
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
  if (flag){
    spr.setTextColor(TFT_BLACK, TFT_WHITE);
  }else{
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
  }
  spr.drawString(msg, 20+x*12, 40+y*20);
}

/*---- for Mouse ----*/
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
  if (isPressed(buttonLeftState) ) { range = 2; }else{ range = 20; }
    
  /*-------------Mouse Button Left Click---------------------------*/
  if (isPressed(buttonMiddleState) && isPressed(buttonLeftState)) {
    Mouse.release(MOUSE_LEFT);  
    if ( isPressed(upState) ){           // Scroll Up
      Mouse.move(0, 0, 1);
      responseDelay = 100;
    }else if ( isPressed(downState) ) {  // Scroll Down
      Mouse.move(0, 0, -1);
      responseDelay = 100;
    }
      
  } else {
    // if the mouse button left is pressed:
    if (isPressed(buttonMiddleState) || isPressed(pushState) ) {
      if (!Mouse.isPressed(MOUSE_LEFT)) { Mouse.press(MOUSE_LEFT); }
    } else { 
      if (Mouse.isPressed(MOUSE_LEFT))  { Mouse.release(MOUSE_LEFT); }
    }
  }
  
  /*-------------Mouse Button Right Click-----------------------------*/
  // if the mouse button right is pressed:
  if (isPressed(buttonRightState) ) {
    if (!Mouse.isPressed(MOUSE_RIGHT)) { Mouse.press(MOUSE_RIGHT); }
  } else {
    if (Mouse.isPressed(MOUSE_RIGHT))  { Mouse.release(MOUSE_RIGHT); }
  }
  return;  
}

/* --- for Keyboard ---*/
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
 
void readMessageFile(char *fname)
{
  std::string msg_buf;
  if (ready_sd){
    File myFile = SD.open(fname, FILE_READ);
    if (myFile){
      while(myFile.available()){
        msg_buf += myFile.read();
      }
    }
    if (msg_buf.length() > 0){
      auto separator = std::string("\n");
      auto separator2 = std::string("\t");
      auto separator_length = separator.length();
      auto offset = std::string::size_type(0);
      nlist = 0;
      std::string item_buf;

      while (1) {
        auto pos = msg_buf.find(separator, offset);
        if (pos == std::string::npos) {
          if(msg_buf.substr(offset).length() > 0){
            item_buf = msg_buf.substr(offset);
            auto pos2 = item_buf.find(separator2,0);
            if(pos2 == std::string::npos){ 
              msg_list.push_back(item_buf);
              msg_item_list.push_back(item_buf);
            }else{
              msg_item_list.push_back(item_buf.substr(0, pos2));
              msg_list.push_back(item_buf.substr(pos2+1));
            }
            nlist += 1;
          }
          break;
        }

        item_buf = msg_buf.substr(offset, pos - offset);
        auto pos2 = item_buf.find(separator2, 0);
        if(pos2 == std::string::npos){ 
          msg_list.push_back(item_buf);
          msg_item_list.push_back(item_buf);
        }else{
          msg_item_list.push_back(item_buf.substr(0, pos2));
          msg_list.push_back(item_buf.substr(pos2+1));
        }
        offset = pos + separator_length;
        nlist += 1;
      }
    }else{
      msg_list.push_back("");
      msg_item_list.push_back("---");
      nlist=1;
    }
  }
  return;
} 

/*
 * Check Special Key
 */
int PressSpecialJpKey(unsigned char ch)
{
  uint8_t chval = ch;
    if (ch == 0x89){ // Backslash
      //Keyboard.writeRaw(chval, 0);
      Keyboard.write(KeyboardKeycode(0x89));
      return 1;

    }else if (ch == 0x87){ // Under score
      //Keyboard.writeRaw((uint8_t)0x87, (uint8_t)0x02);
      Keyboard.write(0x87);
      return 1;
    }else if (ch == 0x88 ){  // 0x88 Pipe
      //Keyboard.writeRaw((uint8_t)ch, 0);
      Keyboard.write(0x88);
      return 1;
      
    }else if (ch == 0x8a){
      //Keyboard.writeRaw((uint8_t)0x89, (uint8_t)0x02);
      Keyboard.write(KeyboardKeycode(0x94));
      
      return 1;
    }else if (ch == 0x8b){
      //Keyboard.writeRaw((uint8_t)0x89, (uint8_t)0x02);
      Keyboard.write(0x8b);
      return 1;

    }else{
      //Keyboard.write(KeyboardKeycode(chval));
      //Keyboard.writeRaw((uint8_t)ch, 0);
      return 0;
      
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
 * 
 */
void outputString(std::string buf)
{
  for(int i=0; i < buf.length(); i++){
     Keyboard.write(buf[i]);
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

#if FOODLY
/*--------- Rotation -------------*/
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

void get_angle_from_acc(float acc[3], float *roll, float *pitch)
{
  float r,p;
  p = atan2( -acc[0], sqrt(acc[1]*acc[1]+acc[2]*acc[2])); 
  r = atan2( acc[1], acc[2]);
  *roll = rad2deg(conv_normal(r+M_PI));
  *pitch = rad2deg( correct_pitch(p, acc) );
  return;
}
#endif
/***** End of file ****/