// Clough42 Electronic Leadscrew
// https://github.com/clough42/electronic-leadscrew
//
// MIT License

#include "NextionPanel.h"
#include "Nextion.h"
#include "Configuration.h"
#include "F28x_Project.h"

#ifndef NEXTION_BAUD
#define NEXTION_BAUD 115200UL
#endif

#ifndef NEXTION_USE_LINA
#define NEXTION_USE_LINA 1
#endif

#ifndef NEXTION_UI_ENCODER_COUNTS_PER_STEP
#define NEXTION_UI_ENCODER_COUNTS_PER_STEP 4
#endif

#define NEXTION_LSPCLK_DIV 4UL
#define NEXTION_LSPCLK_HZ (CPU_CLOCK_HZ / NEXTION_LSPCLK_DIV)
#define NEXTION_RX_BUFFER_SIZE 16
#define NEXTION_TOUCH_EVENT 0x65
#define NEXTION_STARTUP_EVENT 0x88
#define NEXTION_TERMINATOR 0xff
#define NEXTION_STARTUP_WAIT_TICKS (UI_REFRESH_RATE_HZ * 5)
#define NEXTION_SPLASH_HOLD_TICKS (UI_REFRESH_RATE_HZ * 3)
#define NEXTION_UI_ENCODER_MAX_COUNT 0x00ffffffUL
#define NEXTION_UI_ENCODER_COUNT_RANGE (NEXTION_UI_ENCODER_MAX_COUNT + 1UL)

#define NEXTION_PAGE_SPLASH 0
#define NEXTION_PAGE_MAIN 1
#define NEXTION_PAGE_MENU 2
#define NEXTION_PAGE_CUSTOM_PITCH 3
#define NEXTION_PAGE_WIZARD_COUNT 4
#define NEXTION_PAGE_WIZARD_POSITION 5
#define NEXTION_PAGE_DIALOG 6
#define NEXTION_PAGE_WIZARD_RUN 7

#define NEXTION_WIZARD_WAIT_SPINDLE 0
#define NEXTION_WIZARD_RUNNING 1
#define NEXTION_WIZARD_STOP_SPINDLE 2
#define NEXTION_WIZARD_RETRACT_TOOL 3
#define NEXTION_WIZARD_MOVING_TO_START 4

NextionPanel :: NextionPanel(void)
{
    this->rpm = 0;
    this->value = NULL;
    this->leds.all = 0;
    this->pendingKeys.all = 0;
    this->message = NULL;
    this->messageText = NULL;
    this->messageSeverity = MESSAGE_INFO;
    this->lastTouchDownId = 0;
    this->lastRpmValue = 0xffff;
    this->lastFeedText[0] = 0;
    this->lastFeedLabel[0] = 0;
    this->lastMessageText[0] = 0;
    this->lastLeds = 0xffff;
    this->startupWaitTicks = NEXTION_STARTUP_WAIT_TICKS;
    this->startupHoldTicks = NEXTION_SPLASH_HOLD_TICKS;
    this->activePage = NEXTION_PAGE_SPLASH;
    this->popupVisible = false;
    this->nextionReady = false;
    this->nextionStartupSeen = false;
    this->uiEncoderPosition = 0;
    this->uiEncoderDelta = 0;
    this->uiButtonStable = 1;
    this->uiButtonLastSample = 1;
    this->uiButtonDebounce = 0;
    this->uiButtonPressed = false;
    this->customPitchHundredths = 100;
    this->wizardStarts = 1;
    this->wizardStep = 0;
    this->wizardRunState = NEXTION_WIZARD_WAIT_SPINDLE;
    this->wizardCurrentStart = 1;
    this->wizardAtShoulder = false;
    this->wizardAtStart = false;
    this->lastWizardText[0] = 0;
    this->lastMovePic = 0xffff;
}

void NextionPanel :: initHardware(void)
{
    initSerial();
    initUiEncoder();
}

void NextionPanel :: initSerial(void)
{
#if NEXTION_USE_LINA
    initLina();
#else
    initScia();
#endif
}

void NextionPanel :: initScia(void)
{
    Uint32 brr;

    EALLOW;

    ClkCfgRegs.LOSPCP.bit.LSPCLKDIV = 0b010; // LSPCLK = SYSCLK/4

    GpioCtrlRegs.GPAMUX2.bit.GPIO28 = 0x1;   // SCIA_RX
    GpioCtrlRegs.GPAGMUX2.bit.GPIO28 = 0x0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO29 = 0x1;   // SCIA_TX
    GpioCtrlRegs.GPAGMUX2.bit.GPIO29 = 0x0;

    GpioCtrlRegs.GPADIR.bit.GPIO28 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO29 = 1;
    GpioCtrlRegs.GPAPUD.bit.GPIO28 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO29 = 0;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO28 = 3;    // async input qualification

    EDIS;

    brr = ((NEXTION_LSPCLK_HZ + (NEXTION_BAUD * 4UL)) / (NEXTION_BAUD * 8UL)) - 1UL;

    SciaRegs.SCICTL1.bit.SWRESET = 0;
    SciaRegs.SCICCR.all = 0x0007;            // 8 data bits, 1 stop bit, no parity
    SciaRegs.SCICTL1.all = 0x0003;           // enable TX and RX
    SciaRegs.SCICTL2.all = 0x0000;           // polled I/O
    SciaRegs.SCIHBAUD.all = (brr >> 8) & 0xff;
    SciaRegs.SCILBAUD.all = brr & 0xff;
    SciaRegs.SCIFFTX.all = 0xe040;           // enable and reset TX FIFO
    SciaRegs.SCIFFRX.all = 0x2044;           // reset RX FIFO and clear flags
    SciaRegs.SCIFFCT.all = 0x0000;
    SciaRegs.SCICTL1.all = 0x0023;           // release SCI reset
}

