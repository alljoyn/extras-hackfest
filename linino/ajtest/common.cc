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

#include <signal.h>
#include <stdio.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/InterfaceDescription.h>

#include "common.h"


using namespace std;
using namespace qcc;
using namespace ajn;


const char* JS_SERVICE_NAME = "org.allseen.aj_tutorial.Joystick";
const SessionPort JS_SERVICE_PORT = 314;
const char* JS_SERVICE_PATH =  "/org/allseen/aj_tutorial/Joystick";
const char* JS_INTERFACE_NAME = "org.allseen.aj_tutorial.Joystick";


static volatile sig_atomic_t quit = false;

static void SignalHandler(int sig)
{
    quit = true;
}


void SetupSignalHandlers()
{
    struct sigaction act, oldact;
    sigset_t sigmask;

    // block all signals by default for all threads
    sigfillset(&sigmask);
    sigdelset(&sigmask, SIGSEGV);
    pthread_sigmask(SIG_BLOCK, &sigmask, NULL);

    act.sa_handler = SignalHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO | SA_RESTART;

    sigaction(SIGHUP, &act, &oldact);
    sigaction(SIGINT, &act, &oldact);
    sigaction(SIGTERM, &act, &oldact);
}

void WaitForQuit()
{
    sigset_t waitmask;

    sigfillset(&waitmask);
    sigdelset(&waitmask, SIGHUP);
    sigdelset(&waitmask, SIGINT);
    sigdelset(&waitmask, SIGTERM);

    while (!quit) {
        sigsuspend(&waitmask);
    }
}


bool CreateInterface(BusAttachment& bus)
{
    QStatus status;
    InterfaceDescription* intf = NULL;
    status = bus.CreateInterface(JS_INTERFACE_NAME, intf);
    if ((status == ER_OK) && intf) {
        intf->AddSignal("position", "nn", "x,y", 0);
        intf->AddSignal("buttons", "q", "buttons", 0);
        intf->AddProperty("output_range", "(nnnn)", PROP_ACCESS_RW);
        intf->AddProperty("button_mask", "q", PROP_ACCESS_RW);
        intf->Activate();
    } else {
        printf("failed to create interface\n");
        return false;
    }
    return true;
}


bool SetupAllJoyn(BusAttachment& bus, BusListener& bListener, BusObject* object, SessionPortListener* spListener)
{
    QStatus status;

    bus.RegisterBusListener(bListener);

    status = bus.Start();
    if (status != ER_OK) {
        printf("failed to start bus\n");
        return false;
    }

    if (object) {
        status = bus.RegisterBusObject(*object);
        if (status != ER_OK) {
            printf("failed to register bus object\n");
            return false;
        }
    }

    status = bus.Connect();
    if (status != ER_OK) {
        printf("failed to connect to the router node\n");
        return false;
    }

    if (object) {
        bus.RequestName(JS_SERVICE_NAME, DBUS_NAME_FLAG_DO_NOT_QUEUE);
        if (status != ER_OK) {
            printf("failed to request bus name\n");
            return false;
        }

        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        SessionPort sp = JS_SERVICE_PORT;
        status = bus.BindSessionPort(sp, opts, *spListener);
        if (status != ER_OK) {
            printf("failed to bind session port\n");
            return false;
        }

        bus.AdvertiseName(JS_SERVICE_NAME, TRANSPORT_ANY);
        if (status != ER_OK) {
            printf("failed to advertise bus name\n");
            return false;
        }
    } else {
        status = bus.FindAdvertisedName(JS_SERVICE_NAME);
        if (status != ER_OK) {
            printf("failed to find advertise bus name\n");
            return false;
        }
    }

    return true;
}
