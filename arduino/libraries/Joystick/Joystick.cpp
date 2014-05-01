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

static volatile uint8_t readJoystick = 0;

#define QUEUE_DEPTH 16
struct Queue {
    uint8_t q[QUEUE_DEPTH];
    uint8_t head;
    uint8_t tail;
    Queue(): head(0), tail(0) {}
    void Push(uint8_t v) { q[head++] = v; head %= QUEUE_DEPTH; if (head == tail) ++tail; tail %= QUEUE_DEPTH; }
    uint8_t Pop() { uint8_t r = q[tail]; ++tail; tail %= QUEUE_DEPTH; return r; }
    uint8_t Depth() { return head - tail + ((head < tail) ? QUEUE_DEPTH : 0); }
};

Queue q1;
Queue q2;
Queue q3;
Queue q4;

static long xSum;
static long ySum;
static int xHist[ANALOG_HISTORY];
static int yHist[ANALOG_HISTORY];
static uint8_t histPos;

static volatile uint8_t* inputReg1;
static volatile uint8_t* inputReg2;
static volatile uint8_t* inputReg3;
static volatile uint8_t* inputReg4;

ISR(TIMER1_COMPA_vect)
{
    uint8_t oldSREG = SREG;
    cli();

    q1.Push(*inputReg1);
    q2.Push(*inputReg2);
    q3.Push(*inputReg3);
    q4.Push(*inputReg4);

    SREG = oldSREG;

    readJoystick = 1;
}

Joystick::Joystick(uint8_t xPin, uint8_t yPin,
                   uint16_t xMin, uint16_t xMax,
                   uint16_t yMin, uint16_t yMax,
                   const uint8_t* buttonMap, uint8_t numButtons, uint8_t pressInd):
    x(xMin, xMax, ANALOG_MIN, ANALOG_MAX),
    y(yMin, yMax, ANALOG_MIN, ANALOG_MAX),

    oldXPos((x.outMax - x.outMin) / 2),
    oldYPos((y.outMax - y.outMin) / 2),
    xPin(xPin),
    yPin(yPin),
    buttonMap(buttonMap),
    numButtons(buttonMap ? ((numButtons < 14) ? numButtons : 14) : 0),
    pressIndNormalizer((pressInd == 0) ? (1 << numButtons) - 1 : 0),
    debounceSum(NULL)
{
    if (this->numButtons) {
        debounceSum = (uint8_t*)malloc(this->numButtons * sizeof(debounceSum[0]));
    }
}

Joystick::~Joystick()
{
}

void Joystick::begin()
{
    int i;
    for (i = 0; i < numButtons; ++i) {
        pinMode(buttonMap[i], INPUT_PULLUP);
    }

    pinMode(xPin, INPUT);
    pinMode(yPin, INPUT);

    if (debounceSum) {
        memset(debounceSum, 0, sizeof(debounceSum));
    }
    buttonState = pressIndNormalizer;

    memset(xHist, 0, sizeof(xHist));
    memset(yHist, 0, sizeof(yHist));
    histPos = 0;
    xSum = 0;
    ySum = 0;


    inputReg1 = portInputRegister(2);
    inputReg2 = portInputRegister(3);
    inputReg3 = portInputRegister(4);
    inputReg4 = portInputRegister(5);

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
    long filltime = micros() + (POLL_INTERVAL * (ANALOG_HISTORY + 2));
    while (filltime - (long)micros() > 0) {
        stateChanged();
    }

    x.calMid = (xSum + ANALOG_HISTORY / 2) / ANALOG_HISTORY;
    y.calMid = (ySum + ANALOG_HISTORY / 2) / ANALOG_HISTORY;
}

void Joystick::end()
{
    noInterrupts();
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;
    TIMSK1 == 0;
    OCR1A = 0;
    interrupts();
}



void Joystick::setXRange(int left, int right)
{
    x.setRange(left, right);
    oldXPos = x.scaleAnalog((long)oldXPos * ANALOG_HISTORY);
}

void Joystick::setYRange(int up, int down)
{
    y.setRange(down, up);
    oldYPos = y.scaleAnalog((long)oldYPos * ANALOG_HISTORY);
}

void Joystick::reset()
{
    setXRange(ANALOG_MIN, ANALOG_MAX);
    setYRange(ANALOG_MIN, ANALOG_MAX);
}


int Joystick::stateChanged(void)
{
    uint8_t oldButtonState = buttonState;
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

        while ((q1.Depth() > 0) &&
               (q2.Depth() > 0) &&
               (q3.Depth() > 0) &&
               (q4.Depth() > 0)) {
            uint8_t r[4];
            r[0] = q1.Pop();
            r[1] = q2.Pop();
            r[2] = q3.Pop();
            r[3] = q4.Pop();
            for (uint8_t b = 0; b < numButtons; ++b) {
                uint8_t port = digitalPinToPort(buttonMap[b]);
                uint8_t bit = digitalPinToBitMask(buttonMap[b]);
                if ((bit & r[port - 2]) && (debounceSum[b] < DEBOUNCE_COUNT)) {
                    ++debounceSum[b];
                } else if (debounceSum[b] > 0) {
                    --debounceSum[b];
                }
                if (debounceSum[b] == DEBOUNCE_COUNT) {
                    buttonState |= 1 << b;
                } else if (debounceSum[b] == 0) {
                    buttonState &= ~(1 << b);
                }
            }
        }
    }

    int buttonChange = (oldButtonState != buttonState);
    int xChange = (oldXPos != x.scaleAnalog(xSum));
    int yChange = (oldYPos != y.scaleAnalog(ySum));

    return buttonChange || xChange || yChange;
}



int Joystick::readXPos(void)
{
    oldXPos = x.scaleAnalog(xSum);
    return oldXPos;
}


int Joystick::readYPos(void)
{
    oldYPos = y.scaleAnalog(ySum);
    return oldYPos;
}

int Joystick::readRawXPos(void)
{
    return (xSum + ANALOG_HISTORY / 2) / ANALOG_HISTORY;
}

int Joystick::readRawYPos(void)
{
    return (ySum + ANALOG_HISTORY / 2) / ANALOG_HISTORY;
}


int Joystick::AxisInfo::scaleAnalog(long val)
{
    int calOffset;
    int calScale;
    int outOffset = outMin;
    int outScale = (outMax - outMin) / 2;

    val = (val + ANALOG_HISTORY / 2) / ANALOG_HISTORY;

    if (val < calMid) {
        calOffset = calMin;
        calScale = calMid - calMin;
    } else {
        calOffset = calMid;
        calScale = calMax - calMid;
        outOffset += (outMax - outMin) / 2;
    }

    val -= calOffset;

    /*
     * Clamp down analog value to ensure that the output is within the set
     * output range.
     */
    if (val < 0) {
        val = 0;
    } else if (val > calScale) {
        val = calScale;
    }

    if (outScale < 0) {
        return (int)((((val * outScale) - (calScale / 2)) / calScale) + outOffset);
    }
    return (int)((((val * outScale) + (calScale / 2)) / calScale) + outOffset);
}