void NextionPanel :: initLina(void)
{
    Uint32 divisorTimes16;
    Uint32 integerPrescaler;
    Uint32 fractionalPrescaler;

    EALLOW;

    ClkCfgRegs.LOSPCP.bit.LSPCLKDIV = 0b010; // LSPCLK = SYSCLK/4

    GpioCtrlRegs.GPBMUX1.bit.GPIO32 = 0x1;   // mux 5: LINA_TX
    GpioCtrlRegs.GPBGMUX1.bit.GPIO32 = 0x1;
    GpioCtrlRegs.GPBMUX1.bit.GPIO33 = 0x1;   // mux 5: LINA_RX
    GpioCtrlRegs.GPBGMUX1.bit.GPIO33 = 0x1;

    GpioCtrlRegs.GPBDIR.bit.GPIO32 = 1;
    GpioCtrlRegs.GPBDIR.bit.GPIO33 = 0;
    GpioCtrlRegs.GPBPUD.bit.GPIO32 = 0;
    GpioCtrlRegs.GPBPUD.bit.GPIO33 = 0;
    GpioCtrlRegs.GPBQSEL1.bit.GPIO33 = 3;    // async input qualification

    EDIS;

    divisorTimes16 = ((NEXTION_LSPCLK_HZ * 16UL) + (NEXTION_BAUD * 8UL)) / NEXTION_BAUD;
    integerPrescaler = (divisorTimes16 / 16UL) - 1UL;
    fractionalPrescaler = divisorTimes16 & 0x0fUL;

    LinaRegs.SCIGCR0.bit.RESET = 0;
    LinaRegs.SCIGCR0.bit.RESET = 1;
    LinaRegs.SCIGCR1.bit.SWnRST = 0;

    LinaRegs.SCIGCR1.all = 0;
    LinaRegs.SCIGCR1.bit.COMMMODE = 1;       // SCI compatibility mode
    LinaRegs.SCIGCR1.bit.TIMINGMODE = 1;     // SCI timing
    LinaRegs.SCIGCR1.bit.CLK_MASTER = 1;     // enable SCI clock
    LinaRegs.SCIGCR1.bit.LINMODE = 0;
    LinaRegs.SCIGCR1.bit.RXENA = 1;
    LinaRegs.SCIGCR1.bit.TXENA = 1;

    LinaRegs.SCIFORMAT.all = 0;
    LinaRegs.SCIFORMAT.bit.CHAR = 7;         // 8-bit characters
    LinaRegs.SCIFORMAT.bit.LENGTH = 0;

    LinaRegs.BRSR.bit.SCI_LIN_PSL = integerPrescaler & 0xffffUL;
    LinaRegs.BRSR.bit.SCI_LIN_PSH = (integerPrescaler >> 16) & 0xffUL;
    LinaRegs.BRSR.bit.M = fractionalPrescaler & 0x0fUL;

    LinaRegs.SCICLEARINT.all = 0xffffffffUL;
    LinaRegs.LIN_GLB_INT_EN.all = 0;
    LinaRegs.SCIPIO0.bit.RXFUNC = 1;
    LinaRegs.SCIPIO0.bit.TXFUNC = 1;
    LinaRegs.SCIGCR1.bit.SWnRST = 1;
}

void NextionPanel :: initUiEncoder(void)
{
    EALLOW;

    GpioCtrlRegs.GPAPUD.bit.GPIO14 = 0;      // EQEP2A
    GpioCtrlRegs.GPAPUD.bit.GPIO15 = 0;      // EQEP2B
    GpioCtrlRegs.GPAPUD.bit.GPIO26 = 0;      // pushbutton, active low

    GpioCtrlRegs.GPAQSEL1.bit.GPIO14 = 0;
    GpioCtrlRegs.GPAQSEL1.bit.GPIO15 = 0;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO26 = 0;

    GpioCtrlRegs.GPAMUX1.bit.GPIO14 = 2;
    GpioCtrlRegs.GPAGMUX1.bit.GPIO14 = 2;
    GpioCtrlRegs.GPAMUX1.bit.GPIO15 = 2;
    GpioCtrlRegs.GPAGMUX1.bit.GPIO15 = 2;

    GpioCtrlRegs.GPAMUX2.bit.GPIO26 = 0;     // GPIO, not EQEP2I
    GpioCtrlRegs.GPAGMUX2.bit.GPIO26 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO26 = 0;

    EDIS;

    EQep2Regs.QDECCTL.all = 0;
    EQep2Regs.QDECCTL.bit.QSRC = 0;          // quadrature count mode
    EQep2Regs.QEPCTL.all = 0;
    EQep2Regs.QEPCTL.bit.FREE_SOFT = 2;
    EQep2Regs.QEPCTL.bit.PCRM = 1;
    EQep2Regs.QPOSMAX = NEXTION_UI_ENCODER_MAX_COUNT;
    EQep2Regs.QPOSCNT = 0;
    EQep2Regs.QEPCTL.bit.QPEN = 1;

    this->uiEncoderPosition = EQep2Regs.QPOSCNT;
}

KEY_REG NextionPanel :: getKeys(void)
{
    KEY_REG keys;

    pollSerial();
    pollUiEncoder();

    keys = this->pendingKeys;
    this->pendingKeys.all = 0;

    return keys;
}

void NextionPanel :: setBrightness(Uint16 brightness)
{
    if( brightness > 8 ) brightness = 8;

    sendNumber("dim", brightness * 12);
}

Uint16 NextionPanel :: getCustomPitchHundredths(void)
{
    return this->customPitchHundredths;
}

Uint16 NextionPanel :: getWizardStarts(void)
{
    if( this->wizardStarts < 1 ) return 1;
    if( this->wizardStarts > 9 ) return 9;

    return this->wizardStarts;
}

void NextionPanel :: setWizardStatus(bool atShoulder, bool atStart)
{
    this->wizardAtShoulder = atShoulder;
    this->wizardAtStart = atStart;
}

