#ifndef _LCD_SPECIAL_H
#define _LCD_SPECIAL_H
#include <Arduino.h>
#include <stdint.h>

// Creat a set of new characters
const uint8_t charBitmap[][8] = {
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
     B11111},
    {
        // idx 4 - batt 25%
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
        // idx 5 - batt 10%
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
    }};
#endif // _LCD_SPECIAL_H