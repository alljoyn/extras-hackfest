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

#include <alljoyn/BusAttachment.h>
#include <alljoyn/about/AboutClient.h>
#include <alljoyn/about/AnnouncementRegistrar.h>

#include <signal.h>
#include <iostream>
#include <iomanip>
#include <cstdio>

using namespace ajn;
using namespace services;
using namespace qcc;

static const char* HELLO_WORLD_INTERFACE_NAME = "com.samples.helloworld";
static qcc::String remoteHelloWorldObjectPath = "";

static volatile sig_atomic_t quit;

typedef void (*AnnounceHandlerCallback)(qcc::String const& busName, unsigned short port);

void makeHelloWorldCall(const qcc::String& uniqueName, SessionId sessionId);
void connectTo(qcc::String const& busName, unsigned short port);

class HelloWorldClientAnnounceHandler : public ajn::services::AnnounceHandler {

public:
    HelloWorldClientAnnounceHandler() : AnnounceHandler() {};

    virtual ~HelloWorldClientAnnounceHandler() {};

    void Announce(unsigned short version, unsigned short port, const char* busName, const ObjectDescriptions& objectDescs, const AboutData& aboutData)
    {
        std::cout << std::endl << std::endl << "***********************************************************************"
                  << std::endl;
        std::cout << "busName  " << busName << std::endl;
        std::cout << "port  " << port << std::endl;
        std::cout << "ObjectDescriptions" << std::endl;
        for (AboutClient::ObjectDescriptions::const_iterator it = objectDescs.begin(); it != objectDescs.end(); ++it) {
            qcc::String key = it->first;
            std::vector<qcc::String> vector = it->second;
            std::cout << "key=" << key.c_str();
            for (std::vector<qcc::String>::const_iterator itv = vector.begin(); itv != vector.end(); ++itv) {
                if (0 == itv->compare(HELLO_WORLD_INTERFACE_NAME)) {
                    remoteHelloWorldObjectPath = key;
                    connectTo(busName, port);
                }
                std::cout << " value=" << itv->c_str() << std::endl;
            }
        }
    }
};

class HelloWorldClientSessionJoiner : public ajn::BusAttachment::JoinSessionAsyncCB {
private:
	BusAttachment* mBusAttachment;
	qcc::String mBusname;

	void sessionJoinedCallback(qcc::String const& uniqueName, SessionId sessionId)
	{
	  mBusAttachment->EnableConcurrentCallbacks();

	  AboutClient* aboutClient = new AboutClient(*mBusAttachment);
	  std::cout << "-----------------------------------" << std::endl;
	  if (false == remoteHelloWorldObjectPath.empty()) {
		  std::cout << "Joining session with sessionId " << sessionId << " with " << uniqueName.c_str() << std::endl;
		  makeHelloWorldCall(uniqueName, sessionId);
	  }

	  if (aboutClient) {
		  delete aboutClient;
		  aboutClient = NULL;
	  }
	}

public:
	HelloWorldClientSessionJoiner(BusAttachment* bus, const char* name) :
		mBusAttachment(bus), mBusname("")
	{
	  if (name) {
		  mBusname.assign(name);
	  }
	}

	virtual ~HelloWorldClientSessionJoiner() {};

	void JoinSessionCB(QStatus status, SessionId id, const SessionOpts& opts, void* context)
	{
		if (ER_OK == status) {
			std::cout << "JoinSessionCB(" << mBusname.c_str() << ") succeeded with id" << id << std::endl;
			std::cout << "Calling sessionJoinedCallback" << std::endl;
			sessionJoinedCallback(mBusname, id);
		} else {
			std::cout << "JoinSessionCB(" << mBusname.c_str() << ") failed with status: " << QCC_StatusText(status) << std::endl;
		}
	}

};

class HelloWorldSignalHandlerBusObject: public BusObject {
private:
	const InterfaceDescription::Member* helloWorldSignalMember;

public:
	HelloWorldSignalHandlerBusObject(const char* path) : BusObject(path) {
	}