void NextionPanel :: refresh(void)
{
    pollStartupTimeout();

    if( ! this->nextionReady )
    {
        sendStartupText();
        return;
    }

    if( this->message != NULL )
    {
        refreshPopup();
        return;
    }

    if( this->popupVisible )
    {
        closePopup();
    }

    switch( this->activePage )
    {
    case NEXTION_PAGE_MENU:
        refreshMenu();
        break;
    case NEXTION_PAGE_CUSTOM_PITCH:
        refreshCustomPitch();
        break;
    case NEXTION_PAGE_WIZARD_COUNT:
    case NEXTION_PAGE_WIZARD_POSITION:
    case NEXTION_PAGE_DIALOG:
    case NEXTION_PAGE_WIZARD_RUN:
        refreshWizard();
        break;
    default:
        refreshMain();
        break;
    }
}

void NextionPanel :: sendStartupText(void)
{
    static Uint16 startupRefreshDivider = 0;

    if( startupRefreshDivider++ != 0 )
    {
        if( startupRefreshDivider < UI_REFRESH_RATE_HZ ) return;
        startupRefreshDivider = 0;
    }

    sendText(txt_title1, "nELS v1.0");
    sendText(txt_title2, "Based on Clough42 v1.4.00");
}

void NextionPanel :: markReady(void)
{
    if( this->nextionReady ) return;

    this->nextionReady = true;
    openPage(NEXTION_PAGE_MAIN);
}

void NextionPanel :: pollStartupTimeout(void)
{
    if( this->nextionReady ) return;

    if( this->startupHoldTicks > 0 )
    {
        this->startupHoldTicks--;
    }

    if( this->nextionStartupSeen && this->startupHoldTicks == 0 )
    {
        markReady();
        return;
    }

    if( this->startupWaitTicks > 0 )
    {
        this->startupWaitTicks--;
        if( this->startupWaitTicks > 0 ) return;
    }

    markReady();
}

void NextionPanel :: pollSerial(void)
{
    static Uint16 touchPacket[7];
    static Uint16 touchPacketIndex = 0;
    static Uint16 startupPacketIndex = 0;
    static char command[NEXTION_RX_BUFFER_SIZE];
    static Uint16 commandIndex = 0;
    Uint16 c;

    while( serialRxReady() )
    {
        c = serialReadByte() & 0xff;

        if( touchPacketIndex > 0 || c == NEXTION_TOUCH_EVENT )
        {
            touchPacket[touchPacketIndex++] = c;
            if( touchPacketIndex >= 7 )
            {
                if( touchPacket[0] == NEXTION_TOUCH_EVENT &&
                    touchPacket[4] == NEXTION_TERMINATOR &&
                    touchPacket[5] == NEXTION_TERMINATOR &&
                    touchPacket[6] == NEXTION_TERMINATOR )
                {
                    handleTouch(touchPacket[1], touchPacket[2], touchPacket[3]);
                }
                touchPacketIndex = 0;
            }
            continue;
        }

        if( c == NEXTION_STARTUP_EVENT )
        {
            startupPacketIndex = 1;
            continue;
        }
        if( startupPacketIndex > 0 )
        {
            if( c == NEXTION_TERMINATOR )
            {
                startupPacketIndex++;
                if( startupPacketIndex >= 4 )
                {
                    startupPacketIndex = 0;
                    this->nextionStartupSeen = true;
                }
            }
            else
            {
                startupPacketIndex = 0;
            }
            continue;
        }

        if( c == NEXTION_TERMINATOR || c == '\r' || c == '\n' )
        {
            if( commandIndex > 0 )
            {
                command[commandIndex] = 0;
                handleCommand(command);
                commandIndex = 0;
            }
        }
        else if( commandIndex < NEXTION_RX_BUFFER_SIZE - 1 )
        {
            command[commandIndex++] = (char)c;
        }
        else
        {
            commandIndex = 0;
        }
    }

    if( serialRxError() )
    {
        serialResetRxError();
    }
}

void NextionPanel :: pollUiEncoder(void)
{
    Uint32 position = EQep2Regs.QPOSCNT;
    long delta = (long)position - (long)this->uiEncoderPosition;
    Uint16 buttonSample;

    if( delta > (long)(NEXTION_UI_ENCODER_COUNT_RANGE / 2UL) )
    {
        delta -= (long)NEXTION_UI_ENCODER_COUNT_RANGE;
    }
    else if( delta < -((long)(NEXTION_UI_ENCODER_COUNT_RANGE / 2UL)) )
    {
        delta += (long)NEXTION_UI_ENCODER_COUNT_RANGE;
    }

    this->uiEncoderPosition = position;
    this->uiEncoderDelta += delta;

    while( this->uiEncoderDelta >= NEXTION_UI_ENCODER_COUNTS_PER_STEP )
    {
        this->uiEncoderDelta -= NEXTION_UI_ENCODER_COUNTS_PER_STEP;
        queueEncoderStep(true);
    }
    while( this->uiEncoderDelta <= -NEXTION_UI_ENCODER_COUNTS_PER_STEP )
    {
        this->uiEncoderDelta += NEXTION_UI_ENCODER_COUNTS_PER_STEP;
        queueEncoderStep(false);
    }

    buttonSample = GpioDataRegs.GPADAT.bit.GPIO26 ? 1 : 0;
    if( buttonSample != this->uiButtonLastSample )
    {
        this->uiButtonLastSample = buttonSample;
        this->uiButtonDebounce = 1;
    }
    else if( this->uiButtonDebounce > 0 )
    {
        this->uiButtonDebounce--;
        if( this->uiButtonDebounce == 0 )
        {
            this->uiButtonStable = buttonSample;
            handleUiButton();
        }
    }
}

void NextionPanel :: queueEncoderStep(bool up)
{
    switch( this->activePage )
    {
    case NEXTION_PAGE_CUSTOM_PITCH:
        if( up )
        {
            if( this->customPitchHundredths < 100 ) this->customPitchHundredths++;
        }
        else if( this->customPitchHundredths > 10 )
        {
            this->customPitchHundredths--;
        }
        this->lastFeedText[0] = 0;
        break;
    case NEXTION_PAGE_WIZARD_COUNT:
        if( up )
        {
            if( this->wizardStarts < 9 ) this->wizardStarts++;
        }
        else if( this->wizardStarts > 1 )
        {
            this->wizardStarts--;
        }
        this->lastRpmValue = 0xffff;
        break;
    case NEXTION_PAGE_MAIN:
        if( up ) this->pendingKeys.bit.UP = 1;
        else this->pendingKeys.bit.DOWN = 1;
        break;
    default:
        break;
    }
}

