

#include <Servo.h>


const byte ENGINES_OFF = 90;
const unsigned long TIMEOUT_VALUE = 500; // ms


Servo myservo;  // create servo object to control a servo
void setup() {
  // set the data rate for the SoftwareSerial port
  Serial.begin(115200);
 // mySerial.println("Hello, world?");
  
  
  myservo.attach(8);
  myservo.write(ENGINES_OFF);
}
byte pos = ENGINES_OFF;
unsigned long timeout_counter = 0;
void loop() { // run over and over
  if (Serial.available()) {
    pos = Serial.read();
    timeout_counter = millis();
  }  

  if ( millis() - timeout_counter > TIMEOUT_VALUE ) {
    pos = ENGINES_OFF; 
  } 

  myservo.write(pos);
}
