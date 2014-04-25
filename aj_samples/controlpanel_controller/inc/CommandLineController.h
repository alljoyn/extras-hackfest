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

#ifndef COMMANDLINECONTROLLER_H_
#define COMMANDLINECONTROLLER_H_

#include <alljoyn/controlpanel/ControlPanelListener.h>
#include <alljoyn/controlpanel/ControlPanelController.h>
#include <alljoyn/about/AnnouncementRegistrar.h>
#include <alljoyn/controlpanel/ControlPanelService.h>
#include <alljoyn/controlpanel/ControlPanelController.h>
#include <alljoyn/controlpanel/Container.h>
#include <alljoyn/controlpanel/Property.h>
#include <alljoyn/controlpanel/Dialog.h>
#include <alljoyn/controlpanel/Action.h>
#include <alljoyn/services_common/GenericLogger.h>
#include <SrpKeyXListener.h>
#include <CommonSampleUtil.h>
#include <AnnounceHandlerImpl.h>

typedef struct NearbyDeviceStruct {
    
    qcc::String busName;
    unsigned short version;
    unsigned short port;
    ajn::services::AnnounceHandler::ObjectDescriptions objectDescs;
    ajn::services::AnnounceHandler::AboutData aboutData;

    //NearbyDeviceStruct(qcc::String const& mBusName, unsigned short mVersion, unsigned short mPort, const AnnounceHandler::ObjectDescriptions& mObjectDescs, const AnnounceHandler::AboutData& mAboutData) : busName(mBusName), version(mVersion), port(mPort), objectDescs(mObjectDescs), aboutData(mAboutData) {}

} NearbyDeviceStruct;

/*
 *
 */
class CommandLineController : public ajn::services::ControlPanelListener, public ajn::services::AnnounceHandler {
  public:

    CommandLineController();

    ~CommandLineController();

    void initialize();

    void showNearbyDevices();

    void showUnits();

    void showControlPanels();

    void showControlPanel();

    void interactWithPropertyWidget();

    void interactWithActionWidget();

    void shutdown();

    void announceHandlerCallback(qcc::String const& busName, unsigned short version, unsigned short port, const ajn::services::AnnounceHandler::ObjectDescriptions& objectDescs, const ajn::services::AnnounceHandler::AboutData& aboutData);

    virtual void Announce(unsigned short version, unsigned short port, const char* busName, const ObjectDescriptions& objectDescs,
                          const AboutData& aboutData);

    //START ControlPanelListener Callbacks
    void sessionEstablished(ajn::services::ControlPanelDevice* device);

    void sessionLost(ajn::services::ControlPanelDevice* device);

    void errorOccured(ajn::services::ControlPanelDevice* device, QStatus status,
                      ajn::services::ControlPanelTransaction transaction, qcc::String const& error);

    void signalPropertiesChanged(ajn::services::ControlPanelDevice* device, ajn::services::Widget* widget);

    void signalPropertyValueChanged(ajn::services::ControlPanelDevice* device, ajn::services::Property* property);

    void signalDismiss(ajn::services::ControlPanelDevice* device, ajn::services::NotificationAction* notificationAction);
    //END ControlPanelListener Callbacks

  private:

    void getNewValueConstraintRange(ajn::services::ConstraintRange* constraintRange, ajn::services::PropertyType propertyType);

    void getNewValueConstraintList(const std::vector<ajn::services::ConstraintList>& constraintList, ajn::services::PropertyType propertyType);

    void printRootWidget(ajn::services::RootWidget* rootWidget);

    void printPropertyValue(ajn::services::PropertyValue propertyValue, ajn::services::PropertyType propertyType, qcc::String const& indent = "  ");

    void printErrorWidget(ajn::services::Widget* widget, qcc::String const& indent);

    void printBasicWidget(ajn::services::Widget* widget, qcc::String const& widgetType, qcc::String const& indent);

    void printContainer(ajn::services::Container* rootContainer, std::vector<ajn::services::Action*>& actionsToExecute,
                               std::vector<ajn::services::Dialog*>& dialogsToExecute, std::vector<ajn::services::Property*>& propertiesToChange,
                               qcc::String const& indent = "");

    void printProperty(ajn::services::Property* property, qcc::String const& indent);

    void printDialog(ajn::services::Dialog* dialog, qcc::String const& indent);

    void printConstraintRange(ajn::services::ConstraintRange* constraintRange, ajn::services::PropertyType propertyType,
                                     qcc::String const& indent);

    void printConstraintList(const std::vector<ajn::services::ConstraintList>& constraintList, ajn::services::PropertyType propertyType,
                                    qcc::String const& indent);

    ajn::services::ControlPanelController* m_Controller;

    std::vector<qcc::String> m_ConnectedDevices;

    ajn::BusAttachment* bus;
    ajn::services::ControlPanelService* controlPanelService;
    ajn::services::ControlPanelController* controlPanelController;
    SrpKeyXListener* srpKeyXListener;
    AnnounceHandlerImpl* announceHandler;
    qcc::String ControlPanelPrefix;

    std::vector<NearbyDeviceStruct> nearbyDevices;
    ajn::services::ControlPanelDevice* currentCPDevice;
    ajn::services::ControlPanelControllerUnit* currentCPUnit;
    ajn::services::ControlPanel* currentCP;
    qcc::String currentLanguage;

    int currentState;

    std::vector<ajn::services::Action*> actionsToExecute;
    std::vector<ajn::services::Property*> propertiesToChange;
    std::vector<ajn::services::Dialog*> dialogsToExecute;

    ajn::services::Property* propertyToInteract;
    ajn::services::Action* actionToInteract;

};

#endif /* COMMANDLINECONTROLLER_H_ */
