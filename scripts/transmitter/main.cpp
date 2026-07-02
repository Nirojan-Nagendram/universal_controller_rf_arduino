#include <Arduino.h>

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(9, 10);  // CE, CSN
const byte address[6] = "00001";

void setup() {
  Serial.begin(9600);
  bool radio_check = radio.begin();
  if (!radio_check){
    Serial.println("radio hardware failed");
    while(true){} // halts program if no radio.
    }
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW);
  radio.stopListening();  // We're transmitting
}

void loop() {
  int throttle = 5;
  radio.write(&throttle, sizeof(throttle));  // Would read value of throttle, rather than throttle 
  Serial.print("Sending throttle: "); Serial.print(throttle);

  delay(50);  // Send every 50ms
}
