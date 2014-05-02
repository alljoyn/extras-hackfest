/**
 * @file
 * AllJoyn display test program
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

#include <pthread.h>
#include <stdio.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/Status.h>

#include <aj_tutorial/display.h>

#include "common.h"

using namespace std;
using namespace qcc;
using namespace ajn;

class AJDisplay :
    public MessageReceiver,
    public BusListener,
    public SessionListener,
    public BusAttachment::JoinSessionAsyncCB,
    public ProxyBusObject::Listener
{
  public:

    AJDisplay(BusAttachment& bus) : BusListener(), bus(bus), sessionId(0), relative(false), xSpeed(0), ySpeed(0)
    {
        //pthread_create(&handle, NULL, &AJJoystick::MovePointThread, this);
        pthread_mutex_init(&mutex, NULL);

        const InterfaceDescription* intf = bus.GetInterface(JS_INTERFACE_NAME);
        if (!intf) {
            printf("failed to create interface\n");
            _exit(1);
        }

        bus.RegisterSignalHandler(this,
                                  static_cast<MessageReceiver::SignalHandler>(&AJDisplay::PositionHandler),
                                  intf->GetMember("position"),
                                  NULL);
        bus.RegisterSignalHandler(this,
                                  static_cast<MessageReceiver::SignalHandler>(&AJDisplay::ButtonHandler),
                                  intf->GetMember("button"),
                                  NULL);
    }

    ~AJDisplay()
    {
        //pthread_cancel(handle);
        //pthread_join(handle, NULL);
        pthread_mutex_destroy(&mutex);
    }

    void PropertyCB(QStatus status, ProxyBusObject* obj, void* context) { printf("property set\n"); }


    void JoinSessionCB(QStatus status, SessionId id, const SessionOpts& opts, void* context)
    {
        if (status != ER_OK) {
            printf("failed to complete join Joystick service\n");
            return;
        }

        printf("joined session: id = %u\n", id);

        MsgArg outRange;
        MsgArg buttonMask;

        sessionId = id;

        joystick = ProxyBusObject(bus, JS_SERVICE_NAME, JS_SERVICE_PATH, sessionId);
        joystick.AddInterface(JS_INTERFACE_NAME);
        joystick.AddInterface(org::freedesktop::DBus::Properties::InterfaceName);

        outRange.Set("(nnnn)", 0, 13, 0, 8);
        //buttonMask.Set("q", BUTTON_A | BUTTON_B | BUTTON_G);
        buttonMask.Set("q", BUTTON_G);

        joystick.SetPropertyAsync(JS_INTERFACE_NAME, "output_range", outRange, this, static_cast<ajn::ProxyBusObject::Listener::SetPropertyCB>(&AJDisplay::PropertyCB), NULL);
        joystick.SetPropertyAsync(JS_INTERFACE_NAME, "button_mask", buttonMask, this, static_cast<ajn::ProxyBusObject::Listener::SetPropertyCB>(&AJDisplay::PropertyCB), NULL);
    }

    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        printf("found advertised name: %s\n", name);
        if (strcmp(name, JS_SERVICE_NAME) == 0) {
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, transport);
            QStatus status = bus.JoinSessionAsync(name, JS_SERVICE_PORT, this, opts, this);
            if (status != ER_OK) {
                printf("failed to initiate join Joystick service\n");
            }
        }
    }

    void SessionLost(SessionId sessionId, SessionLostReason reason) {
        //printf("session %u lost, quiting\n", sessionId);
        //_exit(1);
    }

    void PositionHandler(const InterfaceDescription::Member* member, const char* srcPath, Message& message)
    {
        int16_t x;
        int16_t y;
        QStatus status = message->GetArgs("nn", &x, &y);
        if (status != ER_OK) {
            return;
        }

        if (relative) {
            xSpeed = 1000 / x;
            ySpeed = 1000 / y;

        } else {
            display.ClearDisplayBuffer();
            display.DrawPoint(x, y);
        }
    }

    void ButtonHandler(const InterfaceDescription::Member* member, const char* srcPath, Message& message)
    {
        MsgArg outRange;
        uint16_t buttons;
        int16_t left, right, up, down;
        QStatus status = message->GetArgs("q", &buttons);
        if (status != ER_OK) {
            return;
        }
        if (buttons & BUTTON_A) {
            relative = true;
            left = -1;
            right = 1;
            up = -1;
            down = 1;
            xSpeed = 0;
            ySpeed = 0;
        }
        if (buttons & BUTTON_B) {
            relative = true;
            left = -10;
            right = 10;
            up = -10;
            down = 10;
            xSpeed = 0;
            ySpeed = 0;
        }
        if (buttons & BUTTON_G) {
            relative = false;
            left = 0;
            right = 13;
            up = 0;
            down = 8;
        }
        outRange.Set("(nnnn)", left, right, up, down);
        joystick.SetPropertyAsync(JS_INTERFACE_NAME, "output_range", outRange, this, static_cast<ajn::ProxyBusObject::Listener::SetPropertyCB>(&AJDisplay::PropertyCB), NULL);
    }


    static void* MovePointThread(void* arg)
    {
        AJDisplay* self = (AJDisplay*)arg;
        Display& display = self->display;
        bool done = false;

        do {
            pthread_mutex_lock(&self->mutex);
            int32_t xSpeed = self->xSpeed;
            int32_t ySpeed = self->ySpeed;
            done = 
            pthread_mutex_unlock(&self->mutex);

            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);


        } while (!done);

        return NULL;
    }

  private:
    BusAttachment& bus;
    ProxyBusObject joystick;
    SessionId sessionId;
    bool relative;
    Display display;
    int32_t xSpeed;
    int32_t ySpeed;
    pthread_t handle;
    pthread_mutex_t mutex;
};


int main(void)
{
    SetupSignalHandlers();

    BusAttachment bus("AJ Display Test", true);

    if (!CreateInterface(bus)) {
        return 1;
    }

    AJDisplay ajDisplay(bus);

    if (!SetupAllJoyn(bus, ajDisplay, NULL, NULL)) {
        return 1;
    }

    WaitForQuit();

    bus.Stop();
    bus.Join();

    return 0;
}
