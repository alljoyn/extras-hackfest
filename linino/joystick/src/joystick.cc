/**
 * @file
 * Arduino Joystick shield communications
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

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <aj_tutorial/joystick.h>
#if defined(HOST_BUILD)
#include <stdlib.h>
#else
#include <aj_tutorial/smsg.h>
#endif


#define CMD_BUF_SIZE 5

enum {
    JS_EVENT,
    SET_X_RANGE,
    SET_Y_RANGE,
    RESET
};

#if defined(HOST_BUILD)
static int16_t rangeLeft = 0;
static int16_t rangeRight = 1024;
static int16_t rangeUp = 0;
static int16_t rangeDown = 1024;
#endif

bool Joystick::ReadJoystick(uint16_t& buttons, int16_t& x, int16_t& y)
{
#if defined(HOST_BUILD)
    usleep(1000 * (random() % 3000 + 100));
    x = random() % (rangeRight - rangeLeft) + rangeLeft;
    y = random() % (rangeDown - rangeUp) + rangeUp;
    buttons = random() & 0xffff;
#else
    uint8_t buf[7];
    int ret;

    ret = smsg.Read(buf, sizeof(buf));
    if ((ret != sizeof(buf)) || (buf[0] != JS_EVENT)) {
        return false;
    }
    buttons = (buf[1] << 8) | buf[2];
    x = (buf[3] << 8) | buf[4];
    y = (buf[5] << 8) | buf[6];
#endif

    return true;
}

bool Joystick::SetOutputRange(int16_t left, int16_t right, int16_t up, int16_t down)
{
#if defined(HOST_BUILD)
    rangeLeft = left;
    rangeRight = right;
    rangeUp = up;
    rangeDown = down;

    return true;
#else
    return (SendSetCmd(SET_X_RANGE, left, right) &&
            SendSetCmd(SET_Y_RANGE, up, down));
#endif
}

bool Joystick::ResetRange()
{
#if defined(HOST_BUILD)
    rangeLeft = 0;
    rangeRight = 1024;
    rangeUp = 0;
    rangeDown = 1024;

    return true;
#else

    uint8_t cmd = RESET;
    return (smsg.Write(&cmd, sizeof(cmd)) != 1);
#endif
}

bool Joystick::SendSetCmd(uint8_t cmd, int16_t i1, int16_t i2)
{
#if defined(HOST_BUILD)
    return true;
#else
    uint8_t buf[CMD_BUF_SIZE];
    buf[0] = cmd;
    buf[1] = i1 >> 8;
    buf[2] = i1 & 0xff;
    buf[3] = i2 >> 8;
    buf[4] = i2 & 0xff;
    int ret = smsg.Write(buf, sizeof(buf));
    usleep(2000);
    return (ret == CMD_BUF_SIZE);
#endif
}
