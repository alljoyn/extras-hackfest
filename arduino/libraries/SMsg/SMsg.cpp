/**
 * @file
 * Arduino Yun SPI communications driver - Arduino side.
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

#include "SMsg.h"

#define RD_TO 500

#define BAUDRATE 250000

static char waitRX(long ms)
{
    long expire = (long)micros() + ms;
    while (expire - (long)micros() > 0) {
        if (Serial1.available()) {
            return 1;
        }
    }
    return 0;
}


static void _write(int c)
{
    Serial1.write(c);
}


SMsg::SMsg()
{
}

SMsg::~SMsg()
{
}

void SMsg::begin(void)
{
    Serial1.begin(BAUDRATE);
}


int SMsg::read(byte* buf, int len)
{
    int ret;
    do {
        ret = readMsg(buf, len);
    } while (ret == 0);
    return ret;
}

int SMsg::write(const byte* buf, int len)
{
    if ((len < 1) || (len > MAX_MSG_LEN)) {
        return 0;
    }

    int ret = writeMsg(buf, len);
    return ret;
}

int SMsg::readMsg(byte* buf, int len)
{
    uint16_t sum = 0;
    uint8_t pos = 0;
    int plen;
    int psumbuf;
    uint8_t i;

    plen = readTO(RD_TO);
    if (plen < 0) {
        /* no flushing */
        return 0;
    }

    if (plen == '[') {
        char c;
        // Linux kernal message -- ignore
        flushRX();
        return 0;
    }

    if ((plen > MAX_MSG_LEN) || (plen > len)) {
        return -1;
    }
    sum += plen * (++pos);

    for (i = 0; i < plen; ++i) {
        int c;
        c = readTO(RD_TO * 2);
        if (c < 0) {
            return -1;
        }
        buf[i] = c;
        sum += c * (++pos);
    }

    psumbuf = readTO(RD_TO);
    if ((psumbuf < 0) || (((sum >> 8) & 0xff) != psumbuf)) {
        return -1;
    }

    psumbuf = readTO(RD_TO);
    if ((psumbuf < 0) || ((sum & 0xff) != psumbuf)) {
        return -1;
    }

exit:
    flushRX();

    return plen;
}


int SMsg::writeMsg(const byte* buf, int len)
{
    uint16_t sum = 0;
    byte pos = 0;
    int i;

    _write(len);
    sum += ((len) * (++pos));

    for (i = 0; i < len; ++i) {
        _write(buf[i]);
        sum += buf[i] * (++pos);
    }

    _write((sum >> 8) & 0xff);
    _write(sum & 0xff);

    delayMicroseconds(RD_TO * 2);

    return len;
}

int SMsg::readTO(long to)
{
    if (waitRX(to)) {
        int c = Serial1.read();
        detectReboot(c);
        return c;
    }
    return -1;
}

void SMsg::flushRX(void)
{
    while (waitRX(RD_TO)) {
        int c = Serial1.read();
        detectReboot(c);
    }
}


#define updatePos(_c, _p, _s) (_p) = (((_s)[(_p)] == (_c)) ? ((_p) + 1) : 0)
#define checkBoot(_p, _s) ((_p) == sizeof(_s) - 1)

void SMsg::detectReboot(uint8_t c)
{
    static const char bootStr1[] = "Arduino Yun (ar9331) U-boot";
    static const char bootStr2[] = "Top of RAM usable for U-Boot at: 84000000";
    static const char bootStr3[] = "Now running in RAM - U-Boot at: 83fdc000";
    static const char bootStr4[] = "Hit any key to stop autoboot";
    static uint8_t mPos1 = 0;
    static uint8_t mPos2 = 0;
    static uint8_t mPos3 = 0;
    static uint8_t mPos4 = 0;

    updatePos(c, mPos1, bootStr1);
    updatePos(c, mPos2, bootStr2);
    updatePos(c, mPos3, bootStr3);
    updatePos(c, mPos4, bootStr4);

    if (checkBoot(mPos1, bootStr1) ||
        checkBoot(mPos2, bootStr2) ||
        checkBoot(mPos3, bootStr3) ||
        checkBoot(mPos4, bootStr4)) {
        rebooting = 1;
    }
}

void SMsg::waitLinuxBoot()
{
    static const char bootDoneStr[] = "--- BOOT DONE ---";
    uint8_t mPos = 0;
    long to = millis() + 120000;  // Linux boots should take no longer than 2 minutes.
    while (((long)millis() - to < 0) && (mPos < sizeof(bootDoneStr) - 1)) {
        if (Serial1.available()) {
            char c = Serial1.read();
            Serial.print(c);
            if (bootDoneStr[mPos] == c) {
                ++mPos;
            } else {
                mPos = 0;
            }
        }
    }
    to = millis() + 100;
    while (((long)millis() - to < 0)) {
        if (Serial1.available()) {
            Serial.print((char)Serial1.read());
            to = millis() + 100;
        }
    }
    rebooting = 0;
}
