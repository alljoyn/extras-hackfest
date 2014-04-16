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

SMsg smsg;

void setup() {
  Serial.begin(9600);
  smsg.begin();
}

void invert(byte* buffer, int size)
{
  while (size > 0) {
    --size;
    buffer[size] = ~buffer[size];
  }
}

void loop() {
  if (smsg.available()) {
    byte buffer[SMsg::MAX_MSG_LEN];
    int ret;
    memset(buffer, 0, sizeof(buffer));
    ret = smsg.read(buffer, sizeof(buffer));
    Serial.print("ret = ");
    Serial.println(ret, DEC);
    if (ret > 0) {
      invert(buffer, ret);
      delay(500);
      smsg.write(buffer, ret);
    }
  }
}

