#include <Arduino.h>
#include <SoftwareSerial.h>
#include <VescUart.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <stdio.h>
#include <Adafruit_NeoPixel.h>

#include "lcd_special.h"

const int WS_PIN = 10; // On Trinket or Gemma, suggest changing this to 1
const int NUMPIXELS = 6;

const int PPM_PULSE_IN_TIME = 32000; // microseconds
const int STEER_PPM_PIN = 4;
const int THROTTLE_PPM_PIN = 5;
const int BUTTON_PPM_PIN = 6;

const int VESC_TX_PIN = 11;
const int VESC_RX_PIN = 12;

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

SoftwareSerial vescSerial(VESC_TX_PIN, VESC_RX_PIN); // RX, TX
VescUart UART;

int readPulseLen(int pin)
{
    int pulse_len = pulseIn(pin, HIGH, PPM_PULSE_IN_TIME);
    return pulse_len;
}

void setupLcd()
{
    int charBitmapSize = (sizeof(charBitmap) / sizeof(charBitmap[0]));

    lcd.begin(16, 2); // initialize the lcd

    for (int i = 0; i < charBitmapSize; i++)
    {
        lcd.createChar(i, (uint8_t *)charBitmap[i]);
    }
}

Adafruit_NeoPixel neopixels(NUMPIXELS, WS_PIN, NEO_GRB + NEO_KHZ800);

class WsDriver
{
private:
    const uint32_t stop_color = Adafruit_NeoPixel::Color(30, 0, 0);
    const uint32_t break_color = Adafruit_NeoPixel::Color(255, 0, 0);
    const uint32_t blink_color = Adafruit_NeoPixel::Color(255, 255, 0);
    Adafruit_NeoPixel &pixels;
    bool breaking = false;
    bool disabled = true;
    enum BLINKER
    {
        NO_BLINKER = 0,
        LEFT_BLINKER = 1,
        RIGHT_BLINKER = 2,
    } blinker_state = NO_BLINKER;
    unsigned long animation_start_time = 0;

    unsigned long blink_interval = 750;
    unsigned long blink_duration = 10000;

public:
    WsDriver(Adafruit_NeoPixel &pixels) : pixels(pixels)
    {
    }

    void init()
    {
        this->pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
    }

    void step()
    {
        this->pixels.clear();
        if (this->disabled)
        {
            this->pixels.show();
            return;
        }

        this->pixels.fill(this->breaking ? this->break_color : this->stop_color);

        unsigned long animation_left_time = millis() - this->animation_start_time;
        switch (this->blinker_state)
        {
        case NO_BLINKER:
        {
            break;
        }
        case LEFT_BLINKER:
        case RIGHT_BLINKER:
        {
            if (animation_left_time > this->blink_duration)
            {
                this->blinker_state = NO_BLINKER;
                break;
            }

            if (animation_left_time % this->blink_interval < this->blink_interval / 2)
            {
                this->pixels.setPixelColor((this->blinker_state == LEFT_BLINKER) ? NUMPIXELS - 1 : 0, blink_color);
            }

            break;
        }
        }
        this->pixels.show(); // Send the updated pixel colors to the hardware.
    }

    void setLeftBlinker()
    {
        this->animation_start_time = millis();
        this->blinker_state = LEFT_BLINKER;
    }
    void setRightBlinker()
    {
        this->animation_start_time = millis();
        this->blinker_state = RIGHT_BLINKER;
    }

    void setBreaking(bool breaking)
    {
        this->breaking = breaking;
    }

    void disable()
    {
        this->disabled = true;
        this->breaking = false;
        this->blinker_state = NO_BLINKER;
    }

    void enable()
    {
        this->disabled = false;
    }
};

WsDriver wsdriver(neopixels);

void setup()
{
    setupLcd();

    lcd.home(); // go home
    lcd.print("    Eluwina");
    lcd.setCursor(0, 1); // go to the next line
    lcd.print("xD-Board is RDY");
    delay(1000);

    // put your setup code here, to run once:
    Serial.begin(115200);
    vescSerial.begin(9600);

    while (!Serial)
    {
        ; // wait for serial port to connect. Needed for Native USB only
    }

    UART.setSerialPort(&vescSerial);
    // UART.setDebugPort(&Serial);
    pinMode(STEER_PPM_PIN, INPUT_PULLUP);
    pinMode(THROTTLE_PPM_PIN, INPUT_PULLUP);
    pinMode(BUTTON_PPM_PIN, INPUT_PULLUP);

    wsdriver.init();

    Serial.println("setup completed");
}

