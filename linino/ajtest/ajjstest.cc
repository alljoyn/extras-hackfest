/**
 * @file
 * AllJoyn Joystick test program
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
#include <alljoyn/Status.h>

#include <aj_tutorial/joystick.h>

#include "common.h"

using namespace std;
using namespace qcc;
using namespace ajn;


class AJJoystick :
    public BusObject,
    public BusListener,
    public SessionListener,
    public SessionPortListener
{
  public:

    AJJoystick(BusAttachment& bus) :
        BusObject(JS_SERVICE_PATH),
        BusListener(),
        bus(bus),
        buttonEvent(NULL),
        joystickEvent(NULL),
        sessionId(0),
        buttonMask(0x7f),
        left(-1),
        right(1),
        up(-1),
        down(1)
    {
        const InterfaceDescription* intf = bus.GetInterface(JS_INTERFACE_NAME);
        if (!intf) {
            printf("failed to create interface\n");
            _exit(1);
        }

        QStatus status = AddInterface(*intf);
        if (status != ER_OK) {
            printf("failed to add interface to bus object\n");
            _exit(1);
        }

        buttonEvent = intf->GetMember("buttons");
        joystickEvent = intf->GetMember("position");
        assert(buttonEvent);
        assert(joystickEvent);

        pthread_create(&handle, NULL, &AJJoystick::JoystickThread, this);
        pthread_mutex_init(&mutex, NULL);
    }

    ~AJJoystick()
    {
        pthread_cancel(handle);
        pthread_join(handle, NULL);
        pthread_mutex_destroy(&mutex);
    }

    void SessionLost(SessionId id, SessionLostReason reason) {
        printf("session %u lost\n", id);
        if (sessionId == id) {
            sessionId = 0;
        }
    }

    void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner)
    {
        printf("session joined by %s: id = %u\n", joiner, id);
        sessionId = id;
    }

    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
    {
        printf("join session from %s on port %u\n", joiner, sessionPort);
        if (sessionPort != JS_SERVICE_PORT) {
            return false;
        }
        return true;
    }

    SessionId GetSessionId() const { return sessionId; }

  protected:
    QStatus Get(const char* ifcName, const char* propName, MsgArg& val)
    {
        QStatus status = ER_OK;
        if (strcmp(propName, "output_range") == 0) {
            pthread_mutex_lock(&mutex);
            val.Set("(nnnn)", left, right, up, down);
            pthread_mutex_unlock(&mutex);
        } else if (strcmp(propName, "button_mask") == 0) {
            pthread_mutex_lock(&mutex);
            val.Set("q", buttonMask);
            pthread_mutex_unlock(&mutex);
        } else {
            status = ER_BUS_NO_SUCH_PROPERTY;
        }
        return status;
    }

    QStatus Set(const char* ifcName, const char* propName, MsgArg& val)
    {
        QStatus status = ER_OK;
        if (strcmp(propName, "output_range") == 0) {
            int16_t l, r, u, d;
            val.Get("(nnnn)", &l, &r, &u, &d);
            joystick.SetOutputRange(l, r, u, d);
            pthread_mutex_lock(&mutex);
            left = l;
            right = r;
            up = u;
            down = down;
            pthread_mutex_unlock(&mutex);
            printf("output_range = %d, %d, %d, %d\n", l, r, u, d);
            
        } else if (strcmp(propName, "button_mask") == 0) {
            pthread_mutex_lock(&mutex);
            val.Get("q", &buttonMask);
            printf("button_mask = %04x\n", buttonMask);
            pthread_mutex_unlock(&mutex);
        } else {
            status = ER_BUS_NO_SUCH_PROPERTY;
        }
        return status;
    }

  private:
    BusAttachment& bus;
    const InterfaceDescription::Member* buttonEvent;
    const InterfaceDescription::Member* joystickEvent;
    SessionId sessionId;
    Joystick joystick;
    uint16_t buttonMask;
    int16_t left, right, up, down;
    pthread_t handle;
    pthread_mutex_t mutex;

    void SendButtonEvent(uint16_t buttons)
    {
        pthread_mutex_lock(&mutex);
        SessionId id = sessionId;
        pthread_mutex_unlock(&mutex);
        
        if (id) {
            MsgArg b;
            b.Set("q", buttons);
            Signal(NULL, sessionId, *buttonEvent, &b, 1);
            //printf("sent button event: %x\n", buttons);
        }
    }

    void SendJoystickEvent(int16_t x, int16_t y)
    {
        pthread_mutex_lock(&mutex);
        SessionId id = sessionId;
        pthread_mutex_unlock(&mutex);
        
        if (id) {
            MsgArg pos[2];
            size_t posSize = 2;
            MsgArg::Set(pos, posSize, "nn", x, y);
            Signal(NULL, sessionId, *joystickEvent, pos, 2);
            printf("sent joystick event: %d, %d\n", x, y);
        }
    }

    static void* JoystickThread(void* arg)
    {
        printf("joystick thread started\n");
        AJJoystick* self = (AJJoystick*) arg;
        Joystick& joystick = self->joystick;
        int16_t oldX = 0;
        int16_t oldY = 0;
        uint16_t oldButtons = 0;

        while (true) {
            uint16_t buttons;
            int16_t x, y;
            if (joystick.ReadJoystick(buttons, x, y)) {
                pthread_mutex_lock(&self->mutex);
                uint16_t mask = self->buttonMask;
                pthread_mutex_unlock(&self->mutex);
                if ((buttons ^ oldButtons) & mask) {
                    self->SendButtonEvent(buttons & mask);
                    oldButtons = buttons;
                }
                if (x != oldX || y != oldY) {
                    self->SendJoystickEvent(x, y);
                    oldX = x;
                    oldY = y;
                }
            }
        }
        return NULL;
    }
};


int main(void)
{
    SetupSignalHandlers();

    BusAttachment bus("AJ Joystick Test", true);

    if (!CreateInterface(bus)) {
        return 1;
    }

    AJJoystick ajJoystick(bus);

    if (!SetupAllJoyn(bus, ajJoystick, &ajJoystick, &ajJoystick)) {
        return 1;
    }

    WaitForQuit();

    bus.Stop();
    bus.Join();

    return 0;
}
