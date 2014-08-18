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

SMsg smsg;

#define ArraySize(a) (sizeof(a) / sizeof(a[0]))

#define CMD_BUF_SIZE 4

#define PIN_CONFIG_UNCONFIG 0x0
#define PIN_CONFIG_OUTPUT 0x1
#define PIN_CONFIG_INPUT 0x2
#define PIN_CONFIG_INPUT_PULLUP 0x3
#define PIN_CONFIG_ANALOG_OUT 0x4
#define PIN_CONFIG_ANALOG_IN 0x5

#define PIN_INTERRUPT_NO_TRIGGER 0x0
#define PIN_INTERRUPT_TRIGGER_ON_RISE 0x1
#define PIN_INTERRUPT_TRIGGER_ON_FALL 0x2

typedef struct {
  uint8_t pin;
  uint8_t config;
  uint8_t trigMode;
  uint8_t trigDebounce;
  uint16_t state;
  const char* pinName;
} PinConf;

const int aPinMap[] = { A0, A1, A2, A3, A4, A5 };
const int stateMap[] = { LOW, HIGH };

const char* aPinNames[] = { "A0", "A1", "A2", "A3", "A4", "A5" };
const char* dPinNames[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13" };

PinConf digitalPinConf[14];
PinConf analogPinConf[sizeof(aPinMap)];

void sendResponse(uint8_t cmd, PinConf* pinConf, uint16_t arg);
void processCommand(byte* buf);
void readPinCmd(PinConf* pinConf);
void writePinCmd(PinConf* pinConf, uint16_t arg);
void togglePinCmd(PinConf* pinConf);
void setInterruptCmd(PinConf* pinConf, uint16_t arg);
void configurePinCmd(PinConf* pinConf, uint16_t arg);


void setup() {
  Serial.begin(115200);
  smsg.begin();
  memset(digitalPinConf, 0, sizeof(digitalPinConf));
  memset(analogPinConf, 0, sizeof(analogPinConf));
  for (int i = 0; i < ArraySize(digitalPinConf); ++i) {
    digitalPinConf[i].pin = i;
    digitalPinConf[i].pinName = dPinNames[i];
  }
  for (int i = 0; i < ArraySize(analogPinConf); ++i) {
    analogPinConf[i].pin = aPinMap[i];
    analogPinConf[i].pinName = aPinNames[i];
  }
  Serial.println("setup complete");
}

void loop() {
  if (smsg.available()) {
    byte buf[CMD_BUF_SIZE];
    memset(buf, 255, sizeof(buf));
    int ret = smsg.read(buf, sizeof(buf));
      //Serial.print("read: ");
      //Serial.println(ret, DEC);
    if (ret < 1) {
      if (smsg.linuxRebooting()) {
        Serial.println("Linino is rebooting - everything is paused for up to 2 minutes...");
        smsg.waitLinuxBoot();
        Serial.println("reboot done");
      }
    } else if (ret == CMD_BUF_SIZE) {
      processCommand(buf);
    } else {
      //Serial.print("   buf: ");
      //Serial.print(buf[0], HEX);
      //Serial.print(" ");
      //Serial.print(buf[1], HEX);
      //Serial.print(" ");
      //Serial.print(buf[2], HEX);
      //Serial.print(" ");
      //Serial.println(buf[3], HEX);
    }
  }
  for (int pin = 0; pin < (ArraySize(digitalPinConf) + ArraySize(analogPinConf)); ++pin) {
    PinConf* pinConf;
    if (pin < sizeof(digitalPinConf)) {
      pinConf = &digitalPinConf[pin];
    } else {
      pinConf = &analogPinConf[pin - sizeof(digitalPinConf)];
    }
    int trigMode = pinConf->trigMode;
    int trigDebounce = pinConf->trigDebounce;
    if (trigMode) {
      uint16_t state = digitalRead(pinConf->pin) == HIGH ? 1 : 0;
      if (state != pinConf->state) {
        delay(trigDebounce);
        if (state != pinConf->state) {
          bool triggered = (((state == 1) && (trigMode & PIN_INTERRUPT_TRIGGER_ON_RISE)) ||
                                    ((state == 0) && (trigMode & PIN_INTERRUPT_TRIGGER_ON_FALL)));
          pinConf->state = state;
          if (triggered) {
            Serial.print("Interrupt Pin: ");
            Serial.print(pinConf->pinName);
            Serial.print(" = ");
            Serial.println(pinConf->state, DEC);
            sendResponse('i', pinConf, state ? PIN_INTERRUPT_TRIGGER_ON_RISE : PIN_INTERRUPT_TRIGGER_ON_FALL);
          }
        }
      }
    }
  }
}

void sendResponse(uint8_t cmd, PinConf* pinConf, uint16_t arg)
{
  uint8_t buf[4];
  buf[0] = cmd;
  buf[1] = pinConf->pin; // assume digital pin
  for (uint8_t i = 0; i < ArraySize(aPinMap); ++i) {
    if (pinConf->pin == aPinMap[i]) {
      buf[1] = 0xa0 + i; // fix assumption with analog pin
      break;
    }
  }
  buf[2] = arg >> 8;
  buf[3] = arg & 0xff;
  smsg.write(buf, sizeof(buf));
}

void processCommand(byte* buf)
{
  char cmd = buf[0];
  PinConf* pinConf;
  uint16_t arg = ((uint16_t)buf[2]) << 8 | buf[3];

  if ((buf[1] & 0xa0) == 0xa0) {
    pinConf = &analogPinConf[buf[1] - 0xa0];
  } else {
    pinConf = &digitalPinConf[buf[1]];
  }

  switch (cmd) {
    case 'r':
      Serial.print("Read Pin: ");
      readPinCmd(pinConf);
      break;

    case 't':
      Serial.print("Toggle Pin: ");
      togglePinCmd(pinConf);
      break;

    case 'w':
      Serial.print("Write Pin: ");
      writePinCmd(pinConf, arg);
      break;

    case 'i':
      Serial.print("Set Interrupt Pin: ");
      setInterruptCmd(pinConf, arg);
      break;

    case 'c':
      Serial.print("Configure Pin: ");
      configurePinCmd(pinConf, arg);
      break;

    default:
      Serial.println("Invalid command");
      pinConf = NULL;
      break;
  }
  if (pinConf) {
    Serial.print(pinConf->pinName);
    Serial.print(" = ");
    if (cmd == 'c' || cmd == 'i') {
      Serial.println(arg, DEC);
    } else {
      Serial.println(pinConf->state);
    }
  }
}


void readPinCmd(PinConf* pinConf)
{
  uint16_t state = 0;
  switch (pinConf->config) {
    case PIN_CONFIG_INPUT:
    case PIN_CONFIG_INPUT_PULLUP:
      state = digitalRead(pinConf->pin);
      break;

    case PIN_CONFIG_ANALOG_IN:
      state = analogRead(pinConf->pin);
      break;
  }
  pinConf->state = state;
  sendResponse('r', pinConf, state);
}

void writePinCmd(PinConf* pinConf, uint16_t arg)
{
  switch (pinConf->config) {
    case PIN_CONFIG_OUTPUT: {
        int state = arg ? 1 : 0;
        pinConf->state = state;
        digitalWrite(pinConf->pin, stateMap[state]);
        break;
      }

    case PIN_CONFIG_ANALOG_OUT:
      analogWrite(pinConf->pin, arg);
      break;
  }
}

void togglePinCmd(PinConf* pinConf)
{
  if (pinConf->config == PIN_CONFIG_OUTPUT) {
    int state = pinConf->state ? 0 : 1;
    pinConf->state = state;
    digitalWrite(pinConf->pin, stateMap[state]);
  }
}

void setInterruptCmd(PinConf* pinConf, uint16_t arg)
{
  if ((pinConf->config == PIN_CONFIG_INPUT) || (pinConf->config == PIN_CONFIG_INPUT_PULLUP)) {
    uint8_t trigMode = arg & 0x0f;
    if (trigMode) {
      pinConf->trigMode |= trigMode;
    } else {
      pinConf->trigMode = 0;
    }
    pinConf->trigDebounce = arg >> 8;
  }
}

void configurePinCmd(PinConf* pinConf, uint16_t arg)
{
  if (arg & 0x10) {
    if ((arg & 0x01) == 0) {
      // Analog input - make sure pullup is disabled to prevent funky ADC reads
      digitalWrite(pinConf->pin, LOW);
      pinMode(pinConf->pin, INPUT);
      pinConf->config = PIN_CONFIG_ANALOG_IN;
    } else {
      pinMode(pinConf->pin, OUTPUT);
      pinConf->config = PIN_CONFIG_ANALOG_OUT;
    }
  } else {
    if (arg & 0x01) {
      pinMode(pinConf->pin, OUTPUT);
      pinConf->config = PIN_CONFIG_OUTPUT;
    } else if (arg & 0x4) {
      pinMode(pinConf->pin, INPUT_PULLUP);
      pinConf->config = PIN_CONFIG_INPUT_PULLUP;
    } else {
      pinMode(pinConf->pin, INPUT);
      pinConf->config = PIN_CONFIG_INPUT;
    }
  }
}

