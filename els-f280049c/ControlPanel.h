// Clough42 Electronic Leadscrew
// https://github.com/clough42/electronic-leadscrew
//
// MIT License
//
// Copyright (c) 2019 James Clough
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#ifndef __CONTROL_PANEL_H
#define __CONTROL_PANEL_H

#include "Panel.h"
#include "SPIBus.h"


class ControlPanel : public Panel
{
private:
    // Common SPI Bus
    SPIBus *spiBus;

    // Current RPM value; 4 decimal digits
    Uint16 rpm;

    // Current displayed setting value, 4 digits
    const Uint16 *value;

    // Current LED states
    LED_REG leds;

    // current key states
    KEY_REG keys;

    // number of times current key state has been seen
    KEY_REG stableKeys;
    Uint16 stableCount;

    // current override message, or NULL if none
    const Uint16 *message;

    // brightness, levels 1-8, 0=off
    Uint16 brightness;

    // Derived state, calculated internally
    Uint16 sevenSegmentData[8];

    // dummy register, for SPI
    Uint16 dummy;

    void decomposeRPM(void);
    void decomposeValue(void);
    KEY_REG readKeys(void);
    Uint16 lcd_char(Uint16 x);
    void sendByte(Uint16 data);
    Uint16 receiveByte(void);
    void sendData(void);
    Uint16 reverse_byte(Uint16 x);
    void initSpi();
    void configureSpiBus(void);
    bool isValidKeyState(KEY_REG);
    bool isStable(KEY_REG);

public:
    ControlPanel(SPIBus *spiBus);

    // initialize the hardware for operation
    void initHardware(void);

    // poll the keys and return a mask
    KEY_REG getKeys(void);

    // set the RPM value to display
    void setRPM(Uint16 rpm);

    // set the value to display
    void setValue(const Uint16 *value);

    // set the LED states
    void setLEDs(LED_REG leds);

    // set a message that overrides the display, 8 characters required
    void setMessage(const Uint16 *message, const char *text, MESSAGE_SEVERITY severity);

    // set a brightness value, 0 (off) to 8 (max)
    void setBrightness(Uint16 brightness);

    // refresh the hardware display
    void refresh(void);
};


inline void ControlPanel :: setRPM(Uint16 rpm)
{
    this->rpm = rpm;
}

inline void ControlPanel :: setValue(const Uint16 *value)
{
    this->value = value;
}

inline void ControlPanel :: setLEDs(LED_REG leds)
{
    this->leds = leds;
}


#endif // __CONTROL_PANEL_H