	void Init(InterfaceDescription* intf) {
		helloWorldSignalMember = intf->GetMember("helloWorldSignal");
		assert(helloWorldSignalMember);
	}

	const InterfaceDescription::Member* GetHelloWorldSignalMember(void) {
		return helloWorldSignalMember;
	}

	void HelloWorldSignalHandler(const InterfaceDescription::Member* member,
								const char* sourcePath,
								Message& msg)
	{
		printf("\n ### signalConsumer: HelloWorldSignal Received ###\n\n");
	}
};

static BusAttachment* busAttachment = NULL;
static HelloWorldSignalHandlerBusObject* helloWorldSignalHandlerBusObject = NULL;
static HelloWorldClientAnnounceHandler* announceHandler = NULL;

static void cleanup()
{
    // Clean up
	if (busAttachment && announceHandler) {
	    AnnouncementRegistrar::UnRegisterAnnounceHandler(*busAttachment, *announceHandler);
	}
	if (announceHandler) {
		delete announceHandler;
		announceHandler = NULL;
	}
	if (helloWorldSignalHandlerBusObject) {
		delete helloWorldSignalHandlerBusObject;
		helloWorldSignalHandlerBusObject = NULL;
	}
    if (busAttachment) {
        delete busAttachment;
        busAttachment = NULL;
    }
}

static void SignalHandler(int sig)
{
    switch (sig) {
    case SIGINT:
    case SIGTERM:
        quit = 1;
        cleanup();
        break;
    }
}

static QStatus BuildBusObject(HelloWorldSignalHandlerBusObject*& helloWorldSignalHandlerBusObject)
{
    InterfaceDescription* intf = NULL;
    QStatus status = busAttachment->CreateInterface(HELLO_WORLD_INTERFACE_NAME, intf);
	if (ER_OK != status) {
		std::cout << "Error in CreateInterface (" << QCC_StatusText(status) << ")." << std::endl;
		cleanup();
		return ER_FAIL;
	}

	printf("Interface created.\n");
	intf->AddMethod("helloWorld", NULL,  NULL, NULL, 0);
	if (ER_OK != status) {
		std::cout << "Error in AddMethod (" << QCC_StatusText(status) << ")." << std::endl;
		cleanup();
		return ER_FAIL;
	}
	intf->AddSignal("helloWorldSignal", NULL, NULL, 0);
	if (ER_OK != status) {
		std::cout << "Error in AddSignal (" << QCC_StatusText(status) << ")." << std::endl;
		cleanup();
		return ER_FAIL;
	}
	intf->Activate();

	helloWorldSignalHandlerBusObject = new HelloWorldSignalHandlerBusObject("/helloWorldSignalHandler");
	helloWorldSignalHandlerBusObject->Init(intf);

	/* register the signal handler for the the 'helloWorldSignal' */
	status =  busAttachment->RegisterSignalHandler(helloWorldSignalHandlerBusObject,
					static_cast<MessageReceiver::SignalHandler>(&HelloWorldSignalHandlerBusObject::HelloWorldSignalHandler),
					helloWorldSignalHandlerBusObject->GetHelloWorldSignalMember(),
					NULL);
	if (ER_OK != status) {
		std::cout << "Error in RegisterSignalHandler (" << QCC_StatusText(status) << ")." << std::endl;
		cleanup();
		return ER_FAIL;
	}

	busAttachment->AddMatch("type='signal',interface='com.samples.helloworld',member='helloWorldSignal'");
	if (ER_OK != status) {
		std::cout << "Error in AddMatch (type='signal') (" << QCC_StatusText(status) << ")." << std::endl;
		cleanup();
		return ER_FAIL;
	}

    return status;
}

void connectTo(qcc::String const& busName, unsigned short port)
{
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);

    HelloWorldClientSessionJoiner* joincb = new HelloWorldClientSessionJoiner(busAttachment, busName.c_str());

    QStatus status = busAttachment->JoinSessionAsync(busName.c_str(), (ajn::SessionPort) port, NULL,
                                                     opts, joincb, NULL);

    if (status != ER_OK) {
        std::cout << "Unable to JoinSession with " << busName.c_str() << std::endl;
        return;
    }
}

