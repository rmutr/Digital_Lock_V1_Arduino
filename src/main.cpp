//----------------------------------------------------------------------------- 
// Project          : RMUTR Digital Lock V1 
// VSCode Extension : PlatformIO IDE 1.10.0 
// Source           : https://github.com/rmutr/Digital_Lock_V1_Arduino.git 
// Board            : Node32s (Gravitech Node32Lite LamLoei) 
// Additional URLs  : https://dl.espressif.com/dl/package_esp32_index.json 
// LED_BUILTIN      : Pin 2 


//----------------------------------------------------------------------------- 
#include <Arduino.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <LiquidCrystal_PCF8574.h>

#include <BLEDevice.h> 
#include <BLEServer.h> 
#include <BLEUtils.h> 
#include <BLE2902.h> 

#define PIN_RX_0             3 
#define PIN_TX_0             1 

#define I2C_SCL             22 
#define I2C_SDA             21 

#define PIN_PROCESS          2 
#define PIN_BTN_START       16 
#define PIN_ALARM            2 

#define PNP_OFF              0 
#define PNP_ON               1 
#define NPN_OFF              1 
#define NPN_ON               0 

char buff_str[200] = {0}; 
unsigned long t_old = 0; 
int tmr_cnt = 0; 
int wait_100ms = 0; 
int alarm_1sec = 0;
int error = 0; 

int state_ix = 0; 
int state_ix_mon = 0; 

int machine_run = 0; 
volatile int machine_stop_start_req = 0; 
int machine_req_wait_100ms = 0; 

//----------------------------------------------------------------------------- 
#define SCREEN_WIDTH       128 
#define SCREEN_HEIGHT       64 
#define OLED_RESET           4 
#define GLCD_LINE_1         20 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define GLCD_LINE_0          5 
#define GLCD_LINE_1         20 
#define GLCD_LINE_2         35 
#define GLCD_LINE_3         50 

//----------------------------------------------------------------------------- 
#define CLCD_ADD_27       0x27 
#define CLCD_ADD_3F       0x3F 

#define CLCD_LINE_0          0 
#define CLCD_LINE_1          1 

LiquidCrystal_PCF8574 clcd(CLCD_ADD_27); 

//----------------------------------------------------------------------------- 
BLEServer *pServer; 
BLECharacteristic *pTxCharacteristic; 
volatile bool bt_connected = false; 
volatile bool bt_connected_old = false; 
String bt_pincode_str = ""; 
std::string bt_rxvalue_str = ""; 
String bt_rxdata_str = ""; 
int bt_login = 0; 
String msg_connected_str = ""; 
String msg_pincode_str = ""; 

// See the following for generating UUIDs: 
// https://www.uuidgenerator.net/ 

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" 
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" 
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" 

//----------------------------------------------------------------------------- 
void Interrupt_Service_Btn_Start(); 

void Interrupt_Service_Btn_Start() { machine_stop_start_req = 1; } 

class MyServerCallbacks: public BLEServerCallbacks { 
  void onConnect(BLEServer* pServer) { 
    bt_connected = true; 
  }; 

  void onDisconnect(BLEServer* pServer) { 
    bt_connected = false; 
  } 
}; 

class MyCallbacks: public BLECharacteristicCallbacks { 
  void onWrite(BLECharacteristic *pCharacteristic) { 
    bt_rxvalue_str = pCharacteristic->getValue(); 
    bt_rxdata_str = String(bt_rxvalue_str.c_str());
  } 
}; 


//----------------------------------------------------------------------------- 
void setup() { 
  pinMode(PIN_PROCESS, OUTPUT); 
  pinMode(PIN_BTN_START, INPUT_PULLUP); 
  pinMode(PIN_ALARM, OUTPUT); 

  digitalWrite(PIN_PROCESS, HIGH); 
  digitalWrite(PIN_BTN_START, HIGH); 
  digitalWrite(PIN_ALARM, HIGH); 

  attachInterrupt(digitalPinToInterrupt(PIN_BTN_START) 
    , Interrupt_Service_Btn_Start, FALLING); 

  sprintf(buff_str, "Digital Lock V1"); 

  Serial.begin(115200); 
  Serial.println(buff_str); 

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); 
  display.setTextSize(1); 
  display.setTextColor(WHITE); 

  display.clearDisplay(); 
  display.setCursor(0, GLCD_LINE_0); 
  display.println(F(buff_str)); 
  display.display(); 

  clcd.begin(16, 2); 
  clcd.init(); 
  clcd.setBacklight(255); 
  clcd.setCursor(0, CLCD_LINE_0); 
  clcd.print(buff_str); 

