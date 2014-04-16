/**
 * @file
 * Arduino Yun SPI communications driver - Linino side.
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

/*
 * One of the main assumptions of this protocol is that messages being sent
 * between the Arduino side and the Linino side are relatively short: less
 * than 64 bytes.
 *
 * This protocol has only 2 basic message types: a payload message and an
 * ack/nack message.
 *
 *
 * The payload message contains 4 fields:
 *
 * - Length byte.  Valid values are between 0 and 63 inclusive.  The length
 *   value excludes the length byte, the sequence byte, and the 2 byte checksum
 *   at the end.  The 2 most signficant bits must be 0 to distinguish from the
 *   ack/nack message.
 *
 * - Sequence byte.  This is just a simple incrementing number.  This allows
 *   the receiver to determine if it receives a duplicate payload message.
 *   This number rolls over at 63.  This is so that the ack/nack message can
 *   indicate the message being acknowlegded.
 *
 * - Payload.  This will consist of 0 to 63 bytes.  The meaning of the contents
 *   are defined by the code that uses this driver.
 *
 * - Checksum.  This is a simple 2 byte checksum.  The shortness of the
 *   message does not warrant the complexity of a full CRC algorithm.  The most
 *   significant byte is sent first.
 *
 *
 * The ack/nack message contains 3 fields packed into a single byte.
 *
 *  msb                         lsb
 * |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 * |   1   | ack   | sequence number                               |
 * +-------+-------+-------+-------+-------+-------+-------+-------+
 *
 * ack = 1 for success; ack = 0 for failure
 *
 *
 * Checksum Calculation:
 *
 * The checksum will be a sum of each byte multiplied by its position starting
 * with the length byte and each payload byte in sequence.  The position count
 * will start with 1 to prevent the length value from always adding 0.
 *
 *
 * Message exchange:
 *
 * The initiating side shall send payload messages.  The responding side must
 * respond with an ack/nack message within 40 ms of receiving the message.  If
 * the initiating side does not receive an ack/nack within 40 ms, it is free
 * to resend the payload message.  The receiving side will have a 20 ms timeout
 * on receiving characters so that it can detect lost characters in a message
 * and send an appropriate nack.
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <aj_tutorial/smsg.h>

#define TTY_DEV "/dev/ttyATH0"

#define MAX_SEQ_NUMBER 0x3f

#define ACK 0xc0
#define NACK 0x80
#define ACK_MASK (ACK | NACK)
#define SEQ_MASK ((~ACK_MASK) & 0xff)

#define RD_TO 50

#define BAUDRATE B57600

#if 1
#define dbg printf
#else
#define dbg(d, ...) do { } while(0)
#endif

class CheckSum {
  public:
    CheckSum() : sum(0), pos(0) { }

    void AddByte(uint8_t b) { sum += (++pos * b); }
    uint8_t GetSumMSB() const { return (sum >> 8) & 0xff; }
    uint8_t GetSumLSB() const { return sum & 0xff; }

  private:
    uint16_t sum;
    uint8_t pos;
};


SMsg::SMsg(void): txseq(0), rxseq(0), fd(open(TTY_DEV, O_RDWR | O_NONBLOCK | O_NOCTTY))
{
    if (fd < 0) {
        perror("opening " TTY_DEV);
    } else {
        struct termios tio;
        memset(&tio, 0, sizeof(tio));
        tio.c_iflag = 0;
        tio.c_oflag = 0;
        tio.c_cflag = CS8 | CREAD | CLOCAL;
        tio.c_lflag = 0;
        tio.c_cc[VMIN] = 0;
        tio.c_cc[VTIME] = 5;
        cfsetspeed(&tio, BAUDRATE);
        tcsetattr(fd, TCSANOW, &tio);
    }
}

SMsg::~SMsg(void)
{
    close(fd);
}


int SMsg::Read(uint8_t* buf, uint8_t len)
{
    int ret;

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    ret = select(fd + 1, &rfds, NULL, NULL, NULL);

    if (ret > 0) {
        ret = ReadMsg(buf, len);
    }
    return ret;
}

int SMsg::Write(const uint8_t* buf, uint8_t len)
{
    if ((len < 1) || (len > MAX_MSG_LEN)) {
        return -1;
    }
    int ret = WriteMsg(buf, len);
    return ret;
}

int SMsg::ReadMsg(uint8_t* buf, uint8_t len)
{
    uint8_t plen;
    uint8_t pseq;
    uint8_t ack = NACK | rxseq;
    uint8_t psumbuf;
    CheckSum sum;
    uint8_t i;


    if (!ReadByte(&plen)) {
        // no flushing, no acking
        return 0;
    }
    if ((plen > MAX_MSG_LEN) || (plen > len)) {
        goto exit;
    }
    sum.AddByte(plen);

    if (!ReadByte(&pseq)) {
        goto exit;
    }
    if (pseq != rxseq) {
        if (pseq <= MAX_SEQ_NUMBER) {
            rxseq = pseq;  // assume that our rxseq value is wrong.
        }
        goto exit;
    }
    sum.AddByte(pseq);

    for (i = 0; i < plen; ++i) {
        if (!ReadByte(&buf[i])) {
            goto exit;
        }
        sum.AddByte(buf[i]);
    }

    if (!ReadByte(&psumbuf)) {
        goto exit;
    }
    if (sum.GetSumMSB() != psumbuf) {
        goto exit;
    }

    if (!ReadByte(&psumbuf)) {
        goto exit;
    }
    if (sum.GetSumLSB() != psumbuf) {
        goto exit;
    }

    ack |= ACK;
    ++rxseq;
    rxseq &= MAX_SEQ_NUMBER;

exit:
    FlushRead();
    WriteByte(ack);

    return ((ack & ACK_MASK) == ACK) ? plen : -1;
}


int SMsg::WriteMsg(const uint8_t* buf, uint8_t len)
{
    CheckSum sum;
    ssize_t ret;
    uint8_t sumbuf;
    uint8_t ack = 0;
    uint8_t i;

    if (!WriteByte(len)) {
        return -1;
    }
    sum.AddByte(len);

    if (!WriteByte(txseq)) {
        return -2;
    }
    sum.AddByte(txseq);

    for (i = 0; i < len; ++i) {
        if (!WriteByte(buf[i])) {
            return -3;
        }
        sum.AddByte(buf[i]);
    }

    sumbuf = sum.GetSumMSB();
    if (!WriteByte(sumbuf)) {
        return -4;
    }

    sumbuf = sum.GetSumLSB();
    if (!WriteByte(sumbuf)) {
        return -5;
    }

    // Get the ack/nack
    WaitForMsg(); //  TO for ACK/NACK is 2x normal TO
    if (!ReadByte(&ack)) {
        return -6;
    }

    if (((ack & ACK_MASK) != ACK) || ((ack & SEQ_MASK) != txseq)) {
        if ((ack & ACK_MASK) == 0) {
            FlushRead();
        }
        printf("ack: %02x   (%02x)\n", ack, txseq);
        return -7;
    }

    ++txseq;
    txseq &= MAX_SEQ_NUMBER;

    return len;
}


bool SMsg::ReadByte(uint8_t* buf)
{
    if (WaitForMsg()) {
        int ret = read(fd, buf, 1);
        if (ret > 0) {
            return true;
        }
    }
    return false;
}

bool SMsg::WriteByte(const uint8_t buf)
{
    int ret = write(fd, &buf, 1);
    return (ret > 0);
}


void SMsg::FlushRead()
{
    uint8_t buf;

    while (WaitForMsg()) {
        read(fd, &buf, 1);
    }
}


bool SMsg::WaitForMsg()
{
    fd_set rfds;
    struct timeval to = { 0, RD_TO * 1000 };
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    return (select(fd + 1, &rfds, NULL, NULL, &to) > 0);
}