void NextionPanel :: handleUiButton(void)
{
    bool pressed = this->uiButtonStable == 0;

    if( pressed && ! this->uiButtonPressed )
    {
        switch( this->activePage )
        {
        case NEXTION_PAGE_MAIN:
            openPage(NEXTION_PAGE_MENU);
            break;
        case NEXTION_PAGE_MENU:
            openPage(NEXTION_PAGE_MAIN);
            break;
        case NEXTION_PAGE_CUSTOM_PITCH:
            this->pendingKeys.bit.CUSTOM_THREAD = 1;
            openPage(NEXTION_PAGE_MAIN);
            break;
        case NEXTION_PAGE_WIZARD_COUNT:
            this->wizardStep = 0;
            openPage(NEXTION_PAGE_WIZARD_POSITION);
            break;
        case NEXTION_PAGE_WIZARD_POSITION:
            if( this->wizardStep == 0 )
            {
                this->pendingKeys.bit.WIZARD_SET_SHOULDER = 1;
            }
            else
            {
                this->pendingKeys.bit.WIZARD_SET_START = 1;
            }
            this->wizardStep++;
            if( this->wizardStep == 1 )
            {
                openPage(NEXTION_PAGE_DIALOG);
            }
            else
            {
                openPage(NEXTION_PAGE_WIZARD_RUN);
            }
            break;
        case NEXTION_PAGE_DIALOG:
            openPage(NEXTION_PAGE_WIZARD_POSITION);
            break;
        case NEXTION_PAGE_WIZARD_RUN:
            this->pendingKeys.bit.WIZARD_END = 1;
            openPage(NEXTION_PAGE_MAIN);
            break;
        default:
            this->pendingKeys.bit.MESSAGE_CLOSE = 1;
            break;
        }
    }

    this->uiButtonPressed = pressed;
}

void NextionPanel :: handleTouch(Uint16 pageId, Uint16 componentId, Uint16 event)
{
    if( ! this->nextionReady )
    {
        markReady();
    }

    if( event == 1 )
    {
        queueButton(pageId, componentId);
        this->lastTouchDownId = componentId;
    }
    else if( event == 0 )
    {
        if( this->lastTouchDownId == 0 )
        {
            queueButton(pageId, componentId);
        }
        if( this->lastTouchDownId == componentId )
        {
            this->lastTouchDownId = 0;
        }
    }
}

void NextionPanel :: handleCommand(const char *command)
{
    if( command[0] == 'u' )
    {
        this->pendingKeys.bit.UP = 1;
    }
    else if( command[0] == 'd' )
    {
        this->pendingKeys.bit.DOWN = 1;
    }
    else if( command[0] == 's' )
    {
        this->pendingKeys.bit.SET = 1;
    }
    else if( command[0] == 'r' )
    {
        this->pendingKeys.bit.FWD_REV = 1;
    }
    else if( command[0] == 'm' )
    {
        this->pendingKeys.bit.IN_MM = 1;
    }
    else if( command[0] == 'f' )
    {
        this->pendingKeys.bit.FEED_THREAD = 1;
    }
    else if( command[0] == 'p' )
    {
        this->pendingKeys.bit.POWER = 1;
    }
}

void NextionPanel :: queueButton(Uint16 pageId, Uint16 componentId)
{
    if( this->popupVisible )
    {
        if( componentId == btn_back6_id )
        {
            this->pendingKeys.bit.MESSAGE_CLOSE = 1;
        }
        return;
    }

    switch( pageId )
    {
    case NEXTION_PAGE_SPLASH:
        markReady();
        break;
    case NEXTION_PAGE_MAIN:
        if( componentId == btn_fwd1_id )
        {
            if( this->leds.bit.REVERSE ) this->pendingKeys.bit.FWD_REV = 1;
        }
        else if( componentId == btn_rev1_id )
        {
            if( ! this->leds.bit.REVERSE ) this->pendingKeys.bit.FWD_REV = 1;
        }
        else if( componentId == btn_gear_id )
        {
            openPage(NEXTION_PAGE_MENU);
        }
        break;
    case NEXTION_PAGE_MENU:
        if( componentId == btn_back2_id )
        {
            openPage(NEXTION_PAGE_MAIN);
        }
        else if( componentId == btn_metric_id )
        {
            if( ! this->leds.bit.MM ) this->pendingKeys.bit.IN_MM = 1;
        }
        else if( componentId == btn_imperial_id )
        {
            if( this->leds.bit.MM ) this->pendingKeys.bit.IN_MM = 1;
        }
        else if( componentId == btn_thread_id )
        {
            if( ! this->leds.bit.THREAD ) this->pendingKeys.bit.FEED_THREAD = 1;
        }
        else if( componentId == btn_feed_id )
        {
            if( this->leds.bit.THREAD ) this->pendingKeys.bit.FEED_THREAD = 1;
        }
        else if( componentId == btn_cust_pitch_id )
        {
            openPage(NEXTION_PAGE_CUSTOM_PITCH);
        }
        else if( componentId == btn_wizard_id )
        {
            openPage(NEXTION_PAGE_WIZARD_COUNT);
        }
        break;
    case NEXTION_PAGE_CUSTOM_PITCH:
        if( componentId == btn_back3_id )
        {
            openPage(NEXTION_PAGE_MENU);
        }
        break;
    case NEXTION_PAGE_WIZARD_COUNT:
        if( componentId == btn_back4_id )
        {
            openPage(NEXTION_PAGE_MENU);
        }
        break;
    case NEXTION_PAGE_WIZARD_POSITION:
        if( componentId == btn_back5_id )
        {
            openPage(NEXTION_PAGE_WIZARD_COUNT);
        }
        else
        {
            if( this->wizardStep == 0 )
            {
                this->pendingKeys.bit.WIZARD_SET_SHOULDER = 1;
            }
            else
            {
                this->pendingKeys.bit.WIZARD_SET_START = 1;
            }
            this->wizardStep++;
            if( this->wizardStep == 1 )
            {
                openPage(NEXTION_PAGE_DIALOG);
            }
            else
            {
                openPage(NEXTION_PAGE_WIZARD_RUN);
            }
        }
        break;
    case NEXTION_PAGE_DIALOG:
        if( componentId == btn_back6_id )
        {
            openPage(NEXTION_PAGE_WIZARD_POSITION);
        }
        break;
    case NEXTION_PAGE_WIZARD_RUN:
        if( componentId == btn_back7_id )
        {
            this->pendingKeys.bit.WIZARD_END = 1;
            openPage(NEXTION_PAGE_MENU);
        }
        else if( componentId == btn_move_to_start_id )
        {
            if( this->wizardRunState == NEXTION_WIZARD_RETRACT_TOOL )
            {
                this->pendingKeys.bit.WIZARD_MOVE_TO_START = 1;
                this->wizardRunState = NEXTION_WIZARD_MOVING_TO_START;
                this->lastWizardText[0] = 0;
            }
        }
        break;
    default:
        break;
    }
}

