/**
 * @file
 * Arduino LOL shield communications
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

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <stdint.h>

#if !defined(HOST_BUILD)
#include <aj_tutorial/smsg.h>
#endif

class Display
{
  public:
    Display();
    ~Display() { }

#if !defined(HOST_BUILD)
    /**
     * Get access to the underlying file descriptor used to communicate with
     * the LOL driver sketch running on the Arduino.  Only use this file
     * descriptor with select() or epoll().  Use the methods in this class for
     * actual communication.
     *
     * @return  file descriptor
     */
    int GetFD() const { return smsg.GetFD(); }
#endif

    /**
     * Clear the display (all LEDs off).
     *
     * @return  true if successfully cleared, false otherwise (communication error)
     */
    bool ClearDisplayBuffer();
    bool ClearDisplay() { return ClearDisplayBuffer() && SendDisplay(); }

    /**
     * Turn an individual LED on or off.
     *
     * @param x     X coordinate to turn on/off
     * @param y     y coordinate to turn on/off
     * @param on    whether to turn the LED on or off (default = on)
     *
     * @return  true if successfully cleared, false otherwise (bad coordinate or communication error)
     */
    bool DrawPointBuffer(uint8_t x, uint8_t y, bool on = true);
    bool DrawPoint(uint8_t x, uint8_t y, bool on = true) { return DrawPointBuffer(x, y, on) && SendDisplay(); }

    /**
     * Turns a line of LEDs on or off.
     *
     * @param x1    X coordinate of the start of the line to turn on/off
     * @param y1    y coordinate of the start of the line to turn on/off
     * @param x2    X coordinate of the end of the line to turn on/off
     * @param y2    y coordinate of the end of the line to turn on/off
     * @param on    whether to turn the LED on or off (default = on)
     *
     * @return  true if successfully cleared, false otherwise (bad coordinate or communication error)
     */
    bool DrawLineBuffer(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool on = true);
    bool DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool on = true)
    {
        return DrawLineBuffer(x1, y1, x2, y2, on) && SendDisplay();
    }

    /**
     * Turns a box of LEDs on or off.
     *
     * @param x1    X coordinate of one corner the box to turn on/off
     * @param y1    y coordinate of one corner the box to turn on/off
     * @param x2    X coordinate of the opposite corner of the box to turn on/off
     * @param y2    y coordinate of the opposite corner of the box to turn on/off
     * @param on    whether to turn the LED on or off (default = on)
     * @param fill  whether to draw just the outline of the box or the full box (default = full box)
     *
     * @return  true if successfully cleared, false otherwise (bad coordinate or communication error)
     */
    bool DrawBoxBuffer(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool on = true, bool fill = true);
    bool DrawBox(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool on = true, bool fill = true)
    {
        return DrawBoxBuffer(x1, y1, x2, y2, on, fill) && SendDisplay();
    }

    /**
     * Sends a bitmap directly to the LOL shield for display.
     *
     * @param bitmap    Array of 9 uint16_t's with the bitmap to display where
     *                  bitmap[0] is the top line, bitmap[8] is the bottom line
     *                  and bit 0 is the right most edge and bit 13 is the left
     *                  most edge.
     *
     * @return  true if successfully cleared, false otherwise (communication error)
     */
    bool DrawBitmapBuffer(const uint16_t* bitmap);
    bool DrawBitmap(const uint16_t* bitmap) { return DrawBitmapBuffer(bitmap) && SendDisplay(); }

    /**
     * Send the display buffer to the Arduino side for display.
     */
    bool SendDisplay();

  private:
#if !defined(HOST_BUILD)
    SMsg smsg;
#endif
    uint16_t display[9];

    void _DrawPoint(uint8_t x, uint8_t y, bool on);
};

#endif