float voltageToPercentage(float voltage)
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

    // Serial.println(y);
    if (y > 100.f)
        return 100.f;
    if (y < 0.f)
        return 0.f;

    return y;
}

void renderVescStats(bool vesc_connected, float voltage, long long tacho, float amp_hours)
{
    float tacho_in_km = tacho * 0.0000037399912542735634;
    int km_major = (int)tacho_in_km;
    int km_minor = (int)(tacho_in_km * 10) % 10;

    int voltage_major = (int)voltage;
    int voltage_minor = (int)(voltage * 10) % 10;

    int amp_hours_major = (int)amp_hours;
    int amp_hours_minor = (int)(amp_hours * 10) % 10;

    float voltage_percentage = voltageToPercentage(voltage);

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

    lcd.clear();
    char buff[17];
    snprintf(buff, sizeof(buff), "XDESKA %s %2d.%dkm", (vesc_connected ? " C" : "NC"), km_major, km_minor);
    lcd.write(buff, 16);

    lcd.setCursor(0, 1); // go to the next line
    snprintf(buff, sizeof(buff), " %2d.%dAh %2d.%dV %c ", amp_hours_major, amp_hours_minor, voltage_major, voltage_minor, battery_idx);

    lcd.write(buff, 16);
}
// 1574 1492 1098
void handleSteer()
{
    uint16_t steer_pulse_len = readPulseLen(STEER_PPM_PIN);
    if (steer_pulse_len == 0)
    {
        return;
    }

    if (steer_pulse_len > 1700)
    {
        wsdriver.setRightBlinker();
    }

    if (steer_pulse_len < 1400)
    {
        wsdriver.setLeftBlinker();
    }
}

void handleThrottle()
{
    uint16_t throttle_pulse_len = readPulseLen(THROTTLE_PPM_PIN);
    if (throttle_pulse_len == 0)
    {
        return;
    }

    if (throttle_pulse_len < 1400)
    {
        wsdriver.setBreaking(true);
    }
    else
    {
        wsdriver.setBreaking(false);
    }
}

void handleButton()
{
    uint16_t button_pulse_len = readPulseLen(BUTTON_PPM_PIN);
    if (button_pulse_len == 0)
    {
        return;
    }

    if (button_pulse_len > 1500)
    {
        wsdriver.disable();
    }
    else
    {
        wsdriver.enable();
    }
}

void loop()
{
    wsdriver.step();

    handleSteer();
    handleThrottle();
    handleButton();
    // Serial.println(UART.data.inpVoltage);

    // Serial.print(STEER_PPM_PIN);
    // Serial.print(" ");
    // Serial.print(readPulseLen(THROTTLE_PPM_PIN));
    // Serial.print(" ");

    bool vesc_read_succeded = UART.getVescValues();

    renderVescStats(vesc_read_succeded, UART.data.inpVoltage, UART.data.tachometerAbs, UART.data.ampHours);

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

// void printVescData()
// {
//     // it resets uC
//     Serial.print("avgMotorCurrent ");
//     Serial.println(UART.data.avgMotorCurrent);
//     Serial.print("avgInputCurrent ");
//     Serial.println(UART.data.avgInputCurrent);
//     Serial.print("dutyCycleNow ");
//     Serial.println(UART.data.dutyCycleNow);
//     Serial.print("RPM ");
//     Serial.println(UART.data.rpm);
//     Serial.print("V ");
//     Serial.println(UART.data.inpVoltage);
//     Serial.print("Ah ");
//     Serial.println(UART.data.ampHours);
//     Serial.print("Ah chg ");
//     Serial.println(UART.data.ampHoursCharged);
//     Serial.print("Tacho ");
//     Serial.println(UART.data.tachometer);
//     Serial.print("Tacho abs ");
//     Serial.println(UART.data.tachometerAbs);
//     Serial.println('\n');
// }

// void testRenderStats()
// {
//     renderVescStats(true, 32.123, 142597 * 55, 12.9123);
//     delay(2000);
//     renderVescStats(true, 16.6 * 2, 142597 * 55, 12.9123);
//     delay(2000);
//     renderVescStats(true, 15.8 * 2, 142597 * 55, 12.9123);
//     delay(2000);
//     renderVescStats(true, 15.18 * 2, 142597 * 55, 12.9123);
//     delay(2000);
//     renderVescStats(true, 14.83 * 2, 142597 * 55, 12.9123);
//     delay(2000);
//     renderVescStats(true, 13.53 * 2, 142597 * 55, 12.9123);
//     delay(2000);
//     renderVescStats(true, 2.123, 142597, 2.9123);
//     delay(2000);
//     renderVescStats(false, 0, 0, 0);
//     delay(2000);
// }