void NextionPanel :: openPage(Uint16 pageId)
{
    this->activePage = pageId;
    this->popupVisible = false;
    this->lastRpmValue = 0xffff;
    this->lastFeedText[0] = 0;
    this->lastFeedLabel[0] = 0;
    this->lastMessageText[0] = 0;
    this->lastWizardText[0] = 0;
    this->lastLeds = 0xffff;
    this->lastMovePic = 0xffff;
    if( pageId == NEXTION_PAGE_WIZARD_RUN )
    {
        this->wizardRunState = NEXTION_WIZARD_WAIT_SPINDLE;
        this->wizardCurrentStart = 1;
    }
    sendPage(pageId);
}

void NextionPanel :: refreshMain(void)
{
    char feedText[12];
    char feedLabel[20];
    Uint16 rpmValue = this->rpm % 10000;

    if( this->activePage != NEXTION_PAGE_MAIN )
    {
        openPage(NEXTION_PAGE_MAIN);
    }

    if( rpmValue != this->lastRpmValue )
    {
        sendValue(num_rpm_disp, rpmValue);
        this->lastRpmValue = rpmValue;
    }

    formatFeedValue(feedText);
    if( textChanged(feedText, this->lastFeedText, sizeof(this->lastFeedText)) )
    {
        sendText(txt_feed_disp, feedText);
        copyText(this->lastFeedText, feedText, sizeof(this->lastFeedText));
    }

    formatFeedLabel(feedLabel);
    if( textChanged(feedLabel, this->lastFeedLabel, sizeof(this->lastFeedLabel)) )
    {
        sendText(txt_feed_lbl, feedLabel);
        copyText(this->lastFeedLabel, feedLabel, sizeof(this->lastFeedLabel));
    }

    if( this->leds.all != this->lastLeds )
    {
        sendPic(btn_fwd, this->leds.bit.FORWARD ? btn_fwd1_img_sel_id : btn_fwd1_img_unsel_id);
        sendPic(btn_rev, this->leds.bit.REVERSE ? btn_rev1_img_sel_id : btn_rev1_img_unsel_id);
        this->lastLeds = this->leds.all;
    }
}

void NextionPanel :: refreshMenu(void)
{
    if( this->leds.all == this->lastLeds ) return;

    sendPic(btn_metric, this->leds.bit.MM ? btn_metric_img_sel_id : btn_metric_img_unsel_id);
    sendPic(btn_imperial, (this->leds.bit.INCH || this->leds.bit.TPI) ? btn_imperial_img_sel_id : btn_imperial_img_unsel_id);
    sendPic(btn_thread, this->leds.bit.THREAD ? btn_thread_img_sel_id : btn_thread_img_unsel_id);
    sendPic(btn_feed, this->leds.bit.FEED ? btn_feed_img_sel_id : btn_feed_img_unsel_id);
    this->lastLeds = this->leds.all;
}

void NextionPanel :: refreshCustomPitch(void)
{
    char pitchText[12];

    sendText(txt_title3, "CUSTOM THREAD");
    sendText(txt_label3, "PITCH MM");

    formatCustomPitch(pitchText);
    if( textChanged(pitchText, this->lastFeedText, sizeof(this->lastFeedText)) )
    {
        sendText(txt_pitch3, pitchText);
        copyText(this->lastFeedText, pitchText, sizeof(this->lastFeedText));
    }
}

