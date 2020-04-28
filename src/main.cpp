//----------------------------------------------------------------------------- 
// Project          : RmutR Digital Lock V1 
// VSCode Extension : PlatformIO IDE 1.10.0 
// Source           : https://github.com/rmutr/Digital_Lock_V1.git 
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

#define PNP_OFF              0 
#define PNP_ON               1 
#define NPN_OFF              1 
#define NPN_ON               0 

char buff_str[200] = {0}; 
unsigned long t_old = 0; 
int tmr_cnt = 0; 
int error = 0; 

int state_ix = 0; 
int state_ix_mon = 0; 

volatile int req_machine_start = 0; 

int bluetooth_linked = 0; 

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
bool deviceConnected = false; 
bool oldDeviceConnected = false; 
uint8_t txvalue = 0; 

// See the following for generating UUIDs: 
// https://www.uuidgenerator.net/ 

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" 
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" 
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" 

//----------------------------------------------------------------------------- 
void Interrupt_Service_Btn_Start(); 

void Interrupt_Service_Btn_Start() { req_machine_start = 1; } 

class MyServerCallbacks: public BLEServerCallbacks { 
  void onConnect(BLEServer* pServer) { 
    deviceConnected = true; 
    //BLEDevice::startAdvertising(); 
  }; 

  void onDisconnect(BLEServer* pServer) { 
    deviceConnected = false; 
  } 
}; 

class MyCallbacks: public BLECharacteristicCallbacks { 
  void onWrite(BLECharacteristic *pCharacteristic) { 
    std::string rxvalue = pCharacteristic->getValue(); 

    if (rxvalue.length() > 0) { 

      Serial.print("Received Value: "); 
      for (int i = 0; i < rxvalue.length(); i++) { 
        Serial.print(rxvalue[i]); 
      } 
      Serial.println(); 

    } 
  } 
}; 


//----------------------------------------------------------------------------- 
void setup() { 
  pinMode(PIN_PROCESS,   OUTPUT); 
  pinMode(PIN_BTN_START, INPUT_PULLUP); 

  digitalWrite(PIN_PROCESS,   HIGH); 
  digitalWrite(PIN_BTN_START, HIGH); 

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
  error = 0; 
  state_ix = 0; 
  state_ix_mon = 0; 
  req_machine_start = 0; 
  bluetooth_linked = 0; 

} 

void loop() { 
  //connected 
  bluetooth_linked = 0; 
  if (deviceConnected) { 
    bluetooth_linked = 1; 
    char bstr[50] = {0}; sprintf(bstr, "%02d", txvalue); 
    pTxCharacteristic->setValue(bstr); 
    pTxCharacteristic->notify(); 
    txvalue++; if (txvalue >= 100) { txvalue = 0; } 
    delay(1000); 
  } 

  // disconnecting 
  if (!deviceConnected && oldDeviceConnected) { 
    delay(500); // give the bluetooth stack the chance to get things ready 
    pServer->startAdvertising(); // restart advertising 
    Serial.println("start advertising"); 
    oldDeviceConnected = deviceConnected; 
  } 

  // connecting 
  if (deviceConnected && !oldDeviceConnected) { 
    oldDeviceConnected = deviceConnected; 
  } 

//----------------------------------------------------------------------------- 
  sprintf(buff_str, " S-%02d L-%d SW-%d | ", state_ix_mon, bluetooth_linked, req_machine_start); 
  Serial.println(buff_str); 

//----------------------------------------------------------------------------- 
  while ((micros() - t_old) < 1000000L); t_old = micros(); 
  tmr_cnt++; if (tmr_cnt >= 10) { tmr_cnt = 0; } 

} 


