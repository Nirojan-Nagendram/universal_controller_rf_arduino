#include <Arduino.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// Arduino pins set up 
// Radio | joysticks | encoders
RF24 radio(9, 10);  // CE, CSN
const byte address[6] = "00001";

namespace logger{               //namespace > class, for enabled variable access
  bool enabled = true;   // this is the switch — a variable that lives in memory
  void joystick (const char* label, int x, int y,bool sw){            // const char* for simplicity
    if (!enabled) return; // if off, then end logger without printing
    Serial.print(label);
    Serial.print(" = x: "); Serial.print(x);
    Serial.print(" | y: "); Serial.print(y);
    Serial.print(" | switch: "); Serial.println(sw);
  }
  void encoder(const char* label, int direction, bool sw){
    if (!enabled) return;                           // if off, then end logger without printing
    Serial.print(label);
    Serial.print(" = Encoder direction: "); Serial.print(direction);
    Serial.print(" | switch: "); Serial.println(sw);
  }
}

const int jx1_pin = A2, jy1_pin = A1, j1_switch_pin = A0; // (const as it never changes)
const int jx2_pin = A6, jy2_pin = A7, j2_switch_pin = 7;

const int e1_CLKpin = A3, e1_DTpin = A4, e1_switchpin = A5;
const int e2_CLKpin = 2, e2_DTpin = 3, e2_switchpin = 4;

// VARIABLES:
// joysticks | encoders
int jx1_value = 0, jy1_value = 0;
bool j1_switch = false;
int prev_jx1 = 0, prev_jy1 = 0;
bool prev_j1_sw = false;

int jx2_value = 0, jy2_value = 0;
bool j2_switch = false;
int prev_jx2 = 0, prev_jy2 = 0;
bool prev_j2_sw = false; 

int e1_direction = 0, lastState_eCLK1 = 0;
int prev_e1_dir = 0, prev_e2_dir = 0;
int e2_direction = 0, lastState_eCLK2 = 0;
bool e1_switch = false, e2_switch = false;
bool prev_e1_sw = false, prev_e2_sw = false;

struct ControllerData {
    int jx1, jy1, jx2, jy2;
    bool j1_sw, j2_sw;
    int e1, e2;
    bool e1_sw, e2_sw;
};

void setup() {
  Serial.begin(9600);
  logger::enabled = true;
  // Radio
  bool radio_check = radio.begin();
  if (!radio_check){
    Serial.println("radio hardware failed");
    while(true){} // halts program if no radio.
    }
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW);
  radio.stopListening();  // We're transmitting

  // joystick
  pinMode(j1_switch_pin, INPUT_PULLUP);
  pinMode(j2_switch_pin, INPUT_PULLUP);
  // encoder
  pinMode(e1_CLKpin, INPUT), pinMode(e1_DTpin, INPUT);
  pinMode(e1_switchpin, INPUT_PULLUP);
  lastState_eCLK1 = digitalRead(e1_CLKpin);

  pinMode(e2_CLKpin, INPUT), pinMode(e2_DTpin, INPUT);
  pinMode(e2_switchpin, INPUT_PULLUP);
  lastState_eCLK2 = digitalRead(e2_CLKpin);
}

int deadzone(int value){
  if (-10<value && value<10) value=0;
  return value;
}
int readJoystick(int joy_pin, bool invert){
  int read_axis = deadzone(map(analogRead(joy_pin),0,1023,invert ? 100:-100,invert ? -100:100));
  return read_axis;
}
int readEncoder(int CLK_pin, int CLK_last, int DT_pin, int direction){
  int CLK_current = digitalRead(CLK_pin);
  if (CLK_current != CLK_last){          //if any change then...
    if (digitalRead(DT_pin)==HIGH)                  //if DT HIGH while CLK is changing, clockwise. and inverse
      direction++;
    else if (digitalRead(DT_pin)==LOW)
      direction--;
  return  direction;
}}

int readPress(int pin){
  int read_press = digitalRead(pin);
  return read_press;
}

void loop() {
  // read the value from the sensor:
  jx1_value =readJoystick(jx1_pin,true);
  jy1_value =readJoystick(jy1_pin,false);
  j1_switch =readPress(j1_switch_pin);
  if(jx1_value!=prev_jx1 or jy1_value!=prev_jy1 or j1_switch!=prev_j1_sw){
    logger::joystick("Joystick 1",jx1_value,jy1_value,j1_switch);
    prev_jx1 = jx1_value, prev_jy1 = jy1_value, prev_j1_sw = j1_switch;
  }

  jx2_value =readJoystick(jx2_pin,false);
  jy2_value =readJoystick(jy2_pin,true);
  j2_switch =readPress(j2_switch_pin);
  if(jx2_value!=prev_jx2 or jy2_value!=prev_jy2 or j2_switch!=prev_j2_sw){
    logger::joystick("Joystick 2",jx2_value,jy2_value,j2_switch);
    prev_jx2 = jx2_value, prev_jy2 = jy2_value, prev_j2_sw = j2_switch;
  }

  e1_direction = readEncoder(e1_CLKpin, lastState_eCLK1, e1_DTpin, e1_direction);
  e1_switch = readPress(e1_switchpin);
  lastState_eCLK1 = digitalRead(e1_CLKpin);
  if(e1_direction!=prev_e1_dir or e1_switch!=prev_e1_sw){
    logger::encoder("encoder 1: ", e1_direction, e1_switch);
    prev_e1_dir = e1_direction, prev_e1_sw = e1_switch;
  }

  e2_direction = readEncoder(e2_CLKpin, lastState_eCLK2, e2_DTpin, e2_direction);
  e2_switch = readPress(e2_switchpin);
  lastState_eCLK2 = digitalRead(e2_CLKpin);
    if(e2_direction!=prev_e2_dir or e2_switch!=prev_e2_sw){
    logger::encoder("encoder 2: ", e2_direction, e2_switch);
    prev_e2_dir = e2_direction, prev_e2_sw = e2_switch;
  }
  ControllerData data = {jx1_value, jy1_value, jx2_value, jy2_value, 
                        j1_switch, j2_switch, e1_direction, e2_direction, e1_switch, e2_switch};
  radio.write(&data, sizeof(data));  // Would read value of data, rather than data itself. 
}
