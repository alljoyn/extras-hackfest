/**
 * @file
 * Arduino LOL (Lots of LEDs) matrix display driver
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

#include <Arduino.h>

#include "LOL.h"


/*
 * This table maps a simple bitmap array to a render buffer that is organized
 * for efficient rendering in a periodic interrupt such that there no
 * calculations in the interrupt, just driving the digital I/O pins
 * appropriately.
 *
 * This table assumes the the bitmap buffer is 9 uint16's where the lower 14
 * bits of each uint16 indicate which LEDs of each row are lit or not.  The
 * first element in the bitmap array represents the top row of LEDs while bit
 * 11 represents the left most LED of that row and bit 0 represents the right
 * most bit of that row.
 *
 * Below is an example image with the resulting bitmap values on the right.
 *
 *         COLUMN
 *          0   1   2   3   4   5   6   7   8   9  10  11  12  13
 *        +-------+---------------+---------------+---------------+
 *  ROW 0 |            @@@         @@@             @@@            | => bitmap[0] = 0x0488;
 *        |            @@@         @@@             @@@            |
 *  ROW 1 |        @@@     @@@     @@@             @@@            | => bitmap[1] = 0x0a88;
 *        |        @@@     @@@     @@@             @@@            |
 *  ROW 2 |        @@@ @@@ @@@     @@@             @@@            | => bitmap[2] = 0x0e88;
 *        |        @@@ @@@ @@@     @@@             @@@            |
 *  ROW 3 |        @@@     @@@     @@@ @@@ @@@     @@@ @@@ @@@    | => bitmap[3] = 0x0aee;
 *        |        @@@     @@@     @@@ @@@ @@@     @@@ @@@ @@@    |
 *  ROW 4 |                                                       | => bitmap[4] = 0x0000;
 *        |                                                       |
 *  ROW 5 |    @@@         @@@     @@@     @@@     @@@         @@@| => bitmap[5] = 0x12a9;
 *        |    @@@         @@@     @@@     @@@     @@@         @@@|
 *  ROW 6 |    @@@     @@@     @@@     @@@         @@@ @@@     @@@| => bitmap[6] = 0x154d;
 *        |    @@@     @@@     @@@     @@@         @@@ @@@     @@@|
 *  ROW 7 |    @@@     @@@     @@@     @@@         @@@     @@@ @@@| => bitmap[7] = 0x154b;
 *        |    @@@     @@@     @@@     @@@         @@@     @@@ @@@|
 *  ROW 8 |@@@             @@@         @@@         @@@         @@@| => bitmap[8] = 0x2249;
 *        |@@@             @@@         @@@         @@@         @@@|
 *  ------+-------+---------------+---------------+---------------+
 *    bit  13  12  11  10   9   8   7   6   5   4   3   2   1   0
 *
 *
 * Below is a table indicating which pins when high/low will light which LEDs.
 * The top number is the pin to set high and the bottom number is which pin to
 * set low.
 *
 *         COLUMN
 *           0    1    2    3    4    5    6    7    8    9   10   11   12   13
 *        +---------+-------------------+-------------------+--------------------+
 *        |                                                                      |
 *  ROW 0 | 13   13   13   13   13   13   13   13   13    4   13    3   13    2  |
 *        |  5    6    7    8    9   10   11   12    4   13    3   13    2   13  |
 *        |                                                                      |
 *  ROW 1 | 12   12   12   12   12   12   12   12   12    4   12    3   12    2  |
 *        |  5    6    7    8    9   10   11   13    4   12    3   12    2   12  |
 *        |                                                                      |
 *  ROW 2 | 11   11   11   11   11   11   11   11   11    4   11    3   11    2  |
 *        |  5    6    7    8    9   10   12   13    4   11    3   11    2   11  |
 *        |                                                                      |
 *  ROW 3 | 10   10   10   10   10   10   10   10   10    4   10    3   10    2  |
 *        |  5    6    7    8    9   11   12   13    4   10    3   10    2   10  |
 *        |                                                                      |
 *  ROW 4 |  9    9    9    9    9    9    9    9    9    4    9    3    9    2  |
 *        |  5    6    7    8   10   11   12   13    4    9    3    9    2    9  |
 *        |                                                                      |
 *  ROW 5 |  8    8    8    8    8    8    8    8    8    4    8    3    8    2  |
 *        |  5    6    7    9   10   11   12   13    4    8    3    8    2    8  |
 *        |                                                                      |
 *  ROW 6 |  7    7    7    7    7    7    7    7    7    4    7    3    7    2  |
 *        |  5    6    8    9   10   11   12   13    4    7    3    7    2    7  |
 *        |                                                                      |
 *  ROW 7 |  6    6    6    6    6    6    6    6    6    4    6    3    6    2  |
 *        |  5    7    8    9   10   11   12   13    4    6    3    6    2    6  |
 *        |                                                                      |
 *  ROW 8 |  5    5    5    5    5    5    5    5    5    4    5    3    5    2  |
 *        |  6    7    8    9   10   11   12   13    4    5    3    5    2    5  |
 *        |                                                                      |
 *  ------+---------+-------------------+-------------------+--------------------+
 *    bit   13   12   11   10    9    8    7    6    5    4    3    2    1    0
 *
 *
 * Below is the inverse of the table immediate above this.  This shows which
 * LEDs are lit according to which pins are driven high/low at any given time.
 * The top number is the column and the bottom number is the row from the
 * previous table.
 *
 *        Low Pin
 *           13    12    11    10     9     8     7     6     5     4     3     2
 * High   +------------------------+-----------------------+-----------------------+
 * Pin    |                                                                        |
 *     13 |   -     7     6     5     4     3     2     1     0     8    10    12  |
 *        |   -     0     0     0     0     0     0     0     0     0     0     0  |
 *        |                                                                        |
 *     12 |   7     -     6     5     4     3     2     1     0     8    10    12  |
 *        |   1     -     1     1     1     1     1     1     1     1     1     1  |
 *        |                                                                        |
 *     11 |   7     6     -     5     4     3     2     1     0     8    10    12  |
 *        |   2     2     -     2     2     2     2     2     2     2     2     2  |
 *        |                                                                        |
 *     10 |   7     6     5     -     4     3     2     1     0     8    10    12  |
 *        |   3     3     3     -     3     3     3     3     3     3     3     3  |
 *        |                                                                        |
 *      9 |   7     6     5     4     -     3     2     1     0     8    10    12  |
 *        |   4     4     4     4     -     4     4     4     4     4     4     4  |
 *        |                                                                        |
 *      8 |   7     6     5     4     3     -     2     1     0     8    10    12  |
 *        |   5     5     5     5     5     -     5     5     5     5     5     5  |
 *        |                                                                        |
 *      7 |   7     6     5     4     3     2     -     1     0     8    10    12  |
 *        |   6     6     6     6     6     6     -     6     6     6     6     6  |
 *        |                                                                        |
 *      6 |   7     6     5     4     3     2     1     -     0     8    10    12  |
 *        |   7     7     7     7     7     7     7     -     7     7     7     7  |
 *        |                                                                        |
 *      5 |   7     6     5     4     3     2     1     0     -     8    10    12  |
 *        |   8     8     8     8     8     8     8     8     -     8     8     8  |
 *        |                                                                        |
 *      4 |   9     9     9     9     9     9     9     9     9     -     x     x  |
 *        |   0     1     2     3     4     5     6     7     8     -     x     x  |
 *        |                                                                        |
 *      3 |  11    11    11    11    11    11    11    11    11     x     -     x  |
 *        |   0     1     2     3     4     5     6     7     8     x     -     x  |
 *        |                                                                        |
 *      2 |  13    13    13    13    13    13    13    13    13     x     x     -  |
 *        |   0     1     2     3     4     5     6     7     8     x     x     -  |
 *        |                                                                        |
 *        +------------------------+-----------------------+-----------------------+
 *
 *
 *
 * Below is the example image mapped to render buffer according to the
 * transform tables above.  At the right is the contents of the render buffer
 * memory.  Empty cells are 0s and filled cells are 1s.
 *
 *        Low Pin
 * High    13  12  11  10   9   8   7   6   5   4   3   2
 * Pin    +---------------+---------------+---------------+
 *     13 |---     @@@         @@@                 @@@    | => renderBuf[0] = 0x0242;
 *        |---     @@@         @@@                 @@@    |
 *     12 |    --- @@@     @@@     @@@             @@@    | => renderBuf[1] = 0x02a2;
 *        |    --- @@@     @@@     @@@             @@@    |
 *     11 |    @@@ ---     @@@ @@@ @@@             @@@    | => renderBuf[2] = 0x04e2;
 *        |    @@@ ---     @@@ @@@ @@@             @@@    |
 *     10 |@@@ @@@     --- @@@     @@@         @@@ @@@ @@@| => renderBuf[3] = 0x0ca7;
 *        |@@@ @@@     --- @@@     @@@         @@@ @@@ @@@|
 *      9 |                ---                            | => renderBuf[4] = 0x0000;
 *        |                ---                            |
 *      8 |    @@@     @@@     ---     @@@     @@@ @@@    | => renderBuf[5] = 0x0516;
 *        |    @@@     @@@     ---     @@@     @@@ @@@    |
 *      7 |@@@     @@@     @@@     --- @@@         @@@    | => renderBuf[6] = 0x0a92;
 *        |@@@     @@@     @@@     --- @@@         @@@    |
 *      6 |@@@     @@@     @@@     @@@ ---         @@@ @@@| => renderBuf[7] = 0x0aa3;
 *        |@@@     @@@     @@@     @@@ ---         @@@ @@@|
 *      5 |@@@         @@@             @@@ ---     @@@    | => renderBuf[8] = 0x0912;
 *        |@@@         @@@             @@@ ---     @@@    |
 *      4 |                                    ---        | => renderBuf[9] = 0x0000;
 *        |                                    ---        |
 *      3 |            @@@         @@@             ---    | => renderBuf[10] = 0x0120;
 *        |            @@@         @@@             ---    |
 *      2 |                    @@@ @@@ @@@ @@@         ---| => renderBuf[11] = 0x0078;
 *        |                    @@@ @@@ @@@ @@@         ---|
 *  ------+---------------+---------------+---------------+
 *    bit  11  10   9   8   7   6   5   4   3   2   1   0
 */
