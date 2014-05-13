/**
 * @file
 * AllJoyn Joystick/Display common bits
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

#ifndef _AJ_COMMON_H_
#define _AJ_COMMON_H_

#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/Session.h>
#include <alljoyn/SessionPortListener.h>


#define BUTTON_A (1 << 0)
#define BUTTON_B (1 << 1)
#define BUTTON_C (1 << 2)
#define BUTTON_D (1 << 3)
#define BUTTON_E (1 << 4)
#define BUTTON_F (1 << 5)
#define BUTTON_G (1 << 6)

#define JS_BUTTON BUTTON_C


extern const char* JS_SERVICE_NAME;
extern const ajn::SessionPort JS_SERVICE_PORT;
extern const char* JS_SERVICE_PATH;
extern const char* JS_INTERFACE_NAME;

void SetupSignalHandlers();
void WaitForQuit();
bool CreateInterface(ajn::BusAttachment& bus);
bool SetupAllJoyn(ajn::BusAttachment& bus,
                  ajn::BusListener& bListener,
                  ajn::BusObject* object,
                  ajn::SessionPortListener* spListener);

#endif
