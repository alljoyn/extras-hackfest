/**
 * @file
 * Arduino Joystick shield driver.
 */

/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include "Joystick.h"

#define POLL_INTERVAL 500 /* µs */
#define DEBOUNCE_TIME 5  /* ms */
#define DEBOUNCE_COUNT (((DEBOUNCE_TIME * 1000) + (POLL_INTERVAL / 2)) / POLL_INTERVAL)

#define ANALOG_HISTORY 16
#define ANALOG_MIN 0
#define ANALOG_MAX 1024

#define PRESCALE_1    (1<< CS10)
#define PRESCALE_8    (1 << CS11)
#define PRESCALE_64   ((1 << CS11) | (1 << CS10))
#define PRESCALE_256  (1 << CS12)
#define PRESCALE_1024 ((1 << CS12) | (1 << CS10))


static const int* _buttonMap = NULL;
static int _numButtons;
static byte* debounceSum = NULL;

static volatile unsigned short buttonState = 0;
static volatile int readJoystick = 0;

static long xSum;
static long ySum;
static int xHist[ANALOG_HISTORY];
static int yHist[ANALOG_HISTORY];
static int histPos;


ISR(TIMER1_COMPA_vect)
{
    int i;

    buttonState = 0;
    for (i = 0; i < _numButtons; ++i) {
        byte val = digitalRead(_buttonMap[i]);
        if (val && (debounceSum[i] < DEBOUNCE_COUNT)) {
            ++debounceSum[i];
        } else if (!val && (debounceSum[i] > 0)) {
            --debounceSum[i];
        }
        if (debounceSum[i] == DEBOUNCE_COUNT) {
            buttonState |= 1 << i;
        } else if (debounceSum[i] == 0) {
            buttonState &= ~(1 << i);
        }
    }
    readJoystick = 1;
}

static int scaleAnalog(long val, int calScale, int calOffset, int outScale, int outOffset)
{
    val = (val + ANALOG_HISTORY / 2) / ANALOG_HISTORY;
    val -= calOffset;

    /*
     * Clamp down analog value to ensure that the output is within the range
     * specified in the constuctor
     */
    if (calScale < 0) {
        // swapping directions
        if (val > 0) {
            val = 0;
        } else if (val < calScale) {
            val = calScale;
        }
    } else {
        if (val < 0) {
            val = 0;
        } else if (val > calScale) {
            val = calScale;
        }
    }

    return (int)((((val * outScale) + (calScale / 2)) / calScale) + outOffset);
}


Joystick::Joystick(int xPin, int yPin, const int* buttonMap, int numButtons, int pressInd):
    xCalOffset(ANALOG_MIN),
    xCalScale(ANALOG_MAX),
    xOutOffset(xCalOffset),
    xOutScale(xCalScale),

    yCalOffset(ANALOG_MIN),
    yCalScale(ANALOG_MAX),
    yOutOffset(yCalOffset),
    yOutScale(yCalScale),

    oldButtonState((pressInd == 0) ? (1 << numButtons) - 1 : 0),
    oldXPos(xOutScale / 2 + xOutOffset),
    oldYPos(yOutScale / 2 + yOutOffset),
    xPin(xPin),
    yPin(yPin),
    pressIndNormalizer((pressInd == 0) ? (1 << numButtons) - 1 : 0)
{
    _buttonMap = buttonMap;
    _numButtons = (numButtons < 14) ? numButtons : 14;
    if (!_buttonMap) {
        _numButtons = 0;
    }
    if (_numButtons) {
        debounceSum = (byte*)malloc(_numButtons * sizeof(debounceSum[0]));
    }
}

Joystick::~Joystick()
{
}

void Joystick::begin()
{
    int i;
    for (i = 0; i < _numButtons; ++i) {
        pinMode(_buttonMap[i], INPUT);
    }

    pinMode(xPin, INPUT);
    pinMode(yPin, INPUT);

    if (debounceSum) {
        memset(debounceSum, 0, sizeof(debounceSum));
    }
    buttonState = 0;

    memset(xHist, 0, sizeof(xHist));
    memset(yHist, 0, sizeof(yHist));
    histPos = 0;
    xSum = 0;
    ySum = 0;

    noInterrupts();
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;
    TCCR1B |= (1 << WGM12);
    TCCR1B |= PRESCALE_8;
    TIMSK1 |= (1 << OCIE1A);
    OCR1A = (16 * POLL_INTERVAL) / 8;  // (16 MHz * POLL_INTERVAL) / 8 prescale
    interrupts();

    // Fill analog capture history
    delay((POLL_INTERVAL * (ANALOG_HISTORY + 2) + 500) / 1000);
}


void Joystick::setXCal(int left, int right)
{
    xCalOffset = left;
    xCalScale = right - left;
    oldXPos = scaleAnalog((long)oldXPos * ANALOG_HISTORY, xCalScale, xCalOffset, xOutScale, xOutOffset);
}

void Joystick::setYCal(int up, int down)
{
    yCalOffset = up;
    yCalScale = down - up;
    oldYPos = scaleAnalog((long)oldYPos * ANALOG_HISTORY, yCalScale, yCalOffset, yOutScale, yOutOffset);
}

void Joystick::setXRange(int left, int right)
{
    xOutOffset = left;
    xOutScale = right - left;
    oldXPos = scaleAnalog((long)oldXPos * ANALOG_HISTORY, xCalScale, xCalOffset, xOutScale, xOutOffset);
}

void Joystick::setYRange(int up, int down)
{
    yOutOffset = up;
    yOutScale = down - up;
    oldYPos = scaleAnalog((long)oldYPos * ANALOG_HISTORY, yCalScale, yCalOffset, yOutScale, yOutOffset);
}

void Joystick::reset()
{
     xCalOffset = ANALOG_MIN;
     xCalScale = ANALOG_MAX;
     xOutOffset = xCalOffset;
     xOutScale = xCalScale;

     yCalOffset = ANALOG_MIN;
     yCalScale = ANALOG_MAX;
     yOutOffset = yCalOffset;
     yOutScale = yCalScale;
}


int Joystick::stateChanged(void)
{
    int buttonChange = (oldButtonState != buttonState);

    if (readJoystick) {
        readJoystick = 0;
        int x = analogRead(xPin);
        int y = analogRead(yPin);
        
        xSum += x - xHist[histPos];
        ySum += y - yHist[histPos];
        xHist[histPos] = x;
        yHist[histPos] = y;
        ++histPos;
        histPos %= ANALOG_HISTORY;
    }


    int xChange = (oldXPos != scaleAnalog(xSum, xCalScale, xCalOffset, xOutScale, xOutOffset));
    int yChange = (oldYPos != scaleAnalog(ySum, yCalScale, yCalOffset, yOutScale, yOutOffset));

    return buttonChange || xChange || yChange;
}


unsigned short Joystick::buttonsChanged(void)
{
    unsigned short changed = buttonState ^ oldButtonState;
    oldButtonState = buttonState;
    return changed;
}


unsigned short Joystick::readButtons(void)
{
    oldButtonState = buttonState;
    return pressIndNormalizer ^ oldButtonState;
}


int Joystick::readXPos(void)
{
    oldXPos = scaleAnalog(xSum, xCalScale, xCalOffset, xOutScale, xOutOffset);
    return oldXPos;
}


int Joystick::readYPos(void)
{
    oldYPos = scaleAnalog(ySum, yCalScale, yCalOffset, yOutScale, yOutOffset);
    return oldYPos;
}
