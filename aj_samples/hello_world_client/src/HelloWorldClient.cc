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

using namespace ajn;
using namespace services;
using namespace qcc;

/*constants*/
static const char* HELLO_WORLD_INTERFACE_NAME = "com.samples.helloworld";
static qcc::String remoteHelloWorldObjectPath = "";

static volatile sig_atomic_t quit;

static BusAttachment* busAttachment;

static void SignalHandler(int sig)
{
    switch (sig) {
    case SIGINT:
    case SIGTERM:
        quit = 1;
        break;
    }
}

#define SERVICE_EXIT_OK       0
#define SERVICE_OPTION_ERROR  1
#define SERVICE_CONFIG_ERROR  2

typedef void (*AnnounceHandlerCallback)(qcc::String const& busName, unsigned short port);

class HelloWorldClientAnnounceHandler : public ajn::services::AnnounceHandler {
private:
  AnnounceHandlerCallback m_Callback;

public:
    HelloWorldClientAnnounceHandler(AnnounceHandlerCallback callback) : AnnounceHandler(), m_Callback(callback) {};

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
                }
                std::cout << " value=" << itv->c_str() << std::endl;
            }
        }

        std::cout << "***********************************************************************" << std::endl << std::endl;

        if (m_Callback) {
            std::cout << "Calling AnnounceHandler Callback" << std::endl;
            m_Callback(busName, port);
        }
    }
};

class HelloWorldClientSessionListener : public ajn::SessionListener {
private:
  ajn::SessionId mySessionID;
  qcc::String serviceName;

public:

	HelloWorldClientSessionListener(qcc::String const& inServiceName, ajn::SessionId sessionId) : mySessionID(sessionId), serviceName(inServiceName) {}

    virtual ~HelloWorldClientSessionListener() {};

    void SessionLost(ajn::SessionId sessionId)
    {
        std::cout << "HelloWorldClient session has been lost for " << serviceName.c_str() << std::endl;
    }
};

class HelloWorldClientSessionJoiner : public ajn::BusAttachment::JoinSessionAsyncCB {
private:
	qcc::String m_Busname;

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

