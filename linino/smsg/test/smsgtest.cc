/**
 * @file
 * SMsg test
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <aj_tutorial/smsg.h>

#define ITERATIONS 10000

void DumpBuf(const char* name, const uint8_t* buf, size_t len)
{
    size_t i;
    printf("%s:", name);
    for (i = 0; i < len; ++i) {
        printf(" %02x", buf[i]);
    }
    printf("\n");
}

void invert(uint8_t* buffer, int size)
{
  while (size > 0) {
    --size;
    buffer[size] = ~buffer[size];
  }
}

int main(void)
{
    SMsg smsg;
    uint8_t txbuf[SMsg::MAX_MSG_LEN + 10];
    uint8_t rxbuf[sizeof(txbuf)];
    uint32_t i;
    int ret;

    for (i = 0; i < sizeof(txbuf); ++i) {
        printf("\rFill %u: ", i); fflush(stdout);
        txbuf[i] = (i & 0xff);
        memset(rxbuf, 0, sizeof(rxbuf));
        if (i > 0) {
            ret = smsg.Write(txbuf, i);
            if ((ret < 0) && (i <= SMsg::MAX_MSG_LEN)) {
                printf("Failed to send %u bytes: %d\n", i, ret);
                sleep(1);
            } else if ((ret > 0) && (i > SMsg::MAX_MSG_LEN)) {
                printf("Sent too large buffer: %d bytes\n", ret);
                sleep(1);
            }
            if ((ret > 0) && (i <= SMsg::MAX_MSG_LEN)) {
                invert(txbuf, i);
                ret = smsg.Read(rxbuf, sizeof(rxbuf));
                if (ret < 0) {
                    printf("Failed to read message: %d\n", ret);
                } else if (ret == 0) {
                    printf("Read timed out\n");
                } else if ((ret > 0) && (memcmp(txbuf, rxbuf, ret) != 0)) {
                    printf("Data received does not match data sent.\n");
                    DumpBuf("tx", txbuf, i);
                    DumpBuf("rx", rxbuf, ret);
                }
                invert(txbuf, i);
            }
        }
    }

    for (i = 0; i < ITERATIONS; ++i) {
        printf("\rIter %u: ", i); fflush(stdout);
        *(uint32_t*)txbuf = i;
        memset(rxbuf, 0, sizeof(rxbuf));
        ret = smsg.Write(txbuf, SMsg::MAX_MSG_LEN);
        if (ret < 0) {
            printf("Failed to send sequence %u\n", i);
            sleep(1);
            continue;
        }
        invert(txbuf, SMsg::MAX_MSG_LEN);
        ret = smsg.Read(rxbuf, sizeof(rxbuf));
        if (ret < SMsg::MAX_MSG_LEN) {
            printf("Failed to receive message sequence %u: %d\n", i, ret);
        } else if ((ret > 0) && (memcmp(txbuf, rxbuf, ret) != 0)) {
            printf("Data received does not match data sent for sequence %u\n", i);
            DumpBuf("tx", txbuf, SMsg::MAX_MSG_LEN);
            DumpBuf("rx", rxbuf, ret);
        }
        invert(txbuf, i);
    }
    printf("Done\n");

    return 0;
}