const static uint8_t transformTable[9][14][2] = {
    { { 11,  3 }, { 11,  4 }, { 11,  5 }, { 11,  6 }, { 11,  7 }, { 11,  8 }, { 11,  9 },
      { 11, 10 }, { 11,  2 }, {  2, 11 }, { 11,  1 }, {  1, 11 }, { 11,  0 }, {  0, 11 } },
    { { 10,  3 }, { 10,  4 }, { 10,  5 }, { 10,  6 }, { 10,  7 }, { 10,  8 }, { 10,  9 },
      { 10, 11 }, { 10,  2 }, {  2, 10 }, { 10,  1 }, {  1, 10 }, { 10,  0 }, {  0, 10 } },
    { {  9,  3 }, {  9,  4 }, {  9,  5 }, {  9,  6 }, {  9,  7 }, {  9,  8 }, {  9, 10 },
      {  9, 11 }, {  9,  2 }, {  2,  9 }, {  9,  1 }, {  1,  9 }, {  9,  0 }, {  0,  9 } },
    { {  8,  3 }, {  8,  4 }, {  8,  5 }, {  8,  6 }, {  8,  7 }, {  8,  9 }, {  8, 10 },
      {  8, 11 }, {  8,  2 }, {  2,  8 }, {  8,  1 }, {  1,  8 }, {  8,  0 }, {  0,  8 } },
    { {  7,  3 }, {  7,  4 }, {  7,  5 }, {  7,  6 }, {  7,  8 }, {  7,  9 }, {  7, 10 },
      {  7, 11 }, {  7,  2 }, {  2,  7 }, {  7,  1 }, {  1,  7 }, {  7,  0 }, {  0,  7 } },
    { {  6,  3 }, {  6,  4 }, {  6,  5 }, {  6,  7 }, {  6,  8 }, {  6,  9 }, {  6, 10 },
      {  6, 11 }, {  6,  2 }, {  2,  6 }, {  6,  1 }, {  1,  6 }, {  6,  0 }, {  0,  6 } },
    { {  5,  3 }, {  5,  4 }, {  5,  6 }, {  5,  7 }, {  5,  8 }, {  5,  9 }, {  5, 10 },
      {  5, 11 }, {  5,  2 }, {  2,  5 }, {  5,  1 }, {  1,  5 }, {  5,  0 }, {  0,  5 } },
    { {  4,  3 }, {  4,  5 }, {  4,  6 }, {  4,  7 }, {  4,  8 }, {  4,  9 }, {  4, 10 },
      {  4, 11 }, {  4,  2 }, {  2,  4 }, {  4,  1 }, {  1,  4 }, {  4,  0 }, {  0,  4 }  },
    { {  3,  4 }, {  3,  5 }, {  3,  6 }, {  3,  7 }, {  3,  8 }, {  3,  9 }, {  3, 10 },
      {  3, 11 }, {  3,  2 }, {  2,  3 }, {  3,  1 }, {  1,  3 }, {  3,  0 }, {  0,  3 } }
};

