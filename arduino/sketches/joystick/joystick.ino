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
#include <SMsg.h>

#define CMD_BUF_SIZE 5

enum {
  JS_EVENT,
  SET_X_CAL,
  SET_Y_CAL,
  SET_X_RANGE,
  SET_Y_RANGE,
  GET_X_CAL,
  GET_Y_CAL,
  GET_X_RANGE,
  GET_Y_RANGE,
  RESET,
  INVALID
};


const int buttonMap[]= { 3, 4, 5, 6, 7, 8, 9 };
Joystick js(A1, A0, buttonMap, sizeof(buttonMap) / sizeof(buttonMap[0]), 0);

SMsg smsg;

int respond(byte* buf, int i1, int i2)
{
  buf[1] = i1 >> 8;
  buf[2] = i1 & 0xff;
  buf[3] = i2 >> 8;
  buf[4] = i2 & 0xff;
  return smsg.write(buf, CMD_BUF_SIZE);
}  


void setup() {
  //Serial.begin(9600);
  smsg.begin();
  js.begin();
}


void loop() {
  if (smsg.available()) {
    processCommand();
  }
  if (js.stateChanged()) {
    //Serial.print("JS event ");
    byte buf[7];
    int x = js.readXPos();
    int y = js.readYPos();
    unsigned short b = js.readButtons();
    //Serial.print(x, DEC);
    //Serial.print(" ");
    //Serial.print(y, DEC);
    //Serial.print(" ");
    //Serial.println(b, HEX);
    buf[0] = JS_EVENT;
    buf[1] = b >> 8;
    buf[2] = b & 0xff;
    buf[3] = x >> 8;
    buf[4] = x & 0xff;
    buf[5] = y >> 8;
    buf[6] = y & 0xff;
    smsg.write(buf, sizeof(buf));
  }
}

void processCommand(void)
{
  byte buf[CMD_BUF_SIZE];
  buf[0] = INVALID;
  int ret = smsg.read(buf, sizeof(buf));
  int i1 = ((int)buf[1] << 8) | buf[2];
  int i2 = ((int)buf[3] << 8) | buf[4];
  switch (buf[0]) {
    case SET_X_CAL:
      if (ret == 5) {
        Serial.println("Set X cal");
        js.setXCal(i1, i2);
      }
      break;

    case SET_Y_CAL:
      if (ret == 5) {
        Serial.println("Set Y cal");
        js.setYCal(i1, i2);
      }
      break;

    case SET_X_RANGE:
      if (ret == 5) {
        Serial.println("Set X range");
        js.setXRange(i1, i2);
      }
      break;

    case SET_Y_RANGE:
      if (ret == 5) {
        Serial.println("Set Y range");
        js.setYRange(i1, i2);
      }
      break;

    case GET_X_CAL:
      if (ret == 1) {
        Serial.println("Get X cal");
        respond(buf, js.getXCalLeft(), js.getXCalRight());
      }
      break;

    case GET_Y_CAL:
      if (ret == 1) {
        Serial.println("Get Y cal");
        respond(buf, js.getYCalUp(), js.getYCalDown());
      }
      break;

    case GET_X_RANGE:
      if (ret == 1) {
        Serial.println("Get X range");
        respond(buf, js.getXOutLeft(), js.getXOutRight());
      }
      break;

    case GET_Y_RANGE:
      if (ret == 1) {
        Serial.println("Get Y range");
        respond(buf, js.getYOutUp(), js.getYOutDown());
      }
      break;

    case RESET:
      if (ret == 1) {
        Serial.println("Reset");
        js.reset();
      }
      break;
  }
}
