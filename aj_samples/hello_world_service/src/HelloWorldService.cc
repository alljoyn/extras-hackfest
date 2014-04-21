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
#include <alljoyn/about/AboutServiceApi.h>
#include <alljoyn/about/AboutPropertyStoreImpl.h>
#include <alljoyn/BusObject.h>

#include <signal.h>

using namespace ajn;
using namespace services;

#define CHECK_RETURN(x) if ((status = x) != ER_OK) { return status; }

#define SERVICE_EXIT_OK       0
#define SERVICE_OPTION_ERROR  1
#define SERVICE_CONFIG_ERROR  2

/*constants*/
static const char* HELLO_WORLD_INTERFACE_NAME = "com.samples.helloworld";
static const char* HELLO_WORLD_OBJECT_PATH = "/helloWorld";

static const SessionPort SERVICE_PORT = 25;

static volatile sig_atomic_t s_interrupt = false;

/** Top level message bus object. */
static BusAttachment* busAttachment = NULL;

static void SigIntHandler(int sig) {
    s_interrupt = true;
}

class BusListenerImpl : public ajn::BusListener, public ajn::SessionPortListener {
private:
  /**
   * The port used as part of the join session request
   */
  ajn::SessionPort m_SessionPort;

public:
    BusListenerImpl() : BusListener(), SessionPortListener(), m_SessionPort(0) {}

    /**
     * @param sessionPort - port of listener
     */
    BusListenerImpl(ajn::SessionPort sessionPort) : BusListener(), SessionPortListener(), m_SessionPort(sessionPort) {}

    ~BusListenerImpl() {}

    /**
     * AcceptSessionJoiner - Receive request to join session and decide whether to accept it or not
     * @param sessionPort - the port of the request
     * @param joiner - the name of the joiner
     * @param opts - the session options
     * @return true/false
     */
    bool AcceptSessionJoiner(ajn::SessionPort sessionPort, const char* joiner, const ajn::SessionOpts& opts)
    {
        if (sessionPort != m_SessionPort) {
            std::cout << "Rejecting join attempt on unexpected session port " << sessionPort << std::endl;
            return false;
        }

        std::cout << "Accepting JoinSessionRequest from " << joiner << " (opts.proximity= " << opts.proximity
                  << ", opts.traffic=" << opts.traffic << ", opts.transports=" << opts.transports << ")." << std::endl;
        return true;
    }

    /**
     * Set the Value of the SessionPort associated with this SessionPortListener
     * @param sessionPort
     */
    void setSessionPort(ajn::SessionPort sessionPort)
    {
        m_SessionPort = sessionPort;
    }

    /**
     * Get the SessionPort of the listener
     * @return
     */
    ajn::SessionPort getSessionPort()
    {
        return m_SessionPort;
    }

};

class HelloWorldBusObject : public BusObject {
private:
	const InterfaceDescription::Member* helloWorldSignalMember;

    QStatus EmitHelloWorldSignal(SessionId sessionId)
    {
    	assert(helloWorldSignalMember);
    	uint8_t flags = 0;
    	QStatus status;
    	if (NULL != helloWorldSignalMember) {
    		printf("Emitting HelloWorld Signal.\n");
    		status = Signal(NULL, sessionId, *helloWorldSignalMember, NULL, 0, 0, flags);
    		if (ER_OK != status) {
    			std::cout << "Signal failed (" << QCC_StatusText(status) << "), for sessionId " << sessionId << std::endl;
    		}
    	}
    	else {
    		printf("ERROR - helloWorldSignalMember was NULL!");
    	}
    	return status;
    }

public:
    HelloWorldBusObject(BusAttachment& bus, const char* path) :
        BusObject(path)
    {
    }

    void Init(InterfaceDescription* intf) {
        helloWorldSignalMember = intf->GetMember("helloWorldSignal");
        assert(helloWorldSignalMember);

    	AddInterface(*intf);

		/** Register the method handlers with the object */
    	assert(intf->GetMember("helloWorld"));
		const MethodEntry methodEntries[] = {
			{ intf->GetMember("helloWorld"), static_cast<MessageReceiver::MethodHandler>(&HelloWorldBusObject::HelloWorld) }
		};
		QStatus status = AddMethodHandlers(methodEntries, sizeof(methodEntries) / sizeof(methodEntries[0]));
		if (ER_OK != status) {
			printf("Failed to register method handlers for HelloWorldSampleObject.\n");
		}
    }

