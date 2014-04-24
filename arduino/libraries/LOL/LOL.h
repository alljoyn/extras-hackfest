/**
 * @file
 * Arduino LOL shield driver library.
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

#ifndef _LOL_DRIVER_H_
#define _LOL_DRIVER_H_

#include <Arduino.h>

class _LOL
{
  public:
    _LOL();
    ~_LOL();

    /**
     * Must be called in setup() to register display interrupt.
     */
    void begin();

    /**
     * Thsi stops the display interrupt.  This must be called when the Linux
     * side is rebooting otherwise the Linux side may hang during boot.
     */
    void end();

    /**
     * Renders the bitmap onto the LOL display.  The bitmap is an array of 9
     * uint16's.  Each uint16 represents one rout of LEDs.  Only the lower 14
     * bits of each uint16 is relevant for display.  The upper 2 bits are
     * ignored.  bitmap[0] represents the top row while bitmap[8] represents
     * the bottom row.  Bit 0 represents the right most column while bit 13
     * represents the left most column.  Orientation is based on digital I/O
     * pins are at the top of the board and the analog pins are at the bottom
     * of the board.
     *
     * @param bitmap    Bitmap image to be displayed.
     */
    void render(const uint16_t* bitmap);
};

extern _LOL LOL;

#endif
