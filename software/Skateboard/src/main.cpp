#include <Arduino.h>
#include <SoftwareSerial.h>
#include <VescUart.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <stdio.h>
#include <Adafruit_NeoPixel.h>

#define WS_PIN        10 // On Trinket or Gemma, suggest changing this to 1

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 7 // Popular NeoPixel ring size

SoftwareSerial vescSerial(11, 12); // RX, TX
VescUart UART;
const int PPM_PULSE_IN_TIME = 40000; // microseconds
const int STEER_PPM_PIN = 4;
const int THROTTLE_PPM_PIN = 5;
const int BUTTON_PPM_PIN = 6;

int readLevel(int pin)
{
    int pwm_value = pulseIn(pin, HIGH, PPM_PULSE_IN_TIME);
    return pwm_value;
}

// Creat a set of new characters
const uint8_t charBitmap[][8] ={
    {
        // idx 0 - batt charge
        B01110,
        B11111,
        B10101,
        B10001,
        B11011,
        B11011,
        B11111,
        B11111,
    },
    {
        // idx 1 - batt 100%
        B01110,
        B11111,
        B11111,
        B11111,
        B11111,
        B11111,
        B11111,
        B11111,
    },
    {
        // idx 2 - batt 75%
        B01110,
        B10001,
        B10001,
        B11111,
        B11111,
        B11111,
        B11111,
        B11111,
    },
    {// idx 3 - batt 50%
        B01110,
        B10001,
        B10001,
        B10001,
        B11111,
        B11111,
        B11111,
        B11111 },
    {
        // idx 4 - batt 10%
        B01110,
        B10001,
        B10001,
        B10001,
        B10001,
        B11111,
        B11111,
        B11111,
    },
    {
        // idx 5 - batt 0%
        B01110,
        B10001,
        B10001,
        B10001,
        B10001,
        B10001,
        B11111,
        B11111,
    },
    {
        // idx 6 - batt 0%
        B01110,
        B10001,
        B10001,
        B10001,
        B10001,
        B10001,
        B10001,
        B11111,
    } };

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

void setupLcd()
{
    int charBitmapSize = (sizeof(charBitmap) / sizeof(charBitmap[0]));

    lcd.begin(16, 2); // initialize the lcd

    for (int i = 0; i < charBitmapSize; i++)
    {
        lcd.createChar(i, (uint8_t *)charBitmap[i]);
    }
}

Adafruit_NeoPixel pixels(NUMPIXELS, WS_PIN, NEO_GRB + NEO_KHZ800);

void setup()
{
    setupLcd();

    lcd.home(); // go home
    lcd.print("Eluwina ziomeczku");
    lcd.setCursor(0, 1); // go to the next line
    lcd.print("Deseczka jest w pytke");
    delay(1000);

    // put your setup code here, to run once:
    Serial.begin(115200);
    vescSerial.begin(9600);
    // vescSerial.begin(57600);

    while (!Serial)
    {
        ; // wait for serial port to connect. Needed for Native USB only
    }

    UART.setSerialPort(&vescSerial);
    // UART.setDebugPort(&Serial);
    pinMode(4, INPUT);
    pinMode(5, INPUT);
    pinMode(6, INPUT);

    pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
}

float voltage_to_percentage(float voltage)
// TODO make better
{
    float x = voltage / 2.0f;
    float factor = x;

    float y = 959444.6;
    y += -245584.2 * factor;
    factor *= x;

    y += 23534.65 * factor;
    factor *= x;

    y += -1000.876 * factor;
    factor *= x;

    y += 15.94083 * factor;

    Serial.println(y);
    if (y > 100.f)
        return 100.f;
    if (y < 0.f)
        return 0.f;

    return y;
}