void NextionPanel :: refreshWizard(void)
{
    if( this->activePage == NEXTION_PAGE_WIZARD_COUNT )
    {
        sendText(txt_title4, "# OF THREAD STARTS");
        if( getWizardStarts() != this->lastRpmValue )
        {
            sendValue(num_count4, getWizardStarts());
            this->lastRpmValue = getWizardStarts();
        }
    }
    else if( this->activePage == NEXTION_PAGE_WIZARD_POSITION )
    {
        if( this->wizardStep == 0 )
        {
            sendText(txt_title5, "MOVE TOOL TO SHOULDER POSITION");
            sendVisible(pic_shoulder, true);
            sendVisible(pic_start, false);
        }
        else
        {
            sendText(txt_title5, "MOVE TOOL TO START POSITION");
            sendVisible(pic_shoulder, false);
            sendVisible(pic_start, true);
        }
    }
    else if( this->activePage == NEXTION_PAGE_DIALOG )
    {
        sendText(txt_dlg6, "       ENGAGE HALF-NUT\r\nIT MUST REMAIN ENGAGED\r\n      FOR THE DURATION");
    }
    else if( this->activePage == NEXTION_PAGE_WIZARD_RUN )
    {
        char text[32];

        if( this->wizardRunState == NEXTION_WIZARD_WAIT_SPINDLE )
        {
            if( this->rpm > 0 )
            {
                this->wizardRunState = NEXTION_WIZARD_RUNNING;
                this->lastWizardText[0] = 0;
            }
        }
        else if( this->wizardRunState == NEXTION_WIZARD_RUNNING )
        {
            if( this->wizardAtShoulder )
            {
                this->wizardRunState = NEXTION_WIZARD_STOP_SPINDLE;
                this->lastWizardText[0] = 0;
            }
        }
        else if( this->wizardRunState == NEXTION_WIZARD_STOP_SPINDLE )
        {
            if( this->rpm == 0 )
            {
                this->wizardRunState = NEXTION_WIZARD_RETRACT_TOOL;
                this->lastWizardText[0] = 0;
            }
        }
        else if( this->wizardRunState == NEXTION_WIZARD_MOVING_TO_START )
        {
            if( this->wizardAtStart )
            {
                this->wizardCurrentStart++;
                if( this->wizardCurrentStart > getWizardStarts() )
                {
                    this->wizardCurrentStart = 1;
                }
                this->wizardRunState = NEXTION_WIZARD_WAIT_SPINDLE;
                this->lastWizardText[0] = 0;
            }
        }

        if( this->wizardRunState == NEXTION_WIZARD_WAIT_SPINDLE )
        {
            setWizardRunText("SET TOOL AND START SPINDLE");
            setMoveButtonPic(btn_move_to_start_img_unsel_id);
        }
        else if( this->wizardRunState == NEXTION_WIZARD_RUNNING )
        {
            formatWizardRunText(text);
            setWizardRunText(text);
            setMoveButtonPic(btn_move_to_start_img_unsel_id);
        }
        else if( this->wizardRunState == NEXTION_WIZARD_STOP_SPINDLE )
        {
            setWizardRunText("STOP SPINDLE");
            setMoveButtonPic(btn_move_to_start_img_unsel_id);
        }
        else if( this->wizardRunState == NEXTION_WIZARD_RETRACT_TOOL )
        {
            setWizardRunText("RETRACT TOOL");
            setMoveButtonPic(btn_move_to_start_img_sel_id);
        }
        else
        {
            setWizardRunText("MOVING TO START");
            setMoveButtonPic(btn_move_to_start_img_unsel_id);
        }
    }
}

void NextionPanel :: formatFeedLabel(char *dest)
{
    if( this->leds.bit.THREAD )
    {
        if( this->leds.bit.MM )
        {
            copyText(dest, "THREAD: mm pitch", 20);
        }
        else
        {
            copyText(dest, "THREAD: TPI", 20);
        }
    }
    else
    {
        if( this->leds.bit.MM )
        {
            copyText(dest, "FEED: mm/rev", 20);
        }
        else
        {
            copyText(dest, "FEED: in/rev", 20);
        }
    }
}

void NextionPanel :: formatFeedValue(char *dest)
{
    if( this->leds.bit.THREAD )
    {
        if( this->leds.bit.MM )
        {
            formatFixedValue(dest, 1, 2);
        }
        else
        {
            formatFixedValue(dest, 2, 2);
        }
    }
    else
    {
        if( this->leds.bit.MM )
        {
            formatFixedValue(dest, 1, 2);
        }
        else
        {
            formatFixedValue(dest, 1, 3);
        }
    }
}

void NextionPanel :: formatFixedValue(char *dest, Uint16 integerDigits, Uint16 fractionDigits)
{
    char raw[12];
    char integer[6];
    char fraction[6];
    Uint16 rawIndex;
    Uint16 integerCount = 0;
    Uint16 fractionCount = 0;
    Uint16 out = 0;
    Uint16 i;
    bool afterPoint = false;

    formatSymbols(raw, this->value, 4, false);

    for( rawIndex = 0; raw[rawIndex] != 0; rawIndex++ )
    {
        char c = raw[rawIndex];

        if( c == '.' )
        {
            afterPoint = true;
        }
        else if( c >= '0' && c <= '9' )
        {
            if( afterPoint )
            {
                if( fractionCount < sizeof(fraction) )
                {
                    fraction[fractionCount++] = c;
                }
            }
            else if( integerCount < sizeof(integer) )
            {
                integer[integerCount++] = c;
            }
        }
    }

    if( integerCount == 0 )
    {
        integer[integerCount++] = '0';
    }

    while( integerCount < integerDigits )
    {
        dest[out++] = '0';
        integerDigits--;
    }

    for( i = 0; i < integerCount; i++ )
    {
        dest[out++] = integer[i];
    }

    dest[out++] = '.';

    for( i = 0; i < fractionDigits; i++ )
    {
        if( i < fractionCount )
        {
            dest[out++] = fraction[i];
        }
        else
        {
            dest[out++] = '0';
        }
    }

    dest[out] = 0;
}

void NextionPanel :: formatCustomPitch(char *dest)
{
    Uint16 whole = this->customPitchHundredths / 100;
    Uint16 fraction = this->customPitchHundredths % 100;
    Uint16 i = 0;
    char digits[5];
    Uint16 count = 0;

    if( whole == 0 )
    {
        dest[i++] = '0';
    }
    else
    {
        while( whole > 0 && count < sizeof(digits) )
        {
            digits[count++] = (char)('0' + (whole % 10));
            whole /= 10;
        }
        while( count > 0 )
        {
            dest[i++] = digits[--count];
        }
    }

    dest[i++] = '.';
    dest[i++] = (char)('0' + (fraction / 10));
    dest[i++] = (char)('0' + (fraction % 10));
    dest[i] = 0;
}

void NextionPanel :: formatWizardRunText(char *dest)
{
    Uint16 i = 0;
    Uint16 current = this->wizardCurrentStart;
    Uint16 total = getWizardStarts();

    if( current == 0 ) current = 1;
    if( current > total ) current = total;

    copyText(dest, "RUNNING ", 32);
    while( dest[i] != 0 ) i++;

    if( current >= 10 )
    {
        dest[i++] = (char)('0' + (current / 10));
    }
    dest[i++] = (char)('0' + (current % 10));
    dest[i++] = '.';
    if( total >= 10 )
    {
        dest[i++] = (char)('0' + (total / 10));
    }
    dest[i++] = (char)('0' + (total % 10));
    dest[i] = 0;
}

