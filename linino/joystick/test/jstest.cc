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

#include <aj_tutorial/joystick.h>

//#define RED_BUTTON    (1 << 3)
//#define GREEN_BUTTON  (1 << 4)
#define RED_BUTTON    (0)
#define GREEN_BUTTON  (1 << 3)
#define BLUE_BUTTON   (1 << 5)
#define YELLOW_BUTTON (1 << 6)
#define JS_BUTTON     (1 << 2)


int main(void)
{
    Joystick js;
    int ret;
    bool done = false;
    uint16_t oldbuttons = 0;

    while (!done) {
        uint16_t buttons;
        int16_t x;
        int16_t y;
        if (js.ReadJoystick(buttons, x, y)) {
            printf("x: %d   y: %d   buttons:", x, y);
            if (buttons & JS_BUTTON) {
                printf(" Joystick");
            }
            if (buttons & RED_BUTTON) {
                printf(" Red");
            }
            if (buttons & GREEN_BUTTON) {
                printf(" Green");
            }
            if (buttons & BLUE_BUTTON) {
                printf(" Blue");
            }
            if (buttons & YELLOW_BUTTON) {
                printf(" Yellow");
            }
            printf("\n");

            if ((buttons & JS_BUTTON) && !(oldbuttons & JS_BUTTON)) {
                done = true;
            }
            if ((buttons & RED_BUTTON) && !(oldbuttons & RED_BUTTON)) {
                js.SetCal(15, 995, 995, 15);
            }
            if ((buttons & GREEN_BUTTON) && !(oldbuttons & GREEN_BUTTON)) {
                js.SetOut(-16, 16, -32, 32);
            }
            if ((buttons & BLUE_BUTTON) && !(oldbuttons & BLUE_BUTTON)) {
                js.ResetCal();
                    
            }
            if ((buttons & YELLOW_BUTTON) && !(oldbuttons & YELLOW_BUTTON)) {
                int16_t left;
                int16_t right;
                int16_t up;
                int16_t down;
                js.GetCal(left, right, up, down);
                printf("Calibration:  left = %d   right = %d   up = %d   down = %d\n", left, right, up, down);
                js.GetOut(left, right, up, down);
                printf("Output range: left = %d   right = %d   up = %d   down = %d\n", left, right, up, down);
            }
            oldbuttons = buttons;
        } else {
            printf("JS read failed\n");
        }
    }

    return 0;
}
