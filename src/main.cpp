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
void Interrupt_Service_Btn_Start(); 

void Interrupt_Service_Btn_Start() { req_machine_start = 1; } 


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
  t_old = 0; 
  tmr_cnt = 0; 
  error = 0; 
  state_ix = 0; 
  state_ix_mon = 0; 
  req_machine_start = 0; 

} 

void loop() { 

//----------------------------------------------------------------------------- 
  sprintf(buff_str, " S-%02d L-%d SW-%d | ", state_ix_mon, 0, req_machine_start); 
  Serial.println(buff_str); 

//----------------------------------------------------------------------------- 
  while ((micros() - t_old) < 1000000L); t_old = micros(); 
  tmr_cnt++; if (tmr_cnt >= 10) { tmr_cnt = 0; } 

} 