void NextionPanel :: setWizardRunText(const char *text)
{
    if( textChanged(text, this->lastWizardText, sizeof(this->lastWizardText)) )
    {
        sendText(txt_title7, text);
        copyText(this->lastWizardText, text, sizeof(this->lastWizardText));
    }
}

void NextionPanel :: setMoveButtonPic(Uint16 imageId)
{
    if( imageId != this->lastMovePic )
    {
        sendPic(btn_move, imageId);
        this->lastMovePic = imageId;
    }
}

void NextionPanel :: refreshPopup(void)
{
    char text[21];

    formatMessage(text);

    if( ! this->popupVisible )
    {
        sendPage(NEXTION_PAGE_DIALOG);
        this->popupVisible = true;
        this->lastMessageText[0] = 0;
    }

    if( textChanged(text, this->lastMessageText, sizeof(this->lastMessageText)) )
    {
        sendText(txt_dlg6, text);
        copyText(this->lastMessageText, text, sizeof(this->lastMessageText));
    }
}

void NextionPanel :: closePopup(void)
{
    this->popupVisible = false;
    openPage(NEXTION_PAGE_MAIN);
}

void NextionPanel :: formatSymbols(char *dest, const Uint16 *symbols, Uint16 count, bool preferLetters)
{
    Uint16 i;
    Uint16 out = 0;
    bool leading = true;

    if( symbols == NULL )
    {
        dest[0] = 0;
        return;
    }

    for( i = 0; i < count && out < 10; i++ )
    {
        Uint16 symbol = symbols[i];
        char c = symbolToChar(symbol & ~POINT, preferLetters);
        bool point = (symbol & POINT) != 0;

        if( c == ' ' && point )
        {
            leading = false;
            dest[out++] = '.';
            continue;
        }

        if( c != ' ' || !leading || i == count - 1 )
        {
            leading = false;
            dest[out++] = c;
        }

        if( point && out < 10 )
        {
            dest[out++] = '.';
        }
    }

    dest[out] = 0;
}

void NextionPanel :: formatMessage(char *dest)
{
    Uint16 i;

    if( this->messageText != NULL )
    {
        for( i = 0; i < 20 && this->messageText[i] != 0; i++ )
        {
            dest[i] = this->messageText[i];
        }
        dest[i] = 0;
        return;
    }

    formatSymbols(dest, this->message, 8, true);
}

char NextionPanel :: symbolToChar(Uint16 symbol, bool preferLetters)
{
    if( preferLetters )
    {
        if( symbol == LETTER_A ) return 'A';
        if( symbol == LETTER_B ) return 'B';
        if( symbol == LETTER_C ) return 'C';
        if( symbol == LETTER_D ) return 'D';
        if( symbol == LETTER_E ) return 'E';
        if( symbol == LETTER_F ) return 'F';
        if( symbol == LETTER_G ) return 'G';
        if( symbol == LETTER_H ) return 'H';
        if( symbol == LETTER_I ) return 'I';
        if( symbol == LETTER_J ) return 'J';
        if( symbol == LETTER_K ) return 'K';
        if( symbol == LETTER_L ) return 'L';
        if( symbol == LETTER_M ) return 'M';
        if( symbol == LETTER_N ) return 'N';
        if( symbol == LETTER_O ) return 'O';
        if( symbol == LETTER_P ) return 'P';
        if( symbol == LETTER_Q ) return 'Q';
        if( symbol == LETTER_R ) return 'R';
        if( symbol == LETTER_S ) return 'S';
        if( symbol == LETTER_T ) return 'T';
        if( symbol == LETTER_U ) return 'U';
        if( symbol == LETTER_V ) return 'V';
        if( symbol == LETTER_W ) return 'W';
        if( symbol == LETTER_X ) return 'X';
        if( symbol == LETTER_Y ) return 'Y';
        if( symbol == LETTER_Z ) return 'Z';
    }

    if( symbol == ZERO ) return '0';
    if( symbol == ONE ) return '1';
    if( symbol == TWO ) return '2';
    if( symbol == THREE ) return '3';
    if( symbol == FOUR ) return '4';
    if( symbol == FIVE ) return '5';
    if( symbol == SIX ) return '6';
    if( symbol == SEVEN ) return '7';
    if( symbol == EIGHT ) return '8';
    if( symbol == NINE ) return '9';

    if( symbol == LETTER_A ) return 'A';
    if( symbol == LETTER_B ) return 'B';
    if( symbol == LETTER_C ) return 'C';
    if( symbol == LETTER_D ) return 'D';
    if( symbol == LETTER_E ) return 'E';
    if( symbol == LETTER_F ) return 'F';
    if( symbol == LETTER_G ) return 'G';
    if( symbol == LETTER_H ) return 'H';
    if( symbol == LETTER_I ) return 'I';
    if( symbol == LETTER_J ) return 'J';
    if( symbol == LETTER_K ) return 'K';
    if( symbol == LETTER_L ) return 'L';
    if( symbol == LETTER_M ) return 'M';
    if( symbol == LETTER_N ) return 'N';
    if( symbol == LETTER_P ) return 'P';
    if( symbol == LETTER_Q ) return 'Q';
    if( symbol == LETTER_R ) return 'R';
    if( symbol == LETTER_T ) return 'T';
    if( symbol == LETTER_U ) return 'U';
    if( symbol == LETTER_V ) return 'V';
    if( symbol == LETTER_W ) return 'W';
    if( symbol == LETTER_X ) return 'X';
    if( symbol == LETTER_Y ) return 'Y';
    if( symbol == LETTER_Z ) return 'Z';
    if( symbol == DASH ) return '-';
    if( symbol == BLANK ) return ' ';
    return '?';
}

