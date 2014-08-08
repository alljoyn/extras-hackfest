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

#include "MyFirstBusObject.h"
#include <alljoyn/InterfaceDescription.h>
#include <alljoyn/ProxyBusObject.h>
#include <iostream>
#include <iomanip>
#include <stdio.h>

using namespace ajn;
using namespace qcc;

static const char * MY_FIRST_OBJECT_PATH = "/my/first/busobject/communicate";
static const char * MY_FIRST_INTERFACE_NAMES[] = {"org.example.my.first.alljoyn.interface.communicate"};
static const uint32_t MY_NUMBER_OF_INTERFACES = 1;
static const char * MY_FIRST_ADD_MATCH_RULE = "type='signal',interface='org.example.my.first.alljoyn.interface.communicate'";

MyFirstBusObject::MyFirstBusObject(BusAttachment& mBusAttachment) : BusObject(MY_FIRST_OBJECT_PATH)
{
    QStatus status;
    InterfaceDescription* myFirstBusObjectIntf = NULL;
    if (!mBusAttachment.GetInterface(MY_FIRST_INTERFACE_NAMES[0])) {
        mBusAttachment.CreateInterface(MY_FIRST_INTERFACE_NAMES[0], myFirstBusObjectIntf);
        assert(myFirstBusObjectIntf);

        //Add BusMethods
        myFirstBusObjectIntf->AddMethod("Tell", "s", "s", "thought,reply", 0);
        myFirstBusObjectIntf->AddSignal("Share", "s", "thought", 0);// on a session
        myFirstBusObjectIntf->AddSignal("Broadcast", "s", "thought", 0);

        myFirstBusObjectIntf->Activate();
    }

    /* Add the service interface to this object */
    const InterfaceDescription* myFirstBusObjectTestIntf = mBusAttachment.GetInterface(MY_FIRST_INTERFACE_NAMES[0]);
    assert(myFirstBusObjectTestIntf);
    AddInterface(*myFirstBusObjectTestIntf);

    /* Set the local methods to which BusMethod linkage */
    const MethodEntry methodEntries[] = {
        { myFirstBusObjectTestIntf->GetMember("Tell"), static_cast<MessageReceiver::MethodHandler>(&MyFirstBusObject::handleTell) },
    };
    AddMethodHandlers(methodEntries, sizeof(methodEntries)/sizeof(methodEntries[0])); 

    /* Register the signal handlers */
    shareMember = myFirstBusObjectTestIntf->GetMember("Share");
    broadcastMember = myFirstBusObjectTestIntf->GetMember("Broadcast");
    assert(shareMember);
    mBusAttachment.RegisterSignalHandler(this,
        static_cast<MessageReceiver::SignalHandler>(&MyFirstBusObject::shareHandler),
        shareMember,
        NULL);

    assert(broadcastMember);
    status =  mBusAttachment.RegisterSignalHandler(this,
        static_cast<MessageReceiver::SignalHandler>(&MyFirstBusObject::broadcastHandler),
        broadcastMember,
        NULL);

    /* Make addMatch calls to complete the registration with the AllJoyn router */
    mBusAttachment.AddMatch(MY_FIRST_ADD_MATCH_RULE);
}

MyFirstBusObject::~MyFirstBusObject()
{
    bus->UnregisterAllHandlers(this);    
}

const uint32_t MyFirstBusObject::getNumberOfInterfaces()
{
    return MY_NUMBER_OF_INTERFACES;
}

const char * MyFirstBusObject::getInterfaceName(uint32_t i)
{
    if( i < MY_NUMBER_OF_INTERFACES) {
        return MY_FIRST_INTERFACE_NAMES[i];
    }
    return NULL;
} 


QStatus MyFirstBusObject::doTell(qcc::String uniqueName, qcc::String thought, int sessionId, qcc::String& reply)
{
    ProxyBusObject remoteObj = ProxyBusObject(*bus, uniqueName.c_str(), MY_FIRST_OBJECT_PATH, (SessionId)sessionId);    
    remoteObj.AddInterface(MY_FIRST_INTERFACE_NAMES[0]);
    Message methodReply(*bus);
    MsgArg arg("s", thought.c_str());
    QStatus status = remoteObj.MethodCall(MY_FIRST_INTERFACE_NAMES[0], "Tell", &arg, 1, methodReply);
    if (ER_OK == status) {
        reply = methodReply->GetArg(0)->v_string.str; 
    }
    return status;
}

void MyFirstBusObject::handleTell(const InterfaceDescription::Member* member, Message& msg)
{
	const char* receivedThought = msg->GetArg(0)->v_string.str;
    printf("Someone(%s) told you (%s)\n", msg->GetSender(), receivedThought);

    MsgArg reply;
    reply.Set("s", "You're so funny!");
	QStatus status = MethodReply(msg, &reply, 1);
	if (status == ER_OK) {
        printf("You let them know they are funny!\n");
    } else {
        printf("An error occured and they do not know that they are funny.\n");
    }
}

QStatus MyFirstBusObject::doShare(qcc::String thought, int sessionId)
{
    MsgArg payload("s", thought.c_str());
    uint8_t flags = 0;
    return Signal(NULL, sessionId, *shareMember, &payload, 1, 0, flags);
}

void MyFirstBusObject::shareHandler(const InterfaceDescription::Member* member, const char* srcPath, Message& msg)
{
	const char* receivedThought = msg->GetArg(0)->v_string.str;
    const char* fromUser = msg->GetSender();
    printf("Received shared thought (%s) from %s on sessionId %d\n", receivedThought, fromUser, msg->GetSessionId());
}

QStatus MyFirstBusObject::doBroadcast(qcc::String thought)
{
    MsgArg payload("s", thought.c_str());
    uint8_t flags = ALLJOYN_FLAG_SESSIONLESS;
    return Signal(NULL, 0, *broadcastMember, &payload, 1, 0, flags);
}

void MyFirstBusObject::broadcastHandler(const InterfaceDescription::Member* member, const char* srcPath, Message& msg)
{
	const char* receivedThought = msg->GetArg(0)->v_string.str;
    const char* fromUser = msg->GetSender();
    printf("Received a broudcast thought (%s) from %s\n", receivedThought, fromUser);  
}
