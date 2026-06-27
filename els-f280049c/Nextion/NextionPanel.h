// Clough42 Electronic Leadscrew
// https://github.com/clough42/electronic-leadscrew
//
// MIT License

#ifndef __NEXTION_PANEL_H
#define __NEXTION_PANEL_H

#include "Panel.h"

class NextionPanel : public Panel
{
private:
    Uint16 rpm;
    const Uint16 *value;
    LED_REG leds;
    KEY_REG pendingKeys;
    const Uint16 *message;
    const char *messageText;
    MESSAGE_SEVERITY messageSeverity;
    Uint16 lastTouchDownId;

    Uint16 lastRpmValue;
    char lastFeedText[12];
    char lastFeedLabel[20];
    char lastMessageText[21];
    Uint16 lastLeds;
    Uint16 startupWaitTicks;
    Uint16 startupHoldTicks;
    Uint16 activePage;
    bool popupVisible;
    bool nextionReady;
    bool nextionStartupSeen;

    Uint32 uiEncoderPosition;
    long uiEncoderDelta;
    Uint16 uiButtonStable;
    Uint16 uiButtonLastSample;
    Uint16 uiButtonDebounce;
    bool uiButtonPressed;

    Uint16 customPitchHundredths;
    Uint16 wizardStarts;
    Uint16 wizardStep;
    Uint16 wizardRunState;
    Uint16 wizardCurrentStart;
    bool wizardAtShoulder;
    bool wizardAtStart;
    char lastWizardText[32];
    Uint16 lastMovePic;

    void initSerial(void);
    void initScia(void);
    void initLina(void);
    void initUiEncoder(void);
    bool serialRxReady(void);
    Uint16 serialReadByte(void);
    bool serialRxError(void);
    void serialResetRxError(void);
    void sendByte(Uint16 data);
    void sendTerminator(void);
    void sendCommand(const char *command, const char *argument);
    void sendPage(Uint16 pageId);
    void sendText(const char *objectName, const char *text);
    void sendNumber(const char *objectName, Uint16 value);
    void sendValue(const char *objectName, Uint16 value);
    void sendPic(const char *objectName, Uint16 imageId);
    void sendVisible(const char *objectName, bool visible);
    void sendStartupText(void);
    void markReady(void);
    void pollStartupTimeout(void);
    void pollSerial(void);
    void pollUiEncoder(void);
    void queueEncoderStep(bool up);
    void handleUiButton(void);
    void handleTouch(Uint16 pageId, Uint16 componentId, Uint16 event);
    void handleCommand(const char *command);
    void queueButton(Uint16 pageId, Uint16 componentId);
    void openPage(Uint16 pageId);
    void refreshMain(void);
    void refreshMenu(void);
    void refreshCustomPitch(void);
    void refreshWizard(void);
    void formatSymbols(char *dest, const Uint16 *symbols, Uint16 count, bool preferLetters);
    void formatMessage(char *dest);
    void formatFeedLabel(char *dest);
    void formatFeedValue(char *dest);
    void formatFixedValue(char *dest, Uint16 integerDigits, Uint16 fractionDigits);
    void formatCustomPitch(char *dest);
    void formatWizardRunText(char *dest);
    void setWizardRunText(const char *text);
    void setMoveButtonPic(Uint16 imageId);
    char symbolToChar(Uint16 symbol, bool preferLetters);
    bool textChanged(const char *a, const char *b, Uint16 maxLength);
    void copyText(char *dest, const char *src, Uint16 maxLength);
    void refreshPopup(void);
    void closePopup(void);

public:
    NextionPanel(void);

    void initHardware(void);
    KEY_REG getKeys(void);
    void setRPM(Uint16 rpm);
    void setValue(const Uint16 *value);
    void setLEDs(LED_REG leds);
    void setMessage(const Uint16 *message, const char *text, MESSAGE_SEVERITY severity);
    void setBrightness(Uint16 brightness);
    void refresh(void);
    Uint16 getCustomPitchHundredths(void);
    Uint16 getWizardStarts(void);
    void setWizardStatus(bool atShoulder, bool atStart);
};

inline void NextionPanel :: setRPM(Uint16 rpm)
{
    this->rpm = rpm;
}

inline void NextionPanel :: setValue(const Uint16 *value)
{
    this->value = value;
}

inline void NextionPanel :: setLEDs(LED_REG leds)
{
    this->leds = leds;
}

inline void NextionPanel :: setMessage(const Uint16 *message, const char *text, MESSAGE_SEVERITY severity)
{
    this->message = message;
    this->messageText = text;
    this->messageSeverity = severity;
}

#endif // __NEXTION_PANEL_H
