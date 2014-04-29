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

#define RD_TO 50

#define BAUDRATE B230400


#if !defined(HOST_BUILD)
#define TTY_DEV "/dev/ttyATH0"
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


SMsg::SMsg(void): fd(-1)
{
#if !defined(HOST_BUILD)
    fd = open(TTY_DEV, O_RDWR | O_NONBLOCK | O_NOCTTY);
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
#endif

    while (WaitForMsg(10 * RD_TO)) {
        FlushRead();
    }
}

SMsg::~SMsg(void)
{
    if (fd > 0) {
        close(fd);
    }
}


int SMsg::Read(uint8_t* buf, uint8_t len)
{
    int ret = -1;

    if (fd > 0) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        ret = select(fd + 1, &rfds, NULL, NULL, NULL);

        if (ret > 0) {
            ret = ReadMsg(buf, len);
        }
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
    uint8_t psumbuf;
    CheckSum sum;
    uint8_t i;


    if (!ReadByte(&plen)) {
        return 0;
    }
    if ((plen > MAX_MSG_LEN) || (plen > len)) {
        return -1;
    }
    sum.AddByte(plen);

    for (i = 0; i < plen; ++i) {
        if (!ReadByte(&buf[i])) {
            return -1;
        }
        sum.AddByte(buf[i]);
    }

    if (!ReadByte(&psumbuf)) {
        return -1;
    }
    if (sum.GetSumMSB() != psumbuf) {
        return -1;
    }

    if (!ReadByte(&psumbuf)) {
        return -1;
    }
    if (sum.GetSumLSB() != psumbuf) {
        return -1;
    }

    FlushRead();

    return plen;
}


int SMsg::WriteMsg(const uint8_t* buf, uint8_t len)
{
    CheckSum sum;
    uint8_t sumbuf;
    uint8_t i;

    if (!WriteByte(len)) {
        return -1;
    }
    sum.AddByte(len);

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

    usleep(RD_TO * 2 * 1000);

    return len;
}


bool SMsg::ReadByte(uint8_t* buf)
{
    if (WaitForMsg(RD_TO)) {
        if (fd > 0) {
            ssize_t ret = read(fd, buf, 1);
            if (ret > 0) {
                //printf(" r%02x", *buf); fflush(stdout);
                return true;
            }
        }
    }
    return false;
}

bool SMsg::WriteByte(const uint8_t buf)
{
    if (fd > 0) {
        //printf(" w%02x", buf); fflush(stdout);
        ssize_t ret = write(fd, &buf, 1);
        return (ret > 0);
    }
    return false;
}


void SMsg::FlushRead()
{
    uint8_t buf;

    while (WaitForMsg(RD_TO)) {
        if (fd > 0) {
            ssize_t ret = read(fd, &buf, 1);
            ret++; // suppress compiler warning.
            //printf(" f%02x", buf); fflush(stdout);
        }
    }
}


bool SMsg::WaitForMsg(uint32_t timeout)
{
    if (fd > 0) {
        fd_set rfds;
        struct timeval to = { 0, timeout * 1000 };
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        return (select(fd + 1, &rfds, NULL, NULL, &to) > 0);
    }
    return false;
}
