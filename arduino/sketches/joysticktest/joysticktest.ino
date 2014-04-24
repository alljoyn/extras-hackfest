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


//const int buttonMap[]= { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
//const int buttonMap[]= { 3, 4, 5, 6, 7, 8, 9 };
const int buttonMap[]= { 7, 3, 4, 5, 6 };

Joystick js(A1, A0, buttonMap, sizeof(buttonMap) / sizeof(buttonMap[0]), 0);

void setup() {
  Serial.begin(9600);
  js.begin();
  //js.setXCal(0, 994, 510);
  //js.setYCal(994, 0, 492);
  //js.setXRange(-16, 16);
  //js.setYRange(-16, 16);
}

int printnow = 0;

void loop() {
  int now = millis();
  if (js.stateChanged() && (now >= printnow)) {
    printnow = now + 1000;
    int i;
    int xpos = js.readXPos();
    int ypos = js.readYPos();
    //int xraw = js.readRawXPos();
    //int yraw = js.readRawYPos();
    unsigned short buttons = js.readButtons();
    Serial.print("x: ");
    Serial.print(xpos);
    Serial.print(" (");
    //Serial.print(xraw);
    Serial.print(")   y: ");
    Serial.print(ypos);
    Serial.print(" (");
    //Serial.print(yraw);
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
    //js.PrintScale();
  }
}
