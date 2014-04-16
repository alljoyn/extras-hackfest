/**
 * @file
 * SPICom test
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
#include <sys/select.h>
#include <unistd.h>

#include <aj_tutorial/display.h>

#define msleep(x) usleep((x) * 1000)

static const uint16_t alljoynBitmap[] = {
    0x0488, //  |   @  @   @   |
    0x0a88, //  |  @ @ @   @   |
    0x0e88, //  |  @@@ @   @   |
    0x0aee, //  |  @ @ @@@ @@@ |
    0x0000, //  |              |
    0x12a9, //  | @  @ @ @ @  @|
    0x154d, //  | @ @ @ @  @@ @|
    0x154b, //  | @ @ @ @  @ @@|
    0x2249  //  |@   @  @  @  @|
};

int main(void)
{
    Display display;
    uint8_t x;
    uint8_t y;

    printf("Draw ALLJOYN\n");
    display.DrawBitmap(alljoynBitmap);

    msleep(2000);
    printf("Draw points on\n");
    display.ClearDisplay();

    for (y = 0; y < 9; ++y) {
        for (x = 0; x < 14; x += (y + 1)) {
            display.DrawPoint(x, y);
            msleep(200);
        }
    }

    msleep(2000);
    printf("Draw lines\n");
    display.ClearDisplay();

    for (x = 0; x < 9; x += 2) {
        display.DrawLine(x, 0, 8 - x, 8);
        msleep(200);
        display.DrawLine(x, 0, 8 - x, 8, false);
    }
    for (y = 0; y < 9; y += 2) {
        display.DrawLine(8, y, 0, 8 - y);
        msleep(200);
        display.DrawLine(8, y, 0, 8 - y, false);
    }
    for (x = 0; x < 9; x += 2) {
        display.DrawLine(8 - x, 8, x, 0);
        msleep(200);
    }
    for (y = 0; y < 9; y += 2) {
        display.DrawLine(0, 8 - y, 8, y);
        msleep(200);
    }

    msleep(2000);
    printf("Draw boxes\n");
    display.ClearDisplay();

    display.DrawBox(0, 0, 13, 8, true, false);
    msleep(200);
    display.DrawBox(2, 2, 11, 6, true, true);
    msleep(200);
    display.DrawBox(0, 0, 13, 8, true, true);
    display.DrawBox(0, 0, 13, 8, false, false);
    msleep(200);
    display.DrawBox(2, 2, 11, 6, false, true);
    msleep(200);


    msleep(2000);
    display.DrawBox(0, 0, 13, 8);

    printf("Draw points off\n");
    for (y = 0; y < 9; ++y) {
        for (x = 0; x < 14; x += (y + 1)) {
            display.DrawPoint(x, y, false);
            msleep(200);
        }
    }

    msleep(2000);
    printf("Done\n");
    display.ClearDisplay();


    return 0;
}