#define PRESCALE_1    (1<< CS10)
#define PRESCALE_8    (1 << CS11)
#define PRESCALE_64   ((1 << CS11) | (1 << CS10))
#define PRESCALE_256  (1 << CS12)
#define PRESCALE_1024 ((1 << CS12) | (1 << CS10))

#define SCAN_INTERVAL 500 /* µs */

_LOL LOL;



typedef struct {
    uint8_t modes[4];
    uint8_t outputs[4];
} scanline_t;

static scanline_t lines[12];

static volatile uint8_t* modeReg1;
static volatile uint8_t* modeReg2;
static volatile uint8_t* modeReg3;
static volatile uint8_t* modeReg4;
static volatile uint8_t* outputReg1;
static volatile uint8_t* outputReg2;
static volatile uint8_t* outputReg3;
static volatile uint8_t* outputReg4;


ISR(TIMER1_COMPA_vect)
{
    static uint8_t line = 0;
    const uint8_t* modes = lines[line].modes;
    const uint8_t* outputs = lines[line].outputs;

    uint8_t oldSREG = SREG;
    cli();

    // Set all pins to inputs first (makes display cleaner)
    *modeReg1 &= 0x0f;
    *outputReg1 &= 0x0f;
    *modeReg2 &= 0x3f;
    *outputReg2 &= 0x3f;
    *modeReg3 &= 0x2c;
    *outputReg3 &= 0x2c;
    *modeReg4 &= 0xbf;
    *outputReg4 &= 0xbf;

    // Now set pins that need to be outputs to outputs and drive them accordingly
    *modeReg1 |= modes[0];
    *outputReg1 |= outputs[0];
    *modeReg2 |= modes[1];
    *outputReg2 |= outputs[1];
    *modeReg3 |= modes[2];
    *outputReg3 |= outputs[2];
    *modeReg4 |= modes[3];
    *outputReg4 |= outputs[3];

    SREG = oldSREG;

    ++line;
    if (line > 11) {
        line = 0;
    }
}