void makeHelloWorldCall(const qcc::String& uniqueName,
		SessionId sessionId) {
	ProxyBusObject remoteObj(*busAttachment, uniqueName.c_str(),
			remoteHelloWorldObjectPath.c_str(), sessionId);
	const InterfaceDescription* alljoynTestIntf =
			busAttachment->GetInterface(HELLO_WORLD_INTERFACE_NAME);
	assert(alljoynTestIntf);
	remoteObj.AddInterface(*alljoynTestIntf);
	Message reply(*busAttachment);
	QStatus status = remoteObj.MethodCall(HELLO_WORLD_INTERFACE_NAME,
			"helloWorld", NULL, 0, reply, 5000);
	if (ER_OK == status) {
		printf("MethodCall on '%s.%s' succeeded.\n",
				HELLO_WORLD_INTERFACE_NAME, "helloWorld");
	} else {
		printf("MethodCall on '%s.%s' failed.\n",
				HELLO_WORLD_INTERFACE_NAME, "helloWorld");
	}
}

int main(int argc, char**argv, char**envArg)
{
	std::cout << "AllJoyn Library version: " << ajn::GetVersion() << std::endl;
	std::cout << "AllJoyn Library build info: " << ajn::GetBuildInfo() << std::endl;

    struct sigaction act, oldact;
    sigset_t sigmask, waitmask;

    // Block all signals by default for all threads.
    sigfillset(&sigmask);
    sigdelset(&sigmask, SIGSEGV);
    pthread_sigmask(SIG_BLOCK, &sigmask, NULL);

    // Setup a handler for SIGINT and SIGTERM
    act.sa_handler = SignalHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGINT, &act, &oldact);
    sigaction(SIGTERM, &act, &oldact);

    // Create message bus
    busAttachment = new BusAttachment("HelloWorldClient", true);
    if (NULL == busAttachment) {
		std::cout << "Could not initialize BusAttachment." << std::endl;
		return ER_OUT_OF_MEMORY;
	}

    // Start the BusAttachment
    QStatus status = busAttachment->Start();
    if (ER_OK != status) {
		std::cout << "Failed to start the BusAttachment (" << QCC_StatusText(status) << ")." << std::endl;
		cleanup();
		return EXIT_FAILURE;
	}

	// Connect to the AJ Router
    status = busAttachment->Connect();
    if (ER_OK != status) {
		std::cout << "Failed to connect to AJ Router (" << QCC_StatusText(status) << ")." << std::endl;
		cleanup();
		return EXIT_FAILURE;
	}

	status = BuildBusObject(helloWorldSignalHandlerBusObject);
	if (ER_OK != status) {
		std::cout << "Error in BuildBusObject (" << QCC_StatusText(status) << ")." << std::endl;
		cleanup();
		return EXIT_FAILURE;
	}

	announceHandler = new HelloWorldClientAnnounceHandler();
    if (NULL == announceHandler) {
		std::cout << "Could not initialize HelloWorldClientAnnounceHandler." << std::endl;
		cleanup();
		return ER_OUT_OF_MEMORY;
	}

    status = AnnouncementRegistrar::RegisterAnnounceHandler(*busAttachment, *announceHandler);
    if (ER_OK != status) {
		std::cout << "Error in RegisterAnnounceHandler (" << QCC_StatusText(status) << ")." << std::endl;
		cleanup();
		return EXIT_FAILURE;
	}

	status = busAttachment->AddMatch("sessionless='t',type='error'");
    if (ER_OK != status) {
		std::cout << "Error in AddMatch (sessionless='t') (" << QCC_StatusText(status) << ")." << std::endl;
		cleanup();
		return EXIT_FAILURE;
	}

    // Setup signals to wait for.
    sigfillset(&waitmask);
    sigdelset(&waitmask, SIGINT);
    sigdelset(&waitmask, SIGTERM);

    quit = 0;
    while (!quit) {
        // Wait for a signal.
        sigsuspend(&waitmask);
    }
    std::cout << "Goodbye!" << std::endl;
    return 0;

} /* main() */
