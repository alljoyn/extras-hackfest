/******************************************************************************
 * Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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
#include <qcc/StringUtil.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdio>

using namespace ajn;
using namespace services;
using namespace qcc;
using namespace std;

/*constants*/
static const char* HELLO_WORLD_INTERFACE_NAME = "com.samples.helloworld";
static qcc::String remoteHelloWorldObjectPath = "";

struct NearbyDevice {
    qcc::String busName;
    unsigned short port;
    qcc::String interestedObjectPath;
    ajn::SessionId sessionId;  
};
static std::vector<NearbyDevice*> deviceList;

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

class AboutClientSessionListener : public ajn::SessionListener {
private:
    ajn::SessionId mySessionID;
    qcc::String serviceName;

public:
    AboutClientSessionListener(qcc::String const& inServiceNAme) : mySessionID(0), serviceName(inServiceNAme) {}

    virtual ~AboutClientSessionListener() {};

    void SessionLost(ajn::SessionId sessionId)
    {
        std::cout << "AboutClient session has been lost for " << serviceName.c_str() << std::endl;
    }
};

QStatus makeHelloWorldCall(NearbyDevice* device) {
    if (false == remoteHelloWorldObjectPath.empty()) {
        ProxyBusObject remoteObj(*busAttachment, device->busName.c_str(), device->interestedObjectPath.c_str(), device->sessionId);
        const InterfaceDescription* alljoynTestIntf = busAttachment->GetInterface(HELLO_WORLD_INTERFACE_NAME);

        assert(alljoynTestIntf);
        remoteObj.AddInterface(*alljoynTestIntf);

        Message reply(*busAttachment);
        QStatus status = remoteObj.MethodCall(HELLO_WORLD_INTERFACE_NAME, "helloWorld", NULL, 0, reply, 5000);

        if (ER_OK == status) {
            printf("MethodCall on '%s.%s' succeeded.\n", HELLO_WORLD_INTERFACE_NAME, "helloWorld");
        } else {
            printf("MethodCall on '%s.%s' failed.\n", HELLO_WORLD_INTERFACE_NAME, "helloWorld");
        }
    }
}

class AboutClientSessionJoiner : public ajn::BusAttachment::JoinSessionAsyncCB {
private:
  qcc::String m_Busname;

public:
    AboutClientSessionJoiner(const char* name) :
		m_Busname("")
	{
		if (name) {
			m_Busname.assign(name);
		}
	}

    virtual ~AboutClientSessionJoiner() {};

    void JoinSessionCB(QStatus status, SessionId id, const SessionOpts& opts, void* context)
    {
        if (status == ER_OK) {
            NearbyDevice* device = (NearbyDevice*)context;
            device->sessionId = id;
            std::cout << "JoinSessionCB(" << m_Busname.c_str() << ") succeeded with id" << id << std::endl;
            deviceList.push_back(device);
        } else {
            std::cout << "JoinSessionCB(" << m_Busname.c_str() << ") failed with status: " << QCC_StatusText(status) << std::endl;
        }

        delete this;
    }
};

class AboutClientAnnounceHandler : public ajn::services::AnnounceHandler {
private:
  AnnounceHandlerCallback m_Callback;

public:
    AboutClientAnnounceHandler(AnnounceHandlerCallback callback) : AnnounceHandler(), m_Callback(callback) {};

    virtual ~AboutClientAnnounceHandler() {};

    void Announce(unsigned short version, unsigned short port, const char* busName, const ObjectDescriptions& objectDescs, const AboutData& aboutData)
    {
        NearbyDevice *device = new NearbyDevice();
        device->busName = busName;
        device->port = port;
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
                    device->interestedObjectPath = key;

			SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);

			AboutClientSessionListener* aboutClientSessionListener = new AboutClientSessionListener(busName);
			AboutClientSessionJoiner* joincb = new AboutClientSessionJoiner(busName);

			QStatus status = busAttachment->JoinSessionAsync(busName,
						    (ajn::SessionPort)port,
                                                    aboutClientSessionListener,
                                                     opts, joincb, device);
                }
                std::cout << " value=" << itv->c_str() << std::endl;
            }
        }

        std::cout << "***********************************************************************" << std::endl << std::endl;


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
    
}

/*
 * get a line of input from the the file pointer (most likely stdin).
 * This will capture the the num-1 characters or till a newline character is
 * entered.
 *
 * @param[out] str a pointer to a character array that will hold the user input
 * @param[in]  num the size of the character array 'str'
 * @param[in]  fp  the file pointer the sting will be read from. (most likely stdin)
 *
 * @return returns the same string as 'str' if there has been a read error a null
 *                 pointer will be returned and 'str' will remain unchanged.
 */
char* get_line(char*str, size_t num, FILE*fp)
{
    char*p = fgets(str, num, fp);

    // fgets will capture the '\n' character if the string entered is shorter than
    // num. Remove the '\n' from the end of the line and replace it with nul '\0'.
    if (p != NULL) {
        size_t last = strlen(str) - 1;
        if (str[last] == '\n') {
            str[last] = '\0';
        }
    }
    return p;
}

static String NextTok(String& inStr)
{
    String ret;
    size_t off = inStr.find_first_of(' ');
    if (off == String::npos) {
        ret = inStr;
        inStr.clear();
    } else {
        ret = inStr.substr(0, off);
        inStr = Trim(inStr.substr(off));
    }
    return Trim(ret);
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

    //set Daemon password only for bundled app
//    #ifdef QCC_USING_BD
//    PasswordManager::SetCredentials("ALLJOYN_PIN_KEYX", "000000");
//    #endif

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

    AboutClientAnnounceHandler* announceHandler = new AboutClientAnnounceHandler(announceHandlerCallback);
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

    const int bufSize = 1024;
    char buf[bufSize];
    std::cout << "Usage: list" << std::endl;
    std::cout << "Usage: call <device index list space seperated>" << std::endl;
    std::cout << "Usage: quit" << std::endl;
    while (get_line(buf, bufSize, stdin)) {
        qcc::String line(buf);
        qcc::String cmd = NextTok(line);
        if(cmd == "list") {
            for (std::vector<NearbyDevice*>::iterator it = deviceList.begin() ; it != deviceList.end(); ++it) {
                std::cout << std::distance(deviceList.begin(), it) << ": " << (*it)->busName.c_str() << std::endl;
            }
        }
        else if (cmd == "call") {
            qcc::String qccIndex = NextTok(line);
            while (!qccIndex.empty()) {
				int loc = atoi(qccIndex.c_str());
				makeHelloWorldCall(deviceList[loc]);     
				qccIndex = NextTok(line);
			}
        }
	else if (cmd == "quit") {
	    AnnouncementRegistrar::UnRegisterAnnounceHandler(*busAttachment, *announceHandler);
	    delete announceHandler;

	    busAttachment->Stop();
	    delete busAttachment;

	    return 0;
	}
        else {
            std::cout << "Usage: list" << std::endl;
            std::cout << "Usage: call <device index list space seperated>" << std::endl;
            std::cout << "Usage: quit" << std::endl;
        }
    }

    return 0;

} /* main() */