void renderLcd(bool vesc_connected, float voltage, long long tacho, float amp_hours)
{
    float tacho_in_km = tacho * 0.0000037399912542735634;
    int km_major = (int)tacho_in_km;
    int km_minor = (int)(tacho_in_km * 10) % 10;

    int voltage_major = (int)voltage;
    int voltage_minor = (int)(voltage * 10) % 10;

    int amp_hours_major = (int)amp_hours;
    int amp_hours_minor = (int)(amp_hours * 10) % 10;

    float voltage_percentage = voltage_to_percentage(voltage);
    Serial.println(voltage_percentage);

    char battery_idx = 1;
    if (voltage_percentage < 80)
    {
        battery_idx = 2;
    }
    if (voltage_percentage < 60)
    {
        battery_idx = 3;
    }
    if (voltage_percentage < 40)
    {
        battery_idx = 4;
    }
    if (voltage_percentage < 20)
    {
        battery_idx = 5;
    }
    if (voltage_percentage < 10)
    {
        battery_idx = 6;
    }

    // XDESKAXNCX99.9km
    // X99.9AhX16.4VXBX

    lcd.home();
    char buff[32];
    snprintf(buff, 32, "XDESKA %s %2d.%dkm", (vesc_connected ? " C" : "NC"), km_major, km_minor);
    lcd.print(buff);

    lcd.setCursor(0, 1); // go to the next line
    snprintf(buff, 32, " %2d.%dAh %2d.%dV %c ", voltage_major, voltage_minor, amp_hours_major, amp_hours_minor, battery_idx);
    lcd.print(buff);
    Serial.println("boot");

}

void loop()
{
    Serial.println("xd2");
    Serial.print(readLevel(4));
    Serial.print(" ");
    Serial.print(readLevel(5));
    Serial.print(" ");
    Serial.println(readLevel(6));

    pixels.clear();

    for (int i=0; i<NUMPIXELS; i++) { // For each pixel...
        pixels.setPixelColor(i, pixels.Color(50, 0, 0));
    }
    // zielony 11 - vesc TX - arduino RX
        // pomarancz 12 - vesc RX = arduino TX
    pixels.show();   // Send the updated pixel colors to the hardware.


    if (UART.getVescValues())
    {

        Serial.print("gotdata ");
        // Serial.print("avgMotorCurrent ");
        // Serial.println(UART.data.avgMotorCurrent);
        // Serial.print("avgInputCurrent ");
        // Serial.println(UART.data.avgInputCurrent);
        // Serial.print("dutyCycleNow ");
        // Serial.println(UART.data.dutyCycleNow);
        // Serial.print("RPM ");
        // Serial.println(UART.data.rpm);
        // Serial.print("V ");
        // Serial.println(UART.data.inpVoltage);
        // Serial.print("Ah ");
        // Serial.println(UART.data.ampHours);
        // Serial.print("Ah chg ");
        // Serial.println(UART.data.ampHoursCharged);
        // Serial.print("Tacho ");
        // Serial.println(UART.data.tachometer);
        // Serial.print("Tacho abs ");
        // Serial.println(UART.data.tachometerAbs);
        // Serial.println('\n');
    }
    else
    {
        Serial.println("Failed to get data!");
    }
    // renderLcd(true, 32.123, 142597 * 55, 12.9123);
    // delay(2000);
    // renderLcd(true, 16.6 * 2, 142597 * 55, 12.9123);
    // delay(2000);
    // renderLcd(true, 15.8 * 2, 142597 * 55, 12.9123);
    // delay(2000);
    // renderLcd(true, 15.18 * 2, 142597 * 55, 12.9123);
    // delay(2000);
    // renderLcd(true, 14.83 * 2, 142597 * 55, 12.9123);
    // delay(2000);
    // renderLcd(true, 13.53 * 2, 142597 * 55, 12.9123);
    // delay(2000);
    // renderLcd(true, 2.123, 142597, 2.9123);
    // delay(2000);
    // renderLcd(false, 0, 0, 0);
    // delay(2000);

    // Do a little animation by writing to the same location
    // for (int j = 0; j < 16; j++)
    // {
    //   lcd.print(char(j % 7));
    // }
    // lcd.setCursor(0, 1);
    // delay(200);
}

// void loop()
// {
//   /** Call the function getVescValues() to acquire data from VESC */

//   delay(1000);
//   // put your main code here, to run repeatedly:
// }
// 0123456789ABCDEF
// XDESKANCX16.4VXB
// XXX99.9Ah

// 0123456789ABCDEF
// XDESKAXNCX99.9km
// X99.9AhX16.4VXBX

// Motor Gear 11
// Wheel Gear 50
// wheel diameter 200x50mm

// 0.6283185307179586m - 1 obrot
// 168 tacho/ 1 obrot

// (tacho / 168) * 0.6 ... = meters