_LOL::_LOL()
{
}

_LOL::~_LOL()
{
}

void _LOL::begin()
{
    for (int8_t pin = 2; pin < 14; ++pin) {
        pinMode(pin, INPUT);
    }

    modeReg1 = portModeRegister(2);
    modeReg2 = portModeRegister(3);
    modeReg3 = portModeRegister(4);
    modeReg4 = portModeRegister(5);

    outputReg1 = portOutputRegister(2);
    outputReg2 = portOutputRegister(3);
    outputReg3 = portOutputRegister(4);
    outputReg4 = portOutputRegister(5);

    noInterrupts();
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;
    OCR1A = (16 * SCAN_INTERVAL) / 8;  // (16 MHz * SCAN_INTERVAL) / 8 prescale
    TCCR1B |= (1 << WGM12);
    TCCR1B |= PRESCALE_8;
    TIMSK1 |= (1 << OCIE1A);
    interrupts();
}


void _LOL::end()
{
    noInterrupts();
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;
    OCR1A = 0;
    TIMSK1 = 0;
    interrupts();

    for (int8_t pin = 2; pin < 14; ++pin) {
        pinMode(pin, INPUT);
    }
}


void _LOL::render(const uint16_t* bitmap)
{
    uint16_t renderBuf[12];
    int i;
    int j;

    memset(renderBuf, 0, sizeof(renderBuf));

    for (i = 0; i < 9; ++i) {
        for (j = 0; j < 14; ++j) {
            bitWrite(renderBuf[transformTable[i][j][0]],
                     transformTable[i][j][1],
                     bitRead(bitmap[i], 13 - j));
        }
    }

    memset(lines, 0, sizeof(lines));
    for (i = 0; i < 12; ++i) {
        uint8_t highPort = digitalPinToPort(i + 2);
        uint8_t highBit = digitalPinToBitMask(i + 2);

        lines[i].modes[highPort - 2] |= highBit;
        lines[i].outputs[highPort - 2] |= highBit;

        for (j = 0; j < 12; ++j) {
            if (i != j) {
                uint8_t lowPort = digitalPinToPort(j + 2);
                uint8_t lowBit = digitalPinToBitMask(j + 2);

                if (bitRead(renderBuf[i], j)) {
                    lines[i].modes[lowPort - 2] |= lowBit;
                }
            }
        }
    }
}