//----------------------------------------------------------------------------- 
  BLEDevice::init(buff_str); 

  pServer = BLEDevice::createServer(); 
  pServer->setCallbacks(new MyServerCallbacks()); 

  BLEService *pService = pServer->createService(SERVICE_UUID); 

  pTxCharacteristic = pService->createCharacteristic( 
                        CHARACTERISTIC_UUID_TX, 
                        BLECharacteristic::PROPERTY_NOTIFY 
                      ); 

  pTxCharacteristic->addDescriptor(new BLE2902()); 

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic( 
                       CHARACTERISTIC_UUID_RX, 
                       BLECharacteristic::PROPERTY_WRITE 
                     ); 

  pRxCharacteristic->setCallbacks(new MyCallbacks()); 

  pService->start(); 

  pServer->getAdvertising()->start(); 
  Serial.println("Waiting a client connection to notify..."); 

//----------------------------------------------------------------------------- 
  t_old = 0; 
  tmr_cnt = 0; 
  wait_100ms = 0; 
  alarm_1sec = 0; 
  error = 0; 
  state_ix = 0; 
  state_ix_mon = 0; 
  machine_run = 0; 
  machine_stop_start_req = 0; 

  bt_connected = false; 
  bt_connected_old = false; 
  bt_pincode_str = "1234"; 
  bt_rxvalue_str = ""; 
  bt_login = 0; 
  msg_connected_str = "";
  msg_pincode_str = ""; 
} 

