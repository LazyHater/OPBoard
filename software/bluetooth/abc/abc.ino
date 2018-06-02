#include <SoftwareSerial.h>

#include <Wire.h>
#include "Nunchuk.h"
#define I2C_NUNCHUK_ADDRESS 0x3C

const byte ENGINES_OFF = 128;
SoftwareSerial mySerial(12, 10); // RX, TX


void setup() {
  mySerial.begin(115200);
  
  Wire.begin();
  nunchuk_init();
}

void loop() { // run over and over
  byte val = ENGINES_OFF;
  if (nunchuk_read()) {
    if (nunchuk_buttonZ()) {
      val = nunchuk_joystickY_raw();
    }
  }
  
  byte pos = map(val, 0, 255, 0, 180);
  mySerial.write(pos);      
  delay(10);
}
