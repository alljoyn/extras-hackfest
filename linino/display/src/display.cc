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
#include <aj_tutorial/smsg.h>


using namespace std;

bool Valid(uint8_t x, uint8_t y)
{
    return (x < 14) && (y < 9);
}


Display::Display()
{
    ClearDisplay();
}



bool Display::ClearDisplay()
{
    memset(display, 0, sizeof(display));
    return SendDisplay();
}

bool Display::DrawPoint(uint8_t x, uint8_t y, bool on)
{
    if (!Valid(x, y)) {
        return false;
    }

    _DrawPoint(x, y, on);
    return SendDisplay();
}

const static int32_t SCALE = 1024 * 1024;

static int32_t ComputeScaledSlope(int32_t d1, int32_t d2)
{
    return (SCALE * d1) / d2;
}

static uint8_t ComputeCoordinate(int32_t slope, uint8_t c, uint8_t i1, uint8_t i2, uint8_t j1, uint8_t j2)
{
    int32_t factor = (slope < 0) ? -1 : 1;
    uint8_t a = c - min(i1, i2);
    uint8_t b = (i1 > i2) ? j2 : j1;
    return (((slope * c) + (factor * SCALE / 2)) / SCALE) + b;
}


bool Display::DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool on)
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
    return SendDisplay();
}

bool Display::DrawBox(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool on, bool fill)
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

    return SendDisplay();
}

bool Display::DrawBitmap(const uint16_t* bitmap)
{
    memcpy(display, bitmap, sizeof(display));
    return SendDisplay();
}

bool Display::SendDisplay()
{
    uint8_t buf[sizeof(display)];
    for (size_t i = 0; i < 9; ++i) {
        // MSB
        buf[2 * i] = display[i] >> 8;
        // LSB
        buf[2 * i + 1] = display[i] & 0xff;
        printf("%u: %04x  %02x %02x\n", (unsigned int)i, display[i], buf[2 * i] , buf[2 * i + 1]);
    }

    int r = smsg.Write(buf, sizeof(buf));
    printf("msg write => %d\n", r);
    return (r == sizeof(buf));
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
