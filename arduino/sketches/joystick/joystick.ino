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
  SET_X_RANGE,
  SET_Y_RANGE,
  RESET,
  INVALID
};


const uint8_t buttonMap[]= { 7, 6, 5, 4, 3, 8, 9 };
Joystick js(A1, A0, 0, 996, 0, 996,
            buttonMap, sizeof(buttonMap) / sizeof(buttonMap[0]), 0);

SMsg smsg;

void setup() {
  Serial.begin(115200);
  smsg.begin();
  js.begin();
}


void loop() {
  if (smsg.available()) {
    byte buf[CMD_BUF_SIZE];
    buf[0] = INVALID;
    int ret = smsg.read(buf, sizeof(buf));
    if (ret < 1) {
      if (smsg.linuxRebooting()) {
        js.end();
        Serial.println("Linino is rebooting - everything is paused for up to 2 minutes...");
        smsg.waitLinuxBoot();
        Serial.println("reboot done");
        js.reset();
        js.begin();
      }
      return;
    }
    processCommand(buf, ret);
  }
  if (js.stateChanged()) {
    byte buf[7];
    int x = js.readXPos();
    int y = js.readYPos();
    unsigned short b = js.readButtons();
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

void processCommand(byte* buf, uint8_t bufSize)
{
  int16_t i1 = ((int16_t)buf[1] << 8) | buf[2];
  int16_t i2 = ((int16_t)buf[3] << 8) | buf[4];
  switch (buf[0]) {
    case SET_X_RANGE:
      if (bufSize == 5) {
        Serial.println("Set X range");
        js.setXRange(i1, i2);
      }
      break;

    case SET_Y_RANGE:
      if (bufSize == 5) {
        Serial.println("Set Y range");
        js.setYRange(i1, i2);
      }
      break;

    case RESET:
      if (bufSize == 1) {
        Serial.println("Reset");
        js.reset();
      }
      break;
  }
}
