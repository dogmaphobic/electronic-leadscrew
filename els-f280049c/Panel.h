// Clough42 Electronic Leadscrew
// https://github.com/clough42/electronic-leadscrew
//
// MIT License

#ifndef __PANEL_H
#define __PANEL_H

#include "F28x_Project.h"

#define ZERO    0b1111110000000000 // 0
#define ONE     0b0110000000000000 // 1
#define TWO     0b1101101000000000 // 2
#define THREE   0b1111001000000000 // 3
#define FOUR    0b0110011000000000 // 4
#define FIVE    0b1011011000000000 // 5
#define SIX     0b1011111000000000 // 6
#define SEVEN   0b1110000000000000 // 7
#define EIGHT   0b1111111000000000 // 8
#define NINE    0b1111011000000000 // 9
#define POINT   0b0000000100000000 // .
#define BLANK   0b0000000000000000

#define LETTER_A 0b1110111000000000
#define LETTER_B 0b0011111000000000
#define LETTER_C 0b1001110000000000
#define LETTER_D 0b0111101000000000
#define LETTER_E 0b1001111000000000
#define LETTER_F 0b1000111000000000
#define LETTER_G 0b1011110000000000
#define LETTER_H 0b0110111000000000
#define LETTER_I 0b0000110000000000
#define LETTER_J 0b0111100000000000
#define LETTER_K 0b1010111000000000
#define LETTER_L 0b0001110000000000
#define LETTER_M 0b1010100000000000
#define LETTER_N 0b1110110000000000
#define LETTER_O 0b1111110000000000
#define LETTER_P 0b1100111000000000
#define LETTER_Q 0b1110011000000000
#define LETTER_R 0b1100110000000000
#define LETTER_S 0b1011011000000000
#define LETTER_T 0b0001111000000000
#define LETTER_U 0b0111110000000000
#define LETTER_V 0b0111010000000000
#define LETTER_W 0b0101010000000000
#define LETTER_X 0b0110110000000000
#define LETTER_Y 0b0111011000000000
#define LETTER_Z 0b1101001000000000

#define DASH 0b0000001000000000

#define LED_TPI 1
#define LED_INCH (1<<1)
#define LED_MM (1<<2)
#define LED_THREAD (1<<3)
#define LED_FEED (1<<4)
#define LED_REVERSE (1<<5)
#define LED_FORWARD (1<<6)
#define LED_POWER (1<<7)

struct LED_BITS
{
    Uint16 TPI:1;
    Uint16 INCH:1;
    Uint16 MM:1;
    Uint16 THREAD:1;
    Uint16 FEED:1;
    Uint16 REVERSE:1;
    Uint16 FORWARD:1;
    Uint16 POWER:1;
};

typedef union LED_REG
{
    Uint16 all;
    struct LED_BITS bit;
} LED_REG;

struct KEY_BITS
{
    Uint16 UP:1;
    Uint16 MESSAGE_CLOSE:1;
    Uint16 DOWN:1;
    Uint16 IN_MM:1;
    Uint16 FEED_THREAD:1;
    Uint16 FWD_REV:1;
    Uint16 SET:1;
    Uint16 POWER:1;
    Uint16 CUSTOM_THREAD:1;
    Uint16 WIZARD_SET_SHOULDER:1;
    Uint16 WIZARD_SET_START:1;
    Uint16 WIZARD_MOVE_TO_START:1;
    Uint16 WIZARD_END:1;
};

typedef union KEY_REG
{
    Uint16 all;
    struct KEY_BITS bit;
} KEY_REG;

typedef enum MESSAGE_SEVERITY
{
    MESSAGE_INFO,
    MESSAGE_ALERT
} MESSAGE_SEVERITY;

class Panel
{
public:
    virtual void initHardware(void) = 0;
    virtual KEY_REG getKeys(void) = 0;
    virtual void setRPM(Uint16 rpm) = 0;
    virtual void setValue(const Uint16 *value) = 0;
    virtual void setLEDs(LED_REG leds) = 0;
    virtual void setMessage(const Uint16 *message, const char *text, MESSAGE_SEVERITY severity) = 0;
    virtual void setBrightness(Uint16 brightness) = 0;
    virtual void refresh(void) = 0;
    virtual Uint16 getCustomPitchHundredths(void) { return 100; }
    virtual Uint16 getWizardStarts(void) { return 1; }
    virtual void setWizardStatus(bool atShoulder, bool atStart) { (void)atShoulder; (void)atStart; }
};

#endif // __PANEL_H