void loop() { 
//----------------------------------------------------------------------------- 
  if (bt_connected_old != bt_connected) { 
    bt_connected_old = bt_connected; 
    if (bt_connected == false) { 
      msg_connected_str = "-> Bluetooth disconnected"; 
      state_ix = 0; 
    } else { 
      msg_connected_str = "-> Bluetooth connected"; 
    } 
  } 

//----------------------------------------------------------------------------- 
  if (machine_stop_start_req == 1) { 
    machine_stop_start_req = 0; 
     if (machine_req_wait_100ms == 0) { 
       machine_req_wait_100ms = 50; 
       machine_run = !machine_run; 
     }
  } 

//----------------------------------------------------------------------------- 
  state_ix_mon = state_ix; 

  switch (state_ix) { 
    default: 
    case 0: 
      bt_login = 0; 
      state_ix++; 
      break; 

    case 1: 
      wait_100ms = 5; //<- Give the bluetooth stack the chance to get things ready 
      state_ix++;
      break; 

    case 2: 
      if (wait_100ms == 0) { state_ix++; } 
      break; 

    case 3: 
      pServer->startAdvertising(); //<- Restart advertising 
      state_ix++;
      break; 

    case 4: 
      if (bt_connected == true) { state_ix++; } 
      break; 

    case 5: 
      if (bt_rxdata_str.length() > 0) { 
        int bmsg_ok = 0; 

        if (bt_rxdata_str.length() >= 7) { 
          String bcmd_str = bt_rxdata_str.substring(0, 3); 
          String bpin_str = bt_rxdata_str.substring(3, 7); 
          int bcmd_len = bt_rxdata_str.length(); 

          if (bt_rxdata_str == "Hi, ESP32") { 
            bmsg_ok = 1; 
            msg_pincode_str = "-> Hi, Flutter"; 
          } 

          if ((bcmd_str == "C0-") && (bcmd_len == 7) && (bpin_str == bt_pincode_str)) { 
            bmsg_ok = 1; 
            msg_pincode_str = "-> Command : Machine Stop"; 
            bt_login = 1; 
            machine_run = 0; 
          } 

          if ((bcmd_str == "C1-") && (bcmd_len == 7) && (bpin_str == bt_pincode_str)) { 
            bmsg_ok = 1; 
            msg_pincode_str = "-> Command : Machine Start"; 
            bt_login = 1; 
            machine_run = 1; 
          } 

          if ((bcmd_str == "C2-") && (bcmd_len == 7) && (bpin_str == bt_pincode_str)) { 
            bmsg_ok = 1; 
            msg_pincode_str = "-> Command : Login"; 
            bt_login = 1; 
          } 

          if ((bcmd_str == "C3-") && (bcmd_len == 12) && (bpin_str == bt_pincode_str)) { 
            bmsg_ok = 1; 
            String bpin_new_str = bt_rxdata_str.substring(8, 12); 
            bt_pincode_str = bpin_new_str; 
            msg_pincode_str = "-> Command : Change Pincode to " + bpin_new_str; 
            bt_login = 1; 
          } 

          if ((bcmd_str == "C4-") && (bcmd_len == 12) && (bpin_str == bt_pincode_str)) { 
            bmsg_ok = 1; 
            String balarm_str = bt_rxdata_str.substring(8, 12); 
            alarm_1sec = balarm_str.toInt(); 
            if (alarm_1sec < 0) { alarm_1sec = 0; } 
            if (alarm_1sec > 9999) { alarm_1sec = 9999; } 
            balarm_str = String(alarm_1sec); 
            msg_pincode_str = "-> Command : Machine Alarm " + balarm_str + " Sec."; 
            bt_login = 1; 
          } 

          if ((bcmd_str == "C5-") && (bcmd_len == 7) && (bpin_str == bt_pincode_str)) { 
            bmsg_ok = 1; 
            msg_pincode_str = "-> Command : Test interrupt stop start switch"; 
            bt_login = 1; 
            machine_stop_start_req = 1; 
          } 
        } 

        if (bmsg_ok == 0) { 
          msg_pincode_str = "-> Invalid message!!!"; 
        } 

        bt_rxdata_str = ""; 
      } 
      break; 

  } 

//----------------------------------------------------------------------------- 
  if (machine_run == 0) { 

  } else { 

  } 

  if (alarm_1sec == 0) { 
    digitalWrite(PIN_ALARM, HIGH); 
  } else { 
    digitalWrite(PIN_ALARM, LOW); 
  } 

//----------------------------------------------------------------------------- 
  if (tmr_cnt == 0) { 
    bool bbusy = false; 

    sprintf(buff_str, " St-%02d Sw-%02d Cnt-%d Li-%d Mc-%d A-%04d | "
      , state_ix_mon, machine_req_wait_100ms, bt_connected, bt_login, machine_run, alarm_1sec); 

    if ((bt_connected == true) && (bt_login == 1)) { 
      char btxdata_str[200] = {0}; 
      sprintf(btxdata_str, "Mc-%d A-%04d", machine_run, alarm_1sec); 
      pTxCharacteristic->setValue(btxdata_str); 
      pTxCharacteristic->notify(); 
    } 

    Serial.print(buff_str); 

    if ((bbusy == false) && (bt_rxvalue_str.length() > 0)) { 
      bbusy = true; 
      Serial.print("<- "); 
      for (int i = 0; i < bt_rxvalue_str.length(); i++) { 
        Serial.print(bt_rxvalue_str[i]); 
      } 
      bt_rxvalue_str = "";
    } 

    if ((bbusy == false) && (msg_connected_str.length() > 0)) { 
      bbusy = true; 
      Serial.print(msg_connected_str); 
      msg_connected_str = "";
    }

    if ((bbusy == false) && (msg_pincode_str.length() > 0)) { 
      bbusy = true; 
      Serial.print(msg_pincode_str); 
      msg_pincode_str = ""; 
    } 

    Serial.println(); 
  } 

//----------------------------------------------------------------------------- 
  while ((micros() - t_old) < 100000L); t_old = micros(); 
  tmr_cnt++; if (tmr_cnt >= 10) { tmr_cnt = 0; } 

  if (wait_100ms > 0) { wait_100ms--; } 

  if (tmr_cnt == 0) { 
    if (alarm_1sec > 0) { alarm_1sec--; } 
  }

  if (machine_req_wait_100ms > 0) { machine_req_wait_100ms--; } 
} 