	void sessionJoinedCallback(qcc::String const& uniqueName, SessionId sessionId)
	{
	  busAttachment->EnableConcurrentCallbacks();

	  AboutClient* aboutClient = new AboutClient(*busAttachment);
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
	HelloWorldClientSessionJoiner(const char* name) :
	  m_Busname("")
	{
	  if (name) {
		  m_Busname.assign(name);
	  }
	}

	virtual ~HelloWorldClientSessionJoiner() {};

	void JoinSessionCB(QStatus status, SessionId id, const SessionOpts& opts, void* context)
	{
		if (ER_OK == status) {
			std::cout << "JoinSessionCB(" << m_Busname.c_str() << ") succeeded with id" << id << std::endl;
			std::cout << "Calling sessionJoinedCallback" << std::endl;
			sessionJoinedCallback(m_Busname, id);
		} else {
			std::cout << "JoinSessionCB(" << m_Busname.c_str() << ") failed with status: " << QCC_StatusText(status) << std::endl;
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
		printf("\n\n--==## signalConsumer: HelloWorldSignal Received ##==--\n\n");
	}
};

QStatus BuildBusObject(HelloWorldSignalHandlerBusObject*& helloWorldSignalHandlerBusObject)
{
    InterfaceDescription* intf = NULL;
    QStatus status = busAttachment->CreateInterface(HELLO_WORLD_INTERFACE_NAME, intf);

    if (status == ER_OK) {
        printf("Interface created.\n");
        intf->AddMethod("helloWorld", NULL,  NULL, NULL, 0);
        intf->AddSignal("helloWorldSignal", NULL, NULL, 0);
        intf->Activate();

        helloWorldSignalHandlerBusObject = new HelloWorldSignalHandlerBusObject("/helloWorldSignalHandler");
        helloWorldSignalHandlerBusObject->Init(intf);

		/* register the signal handler for the the 'helloWorldSignal' */
		status =  busAttachment->RegisterSignalHandler(helloWorldSignalHandlerBusObject,
						static_cast<MessageReceiver::SignalHandler>(&HelloWorldSignalHandlerBusObject::HelloWorldSignalHandler),
						helloWorldSignalHandlerBusObject->GetHelloWorldSignalMember(),
						NULL);

		busAttachment->AddMatch("type='signal',interface='com.samples.helloworld',member='helloWorldSignal'");
    } else {
        printf("Failed to create interface '%s'.\n", HELLO_WORLD_INTERFACE_NAME);
    }

    return status;
}

void announceHandlerCallback(qcc::String const& busName, unsigned short port)
{
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);

    HelloWorldClientSessionListener* sessionListener = new HelloWorldClientSessionListener(busName, (ajn::SessionId) port);
    HelloWorldClientSessionJoiner* joincb = new HelloWorldClientSessionJoiner(busName.c_str());

    QStatus status = busAttachment->JoinSessionAsync(busName.c_str(), (ajn::SessionPort) port, sessionListener,
                                                     opts, joincb, NULL);

    if (status != ER_OK) {
        std::cout << "Unable to JoinSession with " << busName.c_str() << std::endl;
        return;
    }
}

/**
 *  client main function.
 *
 * @return
 *      - 0 if successful.
 *      - 1 if error.
 */
int main(int argc, char**argv, char**envArg)
{
	HelloWorldSignalHandlerBusObject* helloWorldSignalHandlerBusObject;

    QStatus status = ER_OK;
    std::cout << "AllJoyn Library version: " << ajn::GetVersion() << std::endl;
    std::cout << "AllJoyn Library build info: " << ajn::GetBuildInfo() << std::endl;

//    QCC_SetLogLevels("ALLJOYN_ABOUT_CLIENT=7");
//    QCC_SetLogLevels("ALLJOYN_ABOUT_ANNOUNCE_HANDLER=7");
//    QCC_SetLogLevels("ALLJOYN_ABOUT_ANNOUNCEMENT_REGISTRAR=7");

    // QCC_SetLogLevels("ALLJOYN=7;ALL=1");
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

    busAttachment = new BusAttachment("HelloWorldClient", true);

    status = busAttachment->Start();
    if (status == ER_OK) {
        std::cout << "BusAttachment started." << std::endl;
    } else {
        std::cout << "Unable to start BusAttachment. Status: " << QCC_StatusText(status) << std::endl;
        return 1;
    }

    status = busAttachment->Connect();
    if (ER_OK == status) {
        std::cout << "Daemon Connect succeeded." << std::endl;
    } else {
        std::cout << "Failed to connect daemon. Status: " << QCC_StatusText(status) << std::endl;
        return 1;
    }
    if (ER_OK == status) {
    	status = BuildBusObject(helloWorldSignalHandlerBusObject);
    }

    HelloWorldClientAnnounceHandler* announceHandler = new HelloWorldClientAnnounceHandler(announceHandlerCallback);
    if (ER_OK == status) {
    	status = AnnouncementRegistrar::RegisterAnnounceHandler(*busAttachment, *announceHandler);
    }

    if (ER_OK == status) {
    	status = busAttachment->AddMatch("sessionless='t',type='error'");
    }

    // Setup signals to wait for.
    sigfillset(&waitmask);
    sigdelset(&waitmask, SIGINT);
    sigdelset(&waitmask, SIGTERM);

    if (ER_OK != status) {
		std::cout << "Bad status (" << QCC_StatusText(status) << ")." << std::endl;
	}

    quit = 0;
    while (!quit) {
        // Wait for a signal.
        sigsuspend(&waitmask);
    }

    AnnouncementRegistrar::UnRegisterAnnounceHandler(*busAttachment, *announceHandler);
    delete announceHandler;

    busAttachment->Stop();
    delete busAttachment;

    return 0;

} /* main() */
