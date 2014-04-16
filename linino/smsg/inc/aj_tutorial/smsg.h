/**
 * @file
 * Arduino Yun SPI communications driver.
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


class SMsg {
  public:
    static const uint8_t MAX_MSG_LEN = 63;

    SMsg();
    ~SMsg();

    /**
     * Read a message from the Arduino side of the SPI bus.
     *
     * @param[out] buf  Pointer to a buffer to store the message payload.
     * @param[in]  len  Size of the buffer for storing the message payload.
     *
     * @return  The actual number of bytes read or -1 on error.
     */
    int Read(uint8_t* buf, uint8_t len);

    /**
     * Send a message to the Arduino side of the SPI bus.
     *
     * @param buf   Pointer to a buffer with the message payload to send.
     * @param len   Size of the message payload to send.
     *
     * @return  The actual number of bytes sent or -1 on error.
     */
    int Write(const uint8_t* buf, uint8_t len);

    /**
     * Get access to the underlying file descriptor used to communicate with
     * the joystick driver sketch running on the Arduino.  Only use this file
     * descriptor with select() or epoll().  Use the methods in this class for
     * actual communication.
     */
    int GetFD() const { return fd; }

  private:
    uint8_t txseq;
    uint8_t rxseq;
    int fd;

    int ReadMsg(uint8_t* buf, uint8_t len);
    int WriteMsg(const uint8_t* buf, uint8_t len);
    bool ReadByte(uint8_t* buf);
    bool WriteByte(const uint8_t buf);
    void FlushRead();
    bool WaitForMsg();
};



#endif