bool NextionPanel :: textChanged(const char *a, const char *b, Uint16 maxLength)
{
    Uint16 i;

    for( i = 0; i < maxLength; i++ )
    {
        if( a[i] != b[i] ) return true;
        if( a[i] == 0 ) return false;
    }

    return false;
}

void NextionPanel :: copyText(char *dest, const char *src, Uint16 maxLength)
{
    Uint16 i;

    for( i = 0; i < maxLength - 1 && src[i] != 0; i++ )
    {
        dest[i] = src[i];
    }
    dest[i] = 0;
}

void NextionPanel :: sendCommand(const char *command, const char *argument)
{
    while( *command ) sendByte(*command++);
    if( argument != NULL )
    {
        sendByte(' ');
        while( *argument ) sendByte(*argument++);
    }
    sendTerminator();
}

void NextionPanel :: sendPage(Uint16 pageId)
{
    char reversed[7];
    char page[7];
    Uint16 i = 0;
    Uint16 j;

    if( pageId == 0 )
    {
        page[i++] = '0';
    }
    else
    {
        while( pageId > 0 && i < sizeof(reversed) )
        {
            reversed[i++] = (char)('0' + (pageId % 10));
            pageId /= 10;
        }

        for( j = 0; j < i; j++ )
        {
            page[j] = reversed[i - j - 1];
        }
    }

    page[i] = 0;
    sendCommand("page", page);
}

void NextionPanel :: sendText(const char *objectName, const char *text)
{
    while( *objectName ) sendByte(*objectName++);
    sendByte('.');
    sendByte('t');
    sendByte('x');
    sendByte('t');
    sendByte('=');
    sendByte('"');
    while( *text ) sendByte(*text++);
    sendByte('"');
    sendTerminator();
}

void NextionPanel :: sendNumber(const char *objectName, Uint16 value)
{
    char digits[6];
    Uint16 i = 0;
    Uint16 j;

    if( value == 0 )
    {
        digits[i++] = '0';
    }
    else
    {
        while( value > 0 && i < sizeof(digits) )
        {
            digits[i++] = (char)('0' + (value % 10));
            value /= 10;
        }
    }

    while( *objectName ) sendByte(*objectName++);
    sendByte('=');
    for( j = i; j > 0; j-- )
    {
        sendByte(digits[j - 1]);
    }
    sendTerminator();
}

void NextionPanel :: sendValue(const char *objectName, Uint16 value)
{
    char digits[6];
    Uint16 i = 0;
    Uint16 j;

    if( value == 0 )
    {
        digits[i++] = '0';
    }
    else
    {
        while( value > 0 && i < sizeof(digits) )
        {
            digits[i++] = (char)('0' + (value % 10));
            value /= 10;
        }
    }

    while( *objectName ) sendByte(*objectName++);
    sendByte('.');
    sendByte('v');
    sendByte('a');
    sendByte('l');
    sendByte('=');
    for( j = i; j > 0; j-- )
    {
        sendByte(digits[j - 1]);
    }
    sendTerminator();
}

void NextionPanel :: sendPic(const char *objectName, Uint16 imageId)
{
    char digits[6];
    Uint16 i = 0;
    Uint16 j;

    if( imageId == 0 )
    {
        digits[i++] = '0';
    }
    else
    {
        while( imageId > 0 && i < sizeof(digits) )
        {
            digits[i++] = (char)('0' + (imageId % 10));
            imageId /= 10;
        }
    }

    while( *objectName ) sendByte(*objectName++);
    sendByte('.');
    sendByte('p');
    sendByte('i');
    sendByte('c');
    sendByte('=');
    for( j = i; j > 0; j-- )
    {
        sendByte(digits[j - 1]);
    }
    sendTerminator();
}

void NextionPanel :: sendVisible(const char *objectName, bool visible)
{
    sendByte('v');
    sendByte('i');
    sendByte('s');
    sendByte(' ');
    while( *objectName ) sendByte(*objectName++);
    sendByte(',');
    sendByte(visible ? '1' : '0');
    sendTerminator();
}

void NextionPanel :: sendTerminator(void)
{
    sendByte(NEXTION_TERMINATOR);
    sendByte(NEXTION_TERMINATOR);
    sendByte(NEXTION_TERMINATOR);
}

void NextionPanel :: sendByte(Uint16 data)
{
#if NEXTION_USE_LINA
    while( LinaRegs.SCIFLR.bit.TXRDY == 0 ) {}
    LinaRegs.SCITD.bit.TD = data & 0xff;
#else
    while( SciaRegs.SCIFFTX.bit.TXFFST >= 16 ) {}
    SciaRegs.SCITXBUF.all = data & 0xff;
#endif
}

bool NextionPanel :: serialRxReady(void)
{
#if NEXTION_USE_LINA
    return LinaRegs.SCIFLR.bit.RXRDY != 0;
#else
    return SciaRegs.SCIFFRX.bit.RXFFST > 0;
#endif
}

Uint16 NextionPanel :: serialReadByte(void)
{
#if NEXTION_USE_LINA
    return LinaRegs.SCIRD.bit.RD;
#else
    return SciaRegs.SCIRXBUF.bit.SAR;
#endif
}

bool NextionPanel :: serialRxError(void)
{
#if NEXTION_USE_LINA
    return LinaRegs.SCIFLR.bit.PE || LinaRegs.SCIFLR.bit.OE ||
           LinaRegs.SCIFLR.bit.FE || LinaRegs.SCIFLR.bit.BRKDT;
#else
    return SciaRegs.SCIRXST.bit.RXERROR != 0;
#endif
}

void NextionPanel :: serialResetRxError(void)
{
#if NEXTION_USE_LINA
    LinaRegs.SCICLEARINT.bit.CLRPEINT = 1;
    LinaRegs.SCICLEARINT.bit.CLROEINT = 1;
    LinaRegs.SCICLEARINT.bit.CLRFEINT = 1;
    LinaRegs.SCICLEARINT.bit.CLRBRKDTINT = 1;
#else
    SciaRegs.SCICTL1.bit.SWRESET = 0;
    SciaRegs.SCICTL1.bit.SWRESET = 1;
#endif
}
