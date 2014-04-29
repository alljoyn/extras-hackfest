/**
 * @file
 * Arduino LOL shield communications
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

#include <algorithm>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <aj_tutorial/display.h>

#if !defined(HOST_BUILD)
#include <aj_tutorial/smsg.h>
#endif


#if defined(HOST_BUILD)
#define dbg 1
#else
#define dbg 0
#endif

using namespace std;

bool Valid(uint8_t x, uint8_t y)
{
    return (x < 14) && (y < 9);
}


Display::Display()
{
    ClearDisplay();
}



bool Display::ClearDisplayBuffer()
{
    memset(display, 0, sizeof(display));
    return true;
}

bool Display::DrawPointBuffer(uint8_t x, uint8_t y, bool on)
{
    if (!Valid(x, y)) {
        return false;
    }

    _DrawPoint(x, y, on);
    return true;
}

const static int32_t SCALE = 1024 * 1024;

static int32_t ComputeScaledSlope(int32_t d1, int32_t d2)
{
    return (SCALE * d1) / d2;
}

static uint8_t ComputeCoordinate(int32_t slope, uint8_t c, uint8_t i1, uint8_t i2, uint8_t j1, uint8_t j2)
{
    int32_t factor = (slope < 0) ? -1 : 1;
    uint8_t b = (i1 > i2) ? j2 : j1;
    return (((slope * c) + (factor * SCALE / 2)) / SCALE) + b;
}


bool Display::DrawLineBuffer(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool on)
{
    if (!Valid(x1, y1) || !Valid(x2, y2)) {
        return false;
    }

    uint8_t x;
    uint8_t y;

    if (abs(x2 - x1) > abs(y2 - y1)) {
        // stick to integer math - Yuns don't have FP HW
        int32_t slope = ComputeScaledSlope((int32_t)y2 - y1, (int32_t)x2 - x1);
        for (x = min(x1, x2); x <= max(x1, x2); ++x) {
            y = ComputeCoordinate(slope, x, x1, x2, y1, y2);
            _DrawPoint(x, y, on);
        }
    } else {
        // stick to integer math - Yuns don't have FP HW
        int32_t slope = ComputeScaledSlope((int32_t)x2 - x1, (int32_t)y2 - y1);
        for (y = min(y1, y2); y <= max(y1, y2); ++y) {
            x = ComputeCoordinate(slope, y, y1, y2, x1, x2);
            _DrawPoint(x, y, on);
        }
    }
    return true;
}

bool Display::DrawBoxBuffer(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool on, bool fill)
{
    if (!Valid(x1, y1) || !Valid(x2, y2)) {
        return false;
    }

    uint16_t topbottom = 0;
    uint16_t sides = (1 << x1) | (1 << x2);
    uint8_t x;
    uint8_t y;

    for (x = min(x1, x2); x <= max(x1, x2); ++x) {
        topbottom |= 1 << x;
    }

    if (fill) {
        sides = topbottom;
    }

    if (on) {
        display[y1] |= topbottom;
        display[y2] |= topbottom;
    } else {
        topbottom = ~topbottom;
        sides = ~sides;
        display[y1] &= topbottom;
        display[y2] &= topbottom;
    }

    for (y = min(y1, y2) + 1; y <= max(y1, y2) - 1; ++y) {
        if (on) {
            display[y] |= sides;
        } else {
            display[y] &= sides;
        }
    }

    return true;
}

bool Display::DrawBitmapBuffer(const uint16_t* bitmap)
{
    memcpy(display, bitmap, sizeof(display));
    return true;
}


bool Display::DrawScoreBoardBuffer(uint8_t left, uint8_t right, bool highlightLeft, bool highlightRight)
{
    /*
     *  +-------+
     *  |       |
     *  | x xxx |
     *  | x x x |
     *  | x x x |
     *  | x xxx |
     *  | x x x |
     *  | x x x |
     *  | x xxx |
     *  |       |
     *  +-------+
     */
    static const uint16_t score[20][9] = {
        { 0x0000, 0x000e, 0x000a, 0x000a, 0x000a, 0x000a, 0x000a, 0x000e, 0x0000 }, //  0
        { 0x0000, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0000 }, //  1
        { 0x0000, 0x000e, 0x0002, 0x0002, 0x000e, 0x0008, 0x0008, 0x000e, 0x0000 }, //  2
        { 0x0000, 0x000e, 0x0002, 0x0002, 0x000e, 0x0002, 0x0002, 0x000e, 0x0000 }, //  3
        { 0x0000, 0x000a, 0x000a, 0x000a, 0x000e, 0x0002, 0x0002, 0x0002, 0x0000 }, //  4
        { 0x0000, 0x000e, 0x0008, 0x0008, 0x000e, 0x0002, 0x0002, 0x000e, 0x0000 }, //  5
        { 0x0000, 0x000e, 0x0008, 0x0008, 0x000e, 0x000a, 0x000a, 0x000e, 0x0000 }, //  6
        { 0x0000, 0x000e, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0000 }, //  7
        { 0x0000, 0x000e, 0x000a, 0x000a, 0x000e, 0x000a, 0x000a, 0x000e, 0x0000 }, //  8
        { 0x0000, 0x000e, 0x000a, 0x000a, 0x000e, 0x0002, 0x0002, 0x000e, 0x0000 }, //  9
        { 0x0000, 0x002e, 0x002a, 0x002a, 0x002a, 0x002a, 0x002a, 0x002e, 0x0000 }, // 10
        { 0x0000, 0x0022, 0x0022, 0x0022, 0x0022, 0x0022, 0x0022, 0x0022, 0x0000 }, // 11
        { 0x0000, 0x002e, 0x0022, 0x0022, 0x002e, 0x0028, 0x0028, 0x002e, 0x0000 }, // 12
        { 0x0000, 0x002e, 0x0022, 0x0022, 0x002e, 0x0022, 0x0022, 0x002e, 0x0000 }, // 13
        { 0x0000, 0x002a, 0x002a, 0x002a, 0x002e, 0x0022, 0x0022, 0x0022, 0x0000 }, // 14
        { 0x0000, 0x002e, 0x0028, 0x0028, 0x002e, 0x0022, 0x0022, 0x002e, 0x0000 }, // 15
        { 0x0000, 0x002e, 0x0028, 0x0028, 0x002e, 0x002a, 0x002a, 0x002e, 0x0000 }, // 16
        { 0x0000, 0x002e, 0x0022, 0x0022, 0x0022, 0x0022, 0x0022, 0x0022, 0x0000 }, // 17
        { 0x0000, 0x002e, 0x002a, 0x002a, 0x002e, 0x002a, 0x002a, 0x002e, 0x0000 }, // 18
        { 0x0000, 0x002e, 0x002a, 0x002a, 0x002e, 0x0022, 0x0022, 0x002e, 0x0000 }  // 19
    };
    uint16_t leftBitmap[9];
    uint16_t rightBitmap[9];

    left %= 20;
    right %= 20;
    memcpy(leftBitmap, score[left], sizeof(leftBitmap));
    memcpy(rightBitmap, score[right], sizeof(rightBitmap));
    for (int i = 0; i < 9; ++i) {
        if (highlightRight) {
            rightBitmap[i] ^= 0x7f;
        }
        if (highlightLeft) {
            leftBitmap[i] ^= 0x7f;
        }
        display[i] = (leftBitmap[i] << 7) | rightBitmap[i];
    }
}



bool Display::SendDisplay()
{
    uint8_t buf[sizeof(display)];
    if (dbg) printf("        +--------------+\n");
    for (size_t i = 0; i < 9; ++i) {
        // MSB
        buf[2 * i] = display[i] >> 8;
        // LSB
        buf[2 * i + 1] = display[i] & 0xff;
        if (dbg) printf("%u: %04x |", (unsigned int)i, display[i]);
        if (dbg) for (int j = 0; j < 14; ++j) {
            printf("%c", (display[i] & (1 << (13 - j))) ? '*' : ' ');
        }
        if (dbg) printf("| %02x %02x\n", buf[2 * i] , buf[2 * i + 1]);
    }
    if (dbg) printf("        +--------------+\n");

#if defined(HOST_BUILD)
    return true;
#else
    int r = smsg.Write(buf, sizeof(buf));
    return (r == sizeof(buf));
#endif
}

void Display::_DrawPoint(uint8_t x, uint8_t y, bool on)
{
    assert(x < 14);
    assert(y < 9);
    if (on) {
        display[y] |= 1 << (13 - x);
    } else {
        display[y] &= ~(1 << (13 - x));
    }
}
