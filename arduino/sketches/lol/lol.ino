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

#include <SMsg.h>
#include <LOL.h>

SMsg smsg;

long refreshtime = 0;
const long scan = 500;

void setup() {
  Serial.begin(115200);
  smsg.begin();
  LOL.begin();
  refreshtime = micros() + scan;
}


void loop() {
  uint16_t bitmap[9];
  byte buf[sizeof(bitmap)];
  if (smsg.available()) {
    int r = smsg.read(buf, sizeof(buf));
    if (r == sizeof(buf)) {
      int i;
      for (i = 0; i < 9; ++i) {
        bitmap[i] = ((uint16_t)buf[2 * i] << 8) | buf[2 * i + 1];
      }
      LOL.render(bitmap);
      Serial.println("render bitmap");
    } else {
      if (smsg.linuxRebooting()) {
        LOL.end();
        Serial.println("Linino is rebooting - everything is paused for up to 2 minutes...");
        smsg.waitLinuxBoot();
        Serial.println("reboot done");
        LOL.begin();
      } else {
        Serial.println("bad msg");
      }
    }
  }
}