    void ObjectRegistered()
    {
        BusObject::ObjectRegistered();
        printf("ObjectRegistered has been called.\n");
    }

    void HelloWorld(const InterfaceDescription::Member* member, Message& msg)
    {
        QStatus status = MethodReply(msg, ER_OK);
        if (ER_OK != status) {
            printf("Ping: Error sending reply.\n");
        }
        printf("Hello World Method!\n");

        EmitHelloWorldSignal(msg->GetSessionId());
    }

};

/** Start the message bus, report the result to stdout, and return the status code. */
QStatus StartMessageBus(void) {
    QStatus status = busAttachment->Start();

    if (ER_OK == status) {
        std::cout << "BusAttachment started." << std::endl;
    } else {
        std::cout << "Start of BusAttachment failed (" << QCC_StatusText(status) << ")." << std::endl;
    }

    return status;
}

/** Connect to the AJ Router, report the result to stdout, and return the status code. */
QStatus ConnectToAllJoynRouter() {
    QStatus status;
    status = busAttachment->Connect();
    if (ER_OK == status) {
        std::cout << "AJ Router connect succeeded." << std::endl;
    } else {
        std::cout << "Failed to connect to AJ Router (" << QCC_StatusText(status) << ")." << std::endl;
    }
    return status;
}

QStatus FillAboutPropertyStoreImplData(AboutPropertyStoreImpl* propStore)
{
    QStatus status = ER_OK;

    // a platform-specific unique device id - ex. could be the Mac address
    CHECK_RETURN(propStore->setDeviceId("1231232145667745675477"))
    CHECK_RETURN(propStore->setDeviceName("MyDeviceName"))
    // the globally unique identifier for the application - recommend to use an online GUID generator to create
    CHECK_RETURN(propStore->setAppId("000102030405060708090A0B0C0D0E0C"))

    std::vector<qcc::String> languages(1);
    languages[0] = "en";
    CHECK_RETURN(propStore->setSupportedLangs(languages))
    CHECK_RETURN(propStore->setDefaultLang("en"))

    CHECK_RETURN(propStore->setAppName("HelloWorldService"))
    CHECK_RETURN(propStore->setModelNumber("Tutorial5000"))
    CHECK_RETURN(propStore->setDateOfManufacture("12/09/2013"))
    CHECK_RETURN(propStore->setSoftwareVersion("12.20.44 build 44454"))
    CHECK_RETURN(propStore->setAjSoftwareVersion(ajn::GetVersion()))
    CHECK_RETURN(propStore->setHardwareVersion("355.499. b"))

    CHECK_RETURN(propStore->setDescription("This is the HelloWorldService sample", "en"))
    CHECK_RETURN(propStore->setManufacturer("Company", "en"))

    CHECK_RETURN(propStore->setSupportUrl("http://www.allseenalliance.org"))
    return status;
}

/** Create the interface, report the result to stdout, and return the result status. */
QStatus BuildBusObject(HelloWorldBusObject*& helloWorldBusObject)
{
    InterfaceDescription* intf = NULL;
    QStatus status = busAttachment->CreateInterface(HELLO_WORLD_INTERFACE_NAME, intf);

    if (status == ER_OK) {
        printf("Interface created.\n");
        intf->AddMethod("helloWorld", NULL,  NULL, NULL, 0);
        intf->AddSignal("helloWorldSignal", NULL, NULL, 0);
        intf->Activate();

        helloWorldBusObject = new HelloWorldBusObject(*busAttachment, HELLO_WORLD_OBJECT_PATH);
        helloWorldBusObject->Init(intf);
    } else {
        printf("Failed to create interface '%s'.\n", HELLO_WORLD_INTERFACE_NAME);
    }
    return status;
}

