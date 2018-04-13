/*
  Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  version 2 as published by the Free Software Foundation.
*/

//#define SKATEBOARD
#define REMOTE

#if defined SKATEBOARD && defined REMOTE
#error "SKATEBOARD and REMOTE cannot be declared at the same time!"
#endif

const float ENGINES_OFF = 107.0f;

#ifdef REMOTE
// nunchuck stuff
#include <Wire.h>
#include "Nunchuk.h"
#define I2C_NUNCHUK_ADDRESS 0x3C

// oled stuff
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"
#define I2C_OLED_ADDRESS 0x3C
SSD1306AsciiAvrI2c oled;

// others
float pos = ENGINES_OFF;

// experimental
#define RESISTOR_CONSTANT (5.5062509316f)
#define V_REF (1.1f)

#elif defined SKATEBOARD

#include <Servo.h>
#include <Adafruit_NeoPixel.h>
Servo myservo;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(13, 6, NEO_GRB + NEO_KHZ800);
unsigned long comm_timeout_counter = 0;
unsigned long num_timeout = 0;
bool comm_timeout = false;
const unsigned long TIMEOUT_VALUE = 500; // ms
#endif


// common nrf stuff
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio(9, 10);

const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

void setup(void) {
  Serial.begin(115200);

  radio.begin();

  radio.setRetries(15, 15);



#ifdef REMOTE
  pinMode(A0, INPUT);
  analogReference(INTERNAL); // internal 1.1V

  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1, pipes[0]);

  Wire.begin();
  nunchuk_init();

  oled.begin(&Adafruit128x64, I2C_OLED_ADDRESS);
  oled.setFont(Adafruit5x7);

  // magic rotates screen 180 degrees
  oled.ssd1306WriteCmd(0xa0);
  oled.ssd1306WriteCmd(0xc0);

  oled.clear();

#elif defined SKATEBOARD

  myservo.attach(7);
  myservo.write(ENGINES_OFF);

  pixels.begin();

  for (int i = 0; i < 6; i++) {
    pixels.setPixelColor(i, pixels.Color(255, 0, 0));
  }

  for (int i = 7; i < 12; i++) {
    pixels.setPixelColor(i, pixels.Color(255, 255, 255));
  }

  pixels.setPixelColor(12, pixels.Color(255, 141, 2));
  pixels.setPixelColor(6, pixels.Color(255, 141, 2));
  pixels.show();

  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1, pipes[1]);
#endif

  radio.startListening();
}

// application specific functions
#ifdef REMOTE

void printData(float y_val, int pos, float v) {
  char buff[260] = {0};

  char pos_str[7] = {0};
  dtostrf(pos, 5, 2, pos_str);

  char y_val_str[7] = {0};
  dtostrf(y_val, 5, 2, y_val_str);

  char v_str[7] = {0};
  dtostrf(v, 3, 2, v_str);

  sprintf(buff, "Pos: %s\n"
          "Y val: %s\n"
          "Y val raw: %i\n"
          "Z: %s\n"
          "C: %s\n"
          "Voltage: %sV\n",
          pos_str,
          y_val_str,
          ((int)nunchuk_joystickY_raw()),
          (nunchuk_buttonZ() ? "pressed" : "not pressed"),
          (nunchuk_buttonC() ? "pressed" : "not pressed"),
          v_str);

  oled.home();
  oled.print(buff);
}

float get_vcc() {
  return (analogRead(A0) / 1023.0f) * RESISTOR_CONSTANT * V_REF;
}



double pid_current = ENGINES_OFF;
const double P_const = 0.03f;
double getPidResponse(double target) {
  // P part
  double error = target - pid_current;

  pid_current += error * P_const;
  return pid_current;
}

float calc_accel() {
  double y_val = 0.0f;
  if (nunchuk_buttonZ()) { // pid
    y_val = (double) nunchuk_joystickY_raw();
    if (y_val >=  128.0f) {
      y_val = map(y_val, 128.0f, 255.0f, ENGINES_OFF, 180.0f);
    } else {
      y_val = map(y_val, 128.0f, 0.0f, ENGINES_OFF, 0.0f);
    }
    
    pos = getPidResponse( y_val );
    pos = constrain(pos, 0.0f, 180.0f);

  } else if (nunchuk_buttonC()) { // ord

    y_val = (double) (nunchuk_joystickY_raw() - 128); // scale to -255.0f to 255.0f
    y_val /= 127.0f; // scale to -1.0 to 1.0f
    y_val *= 3.0f;
    pos += y_val;
    pos = constrain(pos, 0.0f, 180.0f);

  } else {
    pos = ENGINES_OFF;
    pid_current = pos;
  }

  printData(y_val, pos, get_vcc());
}

#elif defined SKATEBOARD

#endif


void loop(void) {
#ifdef REMOTE
  unsigned long val;
  if (nunchuk_read()) {
    val = nunchuk_joystickY_raw();
  } else {
    Serial.println("Failed to read from nunchuk");
    val = 128;
  }
  // First, stop listening so we can talk.
  radio.stopListening();

  calc_accel();

  // Take the time, and send it.  This will block until complete
  uint8_t pos_b = (uint8_t) pos;
  Serial.print("Now sending "); Serial.print(val); Serial.print("...");
  bool ok = radio.write( &pos_b, sizeof(uint8_t) );

  if (ok)
    Serial.print("ok...");
  else
    Serial.println("failed.");

  // Now, continue listening
  radio.startListening();

  // Wait here until we get a response, or timeout (250ms)
  unsigned long started_waiting_at = millis();
  bool timeout = false;
  while ( ! radio.available() && ! timeout )
    if (millis() - started_waiting_at > 50 )
      timeout = true;

  // Describe the results
  if ( timeout )
  {
    Serial.println("Failed, response timed out.");
    oled.print("Response: ------");
  }
  else
  {
    // Grab the response, compare, and send to debugging spew
    unsigned long got_time;
    radio.read( &got_time, sizeof(unsigned long) );

    Serial.print("Got response "); Serial.println(got_time);
    oled.print("Response: "); oled.print(got_time);
  }

#elif defined SKATEBOARD

  if ( millis() - comm_timeout_counter > TIMEOUT_VALUE ) {
    comm_timeout = true;
  } else {
    comm_timeout = false;
  }

  if (comm_timeout) {
    myservo.write(ENGINES_OFF);
    Serial.println("TIMEOUT");
    num_timeout++;
  }

  // if there is data ready
  if ( radio.available() )
  {
    // Dump the payloads until we've gotten everything
    uint8_t got_val;
    bool done = false;
    while (radio.available())
    {
      comm_timeout_counter = millis();

      // Fetch the payload, and see if this was the last one.
      radio.read( &got_val, sizeof(uint8_t));

      myservo.write(got_val);

      // Spew it
      Serial.print("Got payload "); Serial.print(got_val); Serial.print("...");

      // Delay just a little bit to let the other unit
      // make the transition to receiver
      delay(20);
    }

    // First, stop listening so we can talk
    radio.stopListening();
    unsigned long resp = millis();
    // Send the final one back.
    radio.write( &num_timeout, sizeof(unsigned long) );
    Serial.println("Sent response.");

    // Now, resume listening so we catch the next packets.
    radio.startListening();
  }
#endif
}
// vim:cin:ai:sts=2 sw=2 ft=cpp
