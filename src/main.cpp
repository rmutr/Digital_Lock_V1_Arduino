//----------------------------------------------------------------------------- 
// Project          : RmutR Digital Lock V1 
// VSCode Extension : PlatformIO IDE 1.10.0 
// Source           : https://github.com/rmutr/Digital_Lock_V1.git 
// Board            : Node32s (Gravitech Node32Lite LamLoei) 
// Additional URLs  : https://dl.espressif.com/dl/package_esp32_index.json 
// LED_BUILTIN      : Pin 2 


//----------------------------------------------------------------------------- 
#include <Arduino.h>

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
void setup() { 
  pinMode(PIN_PROCESS,   OUTPUT); 
  pinMode(PIN_BTN_START, INPUT_PULLUP); 

  digitalWrite(PIN_PROCESS,   HIGH); 
  digitalWrite(PIN_BTN_START, HIGH); 

  sprintf(buff_str, "Digital Lock V1"); 

  Serial.begin(115200); 
  Serial.println(buff_str); 

//----------------------------------------------------------------------------- 
  t_old = 0; 
  tmr_cnt = 0; 
  error = 0; 
  state_ix = 0; 
  state_ix_mon = 0; 
  req_machine_start = 0; 

} 

void loop() { 

} 
 
 