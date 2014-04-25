/******************************************************************************
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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
//#include <alljoyn/about/AnnouncementRegistrar.h>
//#include <alljoyn/controlpanel/ControlPanelService.h>
//#include <alljoyn/controlpanel/ControlPanelController.h>
//#include <alljoyn/services_common/GenericLogger.h>
//#include "ControlPanelListenerImpl.h"
//#include <SrpKeyXListener.h>
//#include <CommonSampleUtil.h>
//#include <AnnounceHandlerImpl.h>

#include "CommandLineController.h"

//#define SERVICE_PORT 900

//using namespace ajn;
//using namespace services;
//using namespace qcc;

/*BusAttachment* bus = 0;
ControlPanelService* controlPanelService = 0;
ControlPanelController* controlPanelController = 0;
ControlPanelListenerImpl* controlPanelListener = 0;
SrpKeyXListener* srpKeyXListener = 0;
AnnounceHandlerImpl* announceHandler = 0;
qcc::String ControlPanelPrefix = "/ControlPanel/";

typedef struct NearbyDeviceStruct {
    
    qcc::String busName;
    unsigned short version;
    unsigned short port;
    AnnounceHandler::ObjectDescriptions objectDescs;
    AnnounceHandler::AboutData aboutData;

    //NearbyDeviceStruct(qcc::String const& mBusName, unsigned short mVersion, unsigned short mPort, const AnnounceHandler::ObjectDescriptions& mObjectDescs, const AnnounceHandler::AboutData& mAboutData) : busName(mBusName), version(mVersion), port(mPort), objectDescs(mObjectDescs), aboutData(mAboutData) {}

} NearbyDeviceStruct;

static std::vector<NearbyDeviceStruct> nearbyDevices;
*/

CommandLineController* commandLineController = 0;

void exitApp(int32_t signum)
{
    std::cout << "Program Finished" << std::endl;

    if(commandLineController != NULL)
        commandLineController->shutdown();    
/*
    if (bus && announceHandler) {
        AnnouncementRegistrar::UnRegisterAnnounceHandler(*bus, *announceHandler);
    }

    if (controlPanelService) {
        controlPanelService->shutdownController();
        delete controlPanelService;
    }
    if (controlPanelController) {
        delete controlPanelController;
    }
    if (controlPanelListener) {
        delete controlPanelListener;
    }
    if (announceHandler) {
        delete announceHandler;
    }
    if (srpKeyXListener) {
        delete srpKeyXListener;
    }
    if (bus) {
        delete bus;
    }
*/
    std::cout << "Peace!" << std::endl;
    exit(signum);
}

/*static void announceHandlerCallback(qcc::String const& busName, unsigned short version, unsigned short port,
                                    const AnnounceHandler::ObjectDescriptions& objectDescs, const AnnounceHandler::AboutData& aboutData)
{
    nearbyDevices.push_back(NearbyDeviceStruct());
    size_t index = nearbyDevices.size() - 1;
    nearbyDevices[index].busName = busName;
    nearbyDevices[index].version = version;
    nearbyDevices[index].port = port;
    nearbyDevices[index].objectDescs = objectDescs;
    nearbyDevices[index].aboutData = aboutData;

    //controlPanelController->createControllableDevice(busName, objectDescs);
}
*/

int main()
{
    // Allow CTRL+C to end application
    signal(SIGINT, exitApp);
    std::cout << "Beginning ControlPanel Application. (Press CTRL+C to end application)" << std::endl;

    commandLineController = new CommandLineController();
    commandLineController->initialize();
}

/*
int main()
{
    QStatus status;

    // Allow CTRL+C to end application
    signal(SIGINT, exitApp);
    std::cout << "Beginning ControlPanel Application. (Press CTRL+C to end application)" << std::endl;

    // Initialize Service objects
#ifdef QCC_USING_BD
    PasswordManager::SetCredentials("ALLJOYN_PIN_KEYX", "000000");
#endif

    controlPanelService = ControlPanelService::getInstance();
    controlPanelService->setLogLevel(Log::LogLevel::LEVEL_INFO);

    srpKeyXListener = new SrpKeyXListener();

    bus = CommonSampleUtil::prepareBusAttachment(srpKeyXListener);
    if (bus == NULL) {
        std::cout << "Could not initialize BusAttachment." << std::endl;
        exitApp(1);
    }

    controlPanelController = new ControlPanelController();
    controlPanelListener = new ControlPanelListenerImpl(controlPanelController);

    status = controlPanelService->initController(bus, controlPanelController, controlPanelListener);
    if (status != ER_OK) {
        std::cout << "Could not initialize Controller." << std::endl;
        exitApp(1);
    }

    announceHandler = new AnnounceHandlerImpl(NULL, announceHandlerCallback);
    AnnouncementRegistrar::RegisterAnnounceHandler(*bus, *announceHandler);

    status = CommonSampleUtil::addSessionlessMatch(bus);
    if (status != ER_OK) {
        std::cout << "Could not add Sessionless Match" << std::endl;
        exitApp(1);
    }

    std::cout << "Finished setup. Waiting for Contollees" << std::endl;
    while (1) {
        //sleep(1);

        int selection;

        std::cout << "Enter 1 to show nearby devices:";
        std::cin >> selection;

        if(selection == 1) {
            int nearbyDeviceIndex = 0;
            std::cout << "Choose a nearby device to interact with..." << '\n';
            for (std::vector<NearbyDeviceStruct>::iterator it = nearbyDevices.begin() ; it != nearbyDevices.end(); ++it) {
                std::cout << nearbyDeviceIndex << " ==> " << it->busName.c_str() << '\n';
                nearbyDeviceIndex++;
            }
            std::cin >> selection;

            // Establish a connection with the nearby device
            controlPanelController->createControllableDevice(nearbyDevices[selection].busName, nearbyDevices[selection].objectDescs);
        }
        
    }
}
*/