/** Create the session, report the result to stdout, and return the status code. */
QStatus BindSession(BusListenerImpl& busListener) {
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    SessionPort sp = SERVICE_PORT;
    QStatus status = busAttachment->BindSessionPort(sp, opts, busListener);

    if (ER_OK == status) {
        std::cout << "BindSessionPort succeeded." << std::endl;
    } else {
        std::cout << "BindSessionPort failed (" << QCC_StatusText(status) << ")." << std::endl;
    }

    return status;
}

static void shutdown(BusListenerImpl& busListener, AboutPropertyStoreImpl*& aboutPropertyStore, HelloWorldBusObject*& helloWorldSampleObject)
{
    busAttachment->UnregisterBusListener(busListener);
    busAttachment->UnbindSessionPort(busListener.getSessionPort());

    AboutServiceApi::DestroyInstance();

    if (aboutPropertyStore) {
        delete aboutPropertyStore;
        aboutPropertyStore = NULL;
    }

    if (helloWorldSampleObject) {
    	delete helloWorldSampleObject;
    	helloWorldSampleObject = NULL;
    }

    delete busAttachment;
    busAttachment = NULL;
}

void WaitForSigInt(void) {
    while (s_interrupt == false) {
#ifdef _WIN32
        Sleep(100);
#else
        usleep(100 * 1000);
#endif
    }
}

int main(int argc, char**argv, char**envArg) {
    QStatus status = ER_OK;
    std::cout << "AllJoyn Library version: " << ajn::GetVersion() << std::endl;
    std::cout << "AllJoyn Library build info: " << ajn::GetBuildInfo() << std::endl;
    //QCC_SetLogLevels("ALLJOYN_ABOUT_SERVICE=7;");

    BusListenerImpl busListener;
    busListener.setSessionPort(SERVICE_PORT);

    /* Install SIGINT handler so Ctrl + C deallocates memory properly */
    signal(SIGINT, SigIntHandler);

    /* Create message bus */
    busAttachment = new BusAttachment("HelloWorldService", true);

    if (!busAttachment) {
        status = ER_OUT_OF_MEMORY;
        return status;
    }

    if (ER_OK == status) {
        status = StartMessageBus();
    }

    if (ER_OK == status) {
        status = ConnectToAllJoynRouter();
    }

    if (ER_OK == status) {
        busAttachment->RegisterBusListener(busListener);
    }

    AboutPropertyStoreImpl* aboutPropertyStore = NULL;
    HelloWorldBusObject* helloWorldBusObject = NULL;

    if (ER_OK == status) {
        aboutPropertyStore = new AboutPropertyStoreImpl();
        status = FillAboutPropertyStoreImplData(aboutPropertyStore);
    }
	if (ER_OK != status) {
		shutdown(busListener, aboutPropertyStore, helloWorldBusObject);
		return EXIT_FAILURE;
	}

	AboutServiceApi::Init(*busAttachment, *aboutPropertyStore);
	if (!AboutServiceApi::getInstance()) {
		shutdown(busListener, aboutPropertyStore, helloWorldBusObject);
		return EXIT_FAILURE;
	}

	AboutServiceApi::getInstance()->Register(SERVICE_PORT);
	status = busAttachment->RegisterBusObject(*AboutServiceApi::getInstance());

	std::vector<qcc::String> interfaces;
	interfaces.push_back(HELLO_WORLD_INTERFACE_NAME);
	status = AboutServiceApi::getInstance()->AddObjectDescription(HELLO_WORLD_OBJECT_PATH, interfaces);

	if (ER_OK == status) {
		status = BuildBusObject(helloWorldBusObject);
	}
	if (ER_OK == status) {
		status = busAttachment->RegisterBusObject(*helloWorldBusObject);
	}

    if (ER_OK == status) {
        status = BindSession(busListener);
    }

    if (ER_OK == status) {
        status = AboutServiceApi::getInstance()->Announce();
    }

    /* Perform the service asynchronously until the user signals for an exit. */
    if (ER_OK == status) {
        WaitForSigInt();
    }
    else {
    	std::cout << "Bad status (" << QCC_StatusText(status) << ")." << std::endl;
    }

    shutdown(busListener, aboutPropertyStore, helloWorldBusObject);

    return 0;
} /* main() */




