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

#include <iostream>
#include <sstream>
#include <cstdio>
#include <signal.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/about/AboutServiceApi.h>
#include <alljoyn/about/AboutPropertyStoreImpl.h>
#include <alljoyn/controlpanel/ControlPanelService.h>
#include <alljoyn/controlpanel/ControlPanelControllee.h>
#include <alljoyn/services_common/GenericLogger.h>
#include "generated/ControlPanelGenerated.h"

using namespace ajn;
using namespace services;
using namespace qcc;
using namespace Log;

#define SERVICE_PORT 900
#define CHECK_RETURN(x) if ((status = x) != ER_OK) { return status; }

class BusListenerImpl : public BusListener, public SessionPortListener {
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

static BusAttachment* busAttachment = NULL;
static BusListenerImpl* busListener = NULL;
static AboutPropertyStoreImpl* aboutPropertyStore = NULL;
static ControlPanelService* controlPanelService = NULL;
static ControlPanelControllee* controlPanelControllee = NULL;

static void cleanup()
{
    if (controlPanelService) {
        controlPanelService->shutdownControllee();
        delete controlPanelService;
        controlPanelService = NULL;
    }
    if (controlPanelControllee) {
        delete controlPanelControllee;
        controlPanelControllee = NULL;
    }
    if (aboutPropertyStore) {
		delete aboutPropertyStore;
		aboutPropertyStore = NULL;
	}
	if (busAttachment && busListener) {
		busAttachment->UnregisterBusListener(*busListener);
		busAttachment->UnbindSessionPort(busListener->getSessionPort());
	}
	if (busListener) {
		delete busListener;
		busListener = NULL;
	}
    if (busAttachment) {
        delete busAttachment;
        busAttachment = NULL;
    }
	AboutServiceApi::DestroyInstance();
    ControlPanelGenerated::Shutdown();
}

static void SigIntHandler(int signum) {
    cleanup();
    std::cout << "Goodbye!" << std::endl;
	exit(signum);
}

static QStatus FillAboutPropertyStoreImplData(AboutPropertyStoreImpl* propStore)
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

    CHECK_RETURN(propStore->setAppName("ControlPanelSampleControllee"))
    CHECK_RETURN(propStore->setModelNumber("Tutorial5000"))
    CHECK_RETURN(propStore->setDateOfManufacture("12/09/2013"))
    CHECK_RETURN(propStore->setSoftwareVersion("12.20.44 build 44454"))
    CHECK_RETURN(propStore->setAjSoftwareVersion(ajn::GetVersion()))
    CHECK_RETURN(propStore->setHardwareVersion("355.499. b"))

    CHECK_RETURN(propStore->setDescription("This is the ControlPanelSampleControllee sample", "en"))
    CHECK_RETURN(propStore->setManufacturer("Company", "en"))

    CHECK_RETURN(propStore->setSupportUrl("http://www.allseenalliance.org"))
    return status;
}

int32_t main()
{
	std::cout << "AllJoyn Library version: " << ajn::GetVersion() << std::endl;
	std::cout << "AllJoyn Library build info: " << ajn::GetBuildInfo() << std::endl;

    // Allow CTRL+C to end application
    signal(SIGINT, SigIntHandler);
    std::cout << "Beginning ControlPanel Application. (Press CTRL+C to end application)" << std::endl;

    controlPanelService = ControlPanelService::getInstance();
    if (NULL == controlPanelService) {
		std::cout << "Could not initialize ControlPanelService." << std::endl;
		return ER_OUT_OF_MEMORY;
	}

    // Set loglevel
    controlPanelService->setLogLevel(LEVEL_WARN);

    busAttachment = new BusAttachment("ControlPanelSampleControllee", true);
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

	busListener = new BusListenerImpl();
	busListener->setSessionPort(SERVICE_PORT);
	busAttachment->RegisterBusListener(*busListener);

	aboutPropertyStore = new AboutPropertyStoreImpl();
	status = FillAboutPropertyStoreImplData(aboutPropertyStore);
	if (ER_OK != status) {
		std::cout << "Error in FillAboutPropertyStoreImplData (" << QCC_StatusText(status) << ")." << std::endl;
		cleanup();
		return EXIT_FAILURE;
	}

	AboutServiceApi::Init(*busAttachment, *aboutPropertyStore);
	if (NULL == AboutServiceApi::getInstance()) {
		std::cout << "Could not get an instance of the AboutServiceApi." << std::endl;
		cleanup();
		return EXIT_FAILURE;
	}

	status = AboutServiceApi::getInstance()->Register(SERVICE_PORT);
	if (ER_OK != status) {
		std::cout << "Could not register AboutServiceApi service port (" << QCC_StatusText(status) << ")." << std::endl;
		cleanup();
		return EXIT_FAILURE;
	}

	status = busAttachment->RegisterBusObject(*AboutServiceApi::getInstance());
	if (ER_OK != status) {
		std::cout << "Error returned by RegisterBusObject(AboutServiceApi) (" << QCC_StatusText(status) << ")." << std::endl;
		cleanup();
		return EXIT_FAILURE;
	}

    status = ControlPanelGenerated::PrepareWidgets(controlPanelControllee);
    if (ER_OK != status) {
		std::cout << "Error in PrepareWidgets (" << QCC_StatusText(status) << ")." << std::endl;
        cleanup();
		return EXIT_FAILURE;
    }

    status = controlPanelService->initControllee(busAttachment, controlPanelControllee);
    if (ER_OK != status) {
    	std::cout << "Error in initControllee (" << QCC_StatusText(status) << ")." << std::endl;
		cleanup();
		return EXIT_FAILURE;
    }

	status = AboutServiceApi::getInstance()->Announce();
	if (ER_OK != status) {
		std::cout << "Error returned by Announce (" << QCC_StatusText(status) << ")." << std::endl;
		cleanup();
		return EXIT_FAILURE;
	}

    std::cout << "Sent announce, waiting for Controllers" << std::endl;

    uint32_t counterValue = 1;
    while (1) {
        sleep(5);

        std::cout << "Sending update signal: counter value " << counterValue << std::endl;

        char buf[256];
        sprintf(buf, "%d", counterValue++);
        ControlPanelGenerated::myDeviceCounterValueStringProperty->setValue(buf);
		ControlPanelGenerated::myDeviceCounterValueStringProperty->SendValueChangedSignal();
    }
}
