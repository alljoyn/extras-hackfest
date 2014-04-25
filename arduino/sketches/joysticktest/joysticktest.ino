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

#include <Joystick.h>


//const uint8_t buttonMap[]= { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
const uint8_t buttonMap[]= { 3, 4, 5, 6, 7, 8, 9 };
//const uint8_t buttonMap[]= { 7, 3, 4, 5, 6 };

Joystick js(A1, A0, 0, 996, 0, 996,
            buttonMap, sizeof(buttonMap) / sizeof(buttonMap[0]), 0);

int printnow = 0;
int printinterval = 1000;

void setup() {
  Serial.begin(9600);
  js.begin();
  js.setXRange(-16, 16);
  js.setYRange(-16, 16);
  printnow = millis() + printinterval;
}


void loop() {
  int now = millis();
  if (js.stateChanged() && (now - printnow > 0)) {
    printnow = now + printinterval;
    int i;
    int xpos = js.readXPos();
    int ypos = js.readYPos();
    int xraw = js.readRawXPos();
    int yraw = js.readRawYPos();
    unsigned short buttons = js.readButtons();
    Serial.print("x: ");
    Serial.print(xpos);
    Serial.print(" (");
    Serial.print(xraw);
    Serial.print(")   y: ");
    Serial.print(ypos);
    Serial.print(" (");
    Serial.print(yraw);
    Serial.print(")   buttons[");
    Serial.print(sizeof(buttonMap) / sizeof(buttonMap[0]));
    Serial.print("]:");
    for (i = 0; i < (sizeof(buttonMap) / sizeof(buttonMap[0])); ++i)
    {
      if (bitRead(buttons, i)) {
        Serial.write(' ');
        Serial.print(i);
        Serial.write(':');
        Serial.print(buttonMap[i]);
      }
    }
    Serial.println();
  }
}
