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

#include <aj_tutorial/spicom.h>

#define TTY_DEV "/dev/ttySPI0"

#define MAX_SEQ_NUMBER 0x3f

#define ACK 0xc0
#define NACK 0x80
#define ACK_MASK (ACK | NACK)
#define SEQ_MASK ((~ACK_MASK) & 0xff)

#define RD_TO 20

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


SPICom::SPICom(void): txseq(0), rxseq(0), fd(open(TTY_DEV, O_RDWR | O_NONBLOCK | O_NOCTTY))
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
        tcsetattr(fd, TCSANOW, &tio);
    }
}

SPICom::~SPICom(void)
{
    close(fd);
}


int SPICom::Read(uint8_t* buf, uint8_t len)
{
    int ret;

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    ret = select(fd + 1, &rfds, NULL, NULL, NULL);

    if (ret > 0) {
        printf("RD:");
        ret = ReadMsg(buf, len);
        printf("\n");
    }
    return ret;
}

int SPICom::Write(const uint8_t* buf, uint8_t len)
{
    if ((len < 1) || (len > MAX_MSG_LEN)) {
        return -1;
    }
    printf("WR:");
    int ret = WriteMsg(buf, len);
    printf("\n");
    return ret;
}

int SPICom::ReadMsg(uint8_t* buf, uint8_t len)
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
        //printf("error: plen = %02x\n", plen);
        goto exit;
    }
    sum.AddByte(plen);

    if (!ReadByte(&pseq)) {
        //printf("error: read pseq\n");
        goto exit;
    }
    if (pseq != rxseq) {
        //printf("error: pseq = %02x  (rxseq = %02x)\n", pseq, rxseq);
        if (pseq <= MAX_SEQ_NUMBER) {
            rxseq = pseq;  // assume that our rxseq value is wrong.
        }
        goto exit;
    }
    sum.AddByte(pseq);

    for (i = 0; i < plen; ++i) {
        if (!ReadByte(&buf[i])) {
            //printf("error: read payload byte %d\n", i);
            goto exit;
        }
        sum.AddByte(buf[i]);
    }

    if (!ReadByte(&psumbuf)) {
        //printf("error: read sum high\n");
        goto exit;
    }
    if (sum.GetSumMSB() != psumbuf) {
        //printf("error: high sum = %d  (%02x)\n", psumbuf, sum.GetSumMSB());
        goto exit;
    }

    if (!ReadByte(&psumbuf)) {
        //printf("error: read sum low\n");
        goto exit;
    }
    if (sum.GetSumLSB() != psumbuf) {
        //printf("error: low sum = %02x  (%02x)\n", psumbuf, sum.GetSumLSB());
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


int SPICom::WriteMsg(const uint8_t* buf, uint8_t len)
{
    CheckSum sum;
    ssize_t ret;
    uint8_t sumbuf;
    uint8_t ack = 0;
    uint8_t i;

    sum.AddByte(len);
    if (!WriteByte(len)) {
        return -1;
    }

    sum.AddByte(txseq);
    if (!WriteByte(txseq)) {
        return -1;
    }

    for (i = 0; i < len; ++i) {
        sum.AddByte(buf[i]);
        if (!WriteByte(buf[i])) {
            return -1;
        }
    }

    sumbuf = sum.GetSumMSB();
    if (!WriteByte(sumbuf)) {
        return -1;
    }
    
    sumbuf = sum.GetSumLSB();
    if (!WriteByte(sumbuf)) {
        return -1;
    }

    // Get the ack/nack
    WaitForMsg(); //  TO for ACK/NACK is 2x normal TO
    if (!ReadByte(&ack)) {
        return -1;
    }

    if (((ack & ACK_MASK) != ACK) || ((ack & SEQ_MASK) != txseq)) {
        if ((ack & ACK_MASK) == 0) {
            FlushRead();
        }
        return -1;
    }

    ++txseq;
    txseq &= MAX_SEQ_NUMBER;

    return len;
}


bool SPICom::ReadByte(uint8_t* buf)
{
    if (WaitForMsg()) {
        int ret = read(fd, buf, 1);
        if (ret > 0) {
            printf(" r%02x", *buf);
            return true;
        }
    }
    return false;
}

bool SPICom::WriteByte(const uint8_t buf)
{
    printf(" w%02x", buf);
    int ret = write(fd, &buf, 1);
    return (ret > 0);
}


void SPICom::FlushRead()
{
    uint8_t buf;

    while (WaitForMsg()) {
        read(fd, &buf, 1);
        printf(" f%02x", buf);
    }
}


bool SPICom::WaitForMsg()
{
    fd_set rfds;
    struct timeval to = { 0, RD_TO * 1000 };
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    return (select(fd + 1, &rfds, NULL, NULL, &to) > 0);
}
