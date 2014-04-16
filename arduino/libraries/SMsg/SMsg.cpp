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

#define MAX_SEQ_NUMBER 0x3f

#define SMSG_ACK 0xc0
#define SMSG_NACK 0x80
#define SMSG_ACK_MASK (SMSG_ACK | SMSG_NACK)
#define SMSG_SEQ_MASK (~SMSG_ACK_MASK)

#define RD_TO 50

#define BAUDRATE 57600


static int waitRX(long ms)
{
    long expire = (long)millis() + ms;
    while (expire - (long)millis() > 0) {
        if (Serial1.available()) {
            return 1;
        }
    }
    return 0;
}

static void flushRX(void)
{
    while (waitRX(10)) {
        int c = Serial1.read();
    }
}


static int readTO(long to)
{
    if (waitRX(to)) {
        int c = Serial1.read();
        return c;
    }
    return -1;
}

static void _write(int c)
{
    Serial1.write(c);
}


SMsg::SMsg():
    rxseq(0),
    txseq(0)
{
}

SMsg::~SMsg()
{
}

void SMsg::begin(void)
{
    Serial1.begin(BAUDRATE);

    delay(5000);
    while (Serial1.available()) {
        Serial1.flush();
        delay(1000);
    }
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
    int pos = 0;
    int ack = SMSG_NACK | rxseq;
    int plen;
    int pseq;
    int psumbuf;
    int i;

    plen = readTO(RD_TO);
    if (plen < 0) {
        /* no flushing, no acking */
        return 0;
    }

    if (plen == '[') {
        byte c;
        // Linux kernal message -- ignore
        do {
            c = Serial1.read();
        } while ((c != '\n') && (c != '\r') && waitRX(RD_TO * 2));
        return 0;
    }

    if ((plen > MAX_MSG_LEN) || (plen > len)) {
        Serial.print("len: ");
        Serial.println(plen, HEX);
        goto exit;
    }
    sum += plen * (++pos);

    pseq = readTO(RD_TO);
    if ((pseq < -1) || (pseq != rxseq)) {
        Serial.print("seq: ");
        Serial.print(pseq, HEX);
        Serial.print(" - ");
        Serial.println(rxseq, HEX);
        if ((pseq >= 0) && (pseq <= MAX_SEQ_NUMBER)) {
            rxseq = pseq;  // assume that our rxseq value is wrong.
        }
        goto exit;
    }
    sum += pseq * (++pos);

    for (i = 0; i < plen; ++i) {
        int c;
        c = readTO(RD_TO * 2);
        if (c < 0) {
            Serial.print("payload to: ");
            Serial.print(i, DEC);
            Serial.print("/");
            Serial.println(plen, DEC);
            goto exit;
        }
        buf[i] = c;
        sum += c * (++pos);
    }

    psumbuf = readTO(RD_TO);
    if ((psumbuf < 0) || (((sum >> 8) & 0xff) != psumbuf)) {
        Serial.print("sum H: ");
        Serial.print(psumbuf, HEX);
        Serial.print(" - ");
        Serial.println((sum >> 8), HEX);
        goto exit;
    }

    psumbuf = readTO(RD_TO);
    if ((psumbuf < 0) || ((sum & 0xff) != psumbuf)) {
        Serial.print("sum L: ");
        Serial.print(psumbuf, HEX);
        Serial.print(" - ");
        Serial.println((sum & 0xff), HEX);
        goto exit;
    }

    ack |= SMSG_ACK;
    ++rxseq;
    rxseq &= MAX_SEQ_NUMBER;

exit:
    flushRX();
    _write(ack);

    return ((ack & SMSG_ACK_MASK) == SMSG_ACK) ? plen : -1;
}


int SMsg::writeMsg(const byte* buf, int len)
{
    uint16_t sum = 0;
    byte pos = 0;
    int ack;
    int i;

    _write(len);
    sum += ((len) * (++pos));

    _write(txseq);
    sum += txseq * (++pos);

    for (i = 0; i < len; ++i) {
        _write(buf[i]);
        sum += buf[i] * (++pos);
    }

    _write((sum >> 8) & 0xff);
    _write(sum & 0xff);

    ack = readTO(2 * RD_TO);

    if ((ack < 0) || ((ack & SMSG_ACK_MASK) != SMSG_ACK) || ((ack & SMSG_SEQ_MASK) != txseq)) {
        if ((ack & SMSG_ACK_MASK) == 0) {
            flushRX();
        }
        return -1;
    }

    ++txseq;
    txseq &= MAX_SEQ_NUMBER;

    return len;
}

