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

#ifndef _SMSG_H_
#define _SMSG_H_

#include "Arduino.h"

class SMsg
{
  public:
    static const int MAX_MSG_LEN = 31;

    SMsg();
    ~SMsg();

    /**
     * This must be called in setup().
     */
    void begin();

    void waitLinuxBoot();
    byte linuxRebooting() { return rebooting; }

    int available() { return Serial1.available(); }

    /**
     * This reads a message in to buf provided that the message payload is
     * less than or equal to len in size.  The memory pointed to by buf must
     * be at least len in size.  If the message size is too large the sender
     * will receive a NACK response.
     *
     * @param buf      Buffer to hold the message payload
     * @param len      Size of the buffer (and largest acceptable payload)
     *
     * @returns  number of bytes read on success, -1 otherwise (contents of buf may be altered)
     */
    int read(byte* buf, int len);

    /**
     * This writes a message using the contents of buf as the payload.
     *
     * @param buf      Buffer with the message payload
     * @param len      Number of bytes to send
     *
     * @returns  number of bytes sent on success, -1 otherwise
     */
    int write(const byte* buf, int len);

  private:
    byte rebooting;

    int readMsg(byte* buf, int len);
    int writeMsg(const byte* buf, int len);
    int readTO(long to);
    void flushRX(void);
    void detectReboot(uint8_t c);

};


#endif
