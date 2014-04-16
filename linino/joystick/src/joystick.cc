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

#include <aj_tutorial/joystick.h>
#include <aj_tutorial/smsg.h>

#define CMD_BUF_SIZE 5

enum {
    JS_EVENT,
    SET_X_CAL,
    SET_Y_CAL,
    SET_X_RANGE,
    SET_Y_RANGE,
    GET_X_CAL,
    GET_Y_CAL,
    GET_X_RANGE,
    GET_Y_RANGE,
    RESET
};


bool Joystick::ReadJoystick(uint16_t& buttons, int16_t& x, int16_t& y)
{
    uint8_t buf[7];
    int ret;

    ret = smsg.Read(buf, sizeof(buf));
    if ((ret != sizeof(buf)) || (buf[0] != JS_EVENT)) {
        return false;
    }
    buttons = (buf[1] << 8) | buf[2];
    x = (buf[3] << 8) | buf[4];
    y = (buf[5] << 8) | buf[6];

    return true;
}

bool Joystick::SetCal(int16_t left, int16_t right, int16_t up, int16_t down)
{
    return SendSetCmd(SET_X_CAL, left, right) && SendSetCmd(SET_Y_CAL, up, down);
}

bool Joystick::GetCal(int16_t& left, int16_t& right, int16_t& up, int16_t& down)
{
    return SendGetCmd(GET_X_CAL, left, right) && SendGetCmd(GET_Y_CAL, up, down);
}

bool Joystick::SetOut(int16_t left, int16_t right, int16_t up, int16_t down)
{
    return SendSetCmd(SET_X_RANGE, left, right) && SendSetCmd(SET_Y_RANGE, up, down);
}

bool Joystick::GetOut(int16_t& left, int16_t& right, int16_t& up, int16_t& down)
{
    return SendGetCmd(GET_X_RANGE, left, right) && SendGetCmd(GET_Y_RANGE, up, down);
}

bool Joystick::ResetCal()
{
    uint8_t cmd = RESET;
    return (smsg.Write(&cmd, sizeof(cmd)) != 1);
}

bool Joystick::SendSetCmd(uint8_t cmd, int16_t i1, int16_t i2)
{
    uint8_t buf[CMD_BUF_SIZE];
    buf[0] = cmd;
    buf[1] = i1 >> 8;
    buf[2] = i1 & 0xff;
    buf[3] = i2 >> 8;
    buf[4] = i2 & 0xff;
    return (smsg.Write(buf, sizeof(buf)) == CMD_BUF_SIZE);
}

bool Joystick::SendGetCmd(uint8_t cmd, int16_t& i1, int16_t& i2)
{
    if (smsg.Write(&cmd, sizeof(cmd)) == 1) {
        uint8_t buf[CMD_BUF_SIZE];
        if ((smsg.Read(buf, sizeof(buf)) == CMD_BUF_SIZE) && (buf[0] == cmd)) {
            i1 = ((int)buf[1] << 8) | buf[2];
            i2 = ((int)buf[3] << 8) | buf[4];
            return true;
        }
    }
    return false;
}
