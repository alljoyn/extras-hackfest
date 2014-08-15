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

#include "CommandLineController.h"
#include "ControllerUtil.h"
#include <alljoyn/controlpanel/ControlPanel.h>
#include <alljoyn/controlpanel/ActionWithDialog.h>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <cstdio>

#define SERVICE_PORT 900

using namespace ajn;
using namespace services;
using namespace std;

/////////////////////////////////
// STATE MACHINE
// 
// 0 == Not connected, show nearby devices & choose one to connect to
// 
// 1 == Trying to connect to a nearby device
//
// 2 == Connected, show units, choose one
//
// 3 == For chosen unit, show control panels, choose language
//
// 4 == Show containers & widgets, can hit 'R' to refresh, chose one to interact with
//
// 5 == Property widget chosen, show option for interacting 
//
// 6 == Action widget chosen, show options for interacting
/////////////////////////////////

CommandLineController::CommandLineController()
{
}

CommandLineController::~CommandLineController()
{
}

void CommandLineController::initialize() 
{
    QStatus status;

    ControlPanelPrefix = "/ControlPanel/";

    currentState = 0;
    currentCPDevice = 0;
    currentCPUnit = 0;
    currentCP = 0;

    // Initialize Service objects
#ifdef QCC_USING_BD
    PasswordManager::SetCredentials("ALLJOYN_PIN_KEYX", "000000");
#endif

    controlPanelService = ControlPanelService::getInstance();

    srpKeyXListener = new SrpKeyXListener();

    bus = CommonSampleUtil::prepareBusAttachment(srpKeyXListener);
    if (bus == NULL) {
        std::cout << "Could not initialize BusAttachment." << std::endl;
        shutdown();
        return;
    }

    controlPanelController = new ControlPanelController();

    status = controlPanelService->initController(bus, controlPanelController, this);
    if (status != ER_OK) {
        std::cout << "Could not initialize Controller." << std::endl;
        shutdown();
        return;
    }

    AnnouncementRegistrar::RegisterAnnounceHandler(*bus, *this);

    status = CommonSampleUtil::addSessionlessMatch(bus);
    if (status != ER_OK) {
        std::cout << "Could not add Sessionless Match" << std::endl;
        shutdown();
        return;
    }

    while (1) 
    {
        switch (currentState)
        {
            case 0:
                showNearbyDevices(); 
                break;
            case 1:
                // trying to connect to a nearby device
                break;
            case 2:
                showUnits();
                break;
            case 3:
                showControlPanels();
                break;
            case 4:
                showControlPanel();
                break;
            case 5:
                interactWithPropertyWidget();
                break;
            case 6:
                interactWithActionWidget();
                break;
            default:
                sleep(1);
                break;
        }       
    }
}

void CommandLineController::showNearbyDevices()
{
    int selection;
    int nearbyDeviceIndex = 0;
    string input;

    //if we were previously connected to a control panel, disconnect from it
    if(currentCPDevice)
    {
        if (controlPanelController) {
            QStatus status = controlPanelController->deleteControllableDevice(currentCPDevice->getDeviceBusName());
            //std::cout << "    Deleting Controllable Device " << (status == ER_OK ? "succeeded" : "failed") << std::endl;
            currentCPDevice = 0;
        }
    }

    std::map<qcc::String, ajn::MsgArg>::iterator about_data_iter;

    std::cout << "====================================================\n";
    std::cout << "Choose a nearby device to interact with: \n";
    for (std::vector<NearbyDeviceStruct>::iterator it = nearbyDevices.begin() ; it != nearbyDevices.end(); ++it) 
    {
        std::cout << nearbyDeviceIndex+1 << " ==> ";
        about_data_iter = it->aboutData.find("AppName");
        std::cout << about_data_iter->second.v_string.str;
        std::cout << " (" << it->busName.c_str() << ")\n";
        nearbyDeviceIndex++;
    }
    std::cout << "r ==> refresh \n";
    std::cout << "\n";
    std::cout << "Enter selection: ";
    
    getline(cin,input);
    
    if(input.compare("r") == 0 || input.compare("R") == 0)
    {
        return;
    }

    // Establish a connection with the nearby device
    stringstream(input) >> selection;

    if( (selection >= 1) && (selection <= nearbyDeviceIndex) )
    { 
        std::cout << "\nConnecting to nearby device ..." << '\n';
        currentState = 1; //nearby device chosen, now trying to connect
        controlPanelController->createControllableDevice(nearbyDevices[selection-1].busName, nearbyDevices[selection-1].objectDescs);
    }
    else
    {
        //user-entered input is invalid
        std::cout << "!!Unrecognized input, returning to previous screen!! \n";
    }

}

void CommandLineController::showUnits()
{
    std::map<qcc::String, ControlPanelControllerUnit*> units = currentCPDevice->getDeviceUnits();
    std::map<qcc::String, ControlPanelControllerUnit*>::iterator iter;

    m_ConnectedDevices.push_back(currentCPDevice->getDeviceBusName());

    string input;
    int selection;
    int unitIndex = 0;
    
    std::cout << "====================================================\n";
    std::cout << "Choose a unit to interact with: \n";
    for (iter = units.begin(); iter != units.end(); iter++) {
        std::cout << unitIndex+1 << " ==> " << iter->first.c_str() << '\n';
        unitIndex++;       
    }
    std::cout << "b ==> go back \n";
    std::cout << "\n";
    std::cout << "Enter selection: ";
    getline(cin,input);

    if(input.compare("b") == 0 || input.compare("B") == 0)
    {
        // need to disconnect from the device we connected to...
        currentState = 0;
        return;
    }
    stringstream(input) >> selection;
    
    if( (selection >= 1) && (selection <= unitIndex) )
    {
        unitIndex = 1; //start at 1 because we display starting at 1
        for (iter = units.begin(); iter != units.end(); iter++) {
            if(unitIndex == selection) 
            {
                currentCPUnit = iter->second;
                currentState = 3;
                break;
            }
            unitIndex++;       
        }
    }
    else
    {
        //user-entered input is invalid
        std::cout << "!!Unrecognized input, returning to previous screen!! \n";
    }

}

void CommandLineController::showControlPanels() 
{
    std::map<qcc::String, ControlPanel*>::iterator it;
    std::map<qcc::String, ControlPanel*> controlPanels = currentCPUnit->getControlPanels();

    int selection;
    string input;
    int cpIndex = 0;
    
    std::cout << "====================================================\n";
    std::cout << "Choose a control panel to interact with: \n";
    for (it = controlPanels.begin(); it != controlPanels.end(); it++) {
        std::vector<qcc::String> languages = it->second->getLanguageSet().getLanguages();
        for (size_t i = 0; i < languages.size(); i++) {
            std::cout << cpIndex+1 << " ==> " << it->first.c_str() << " (" << languages[i].c_str() << ")" << std::endl;
            cpIndex++;
        }
    }
    std::cout << "b ==> go back \n";
    std::cout << "\n";
    std::cout << "Enter selection: ";
    getline(cin,input);

    if(input.compare("b") == 0 || input.compare("B") == 0)
    {
        // go back to the 'choose a unit' section
        currentState = 2;
        return;
    }
    stringstream(input) >> selection;

    if( (selection >= 1) && (selection <= cpIndex) )
    {
        cpIndex = 1; //start at 1 because we display starting at 1
        for (it = controlPanels.begin(); it != controlPanels.end(); it++) {
            std::vector<qcc::String> languages = it->second->getLanguageSet().getLanguages();
            for (size_t i = 0; i < languages.size(); i++) {
                if(cpIndex == selection) 
                {
                    currentCP = it->second;
                    currentLanguage = languages[i].c_str();
                    currentState = 4;
                    break;
                }            

                cpIndex++;
            }
        }
    }
    else
    {
        //user-entered input is invalid
        std::cout << "!!Unrecognized input, returning to previous screen!! \n";
    }

}

void CommandLineController::showControlPanel()
{
    Container* rootContainer = currentCP->getRootWidget(currentLanguage);
    
    std::cout << "====================================================\n";
    printRootWidget(rootContainer);
    
    int selection;
    string input;

    std::cout << "====================================================\n";
    std::cout << "Choose a widget to interact with: \n";    

    int index = 1;
    for (size_t i = 0; i < propertiesToChange.size(); i++) 
    {
        std::cout << index << " ==> (Property) " << propertiesToChange[i]->getWidgetName().c_str() << std::endl;
        index++;
    }
    for (size_t i = 0; i < actionsToExecute.size(); i++) 
    {
        std::cout << index << " ==> (Action) " << actionsToExecute[i]->getWidgetName().c_str() << std::endl;
        index++;
    }

    std::cout << "b ==> go back \n";
    std::cout << "r ==> refresh \n";
    std::cout << "\n";
    std::cout << "Enter selection: ";
    getline(cin,input);

    if(input.compare("b") == 0 || input.compare("B") == 0)
    {
        // go back to the 'choose a cp' section
        currentState = 3;
        return;
    }
    if(input.compare("r") == 0 || input.compare("R") == 0)
    {
        // need to refresh the control panel
        currentState = 4;
        return;
    }

    stringstream(input) >> selection;

    if( (selection >= 1) && (selection <= index) )
    {
        index = 1;
        for (size_t i = 0; i < propertiesToChange.size(); i++) 
        {
            if(index == selection) 
            {
                propertyToInteract = propertiesToChange[i];
                currentState = 5;
                return;
            }
            index++;
        }
        for (size_t i = 0; i < actionsToExecute.size(); i++) 
        {
            if(index == selection) 
            {
                actionToInteract = actionsToExecute[i];
                currentState = 6;
                return;
            }
            index++;
        }
    }
    else
    {
        //user-entered input is invalid
        std::cout << "!!Unrecognized input, returning to previous screen!! \n";
    }
}

void CommandLineController::interactWithPropertyWidget()
{
    qcc::String indent = "";
    std::cout << "*** Interaction with Property Widget ***\n";

    printBasicWidget(propertyToInteract, "Property", "");
    printPropertyValue(propertyToInteract->getPropertyValue(), propertyToInteract->getPropertyType(), indent);

    if (propertyToInteract->getConstraintRange()) {
        std::cout << "====================================================\n";
        printConstraintRange(propertyToInteract->getConstraintRange(), propertyToInteract->getPropertyType(), indent + "  ");

        getNewValueConstraintRange(propertyToInteract->getConstraintRange(), propertyToInteract->getPropertyType());
    }
    else if (propertyToInteract->getConstraintList().size()) {
        std::cout << "====================================================\n";
        printConstraintList(propertyToInteract->getConstraintList(), propertyToInteract->getPropertyType(), indent + "  ");

        getNewValueConstraintList(propertyToInteract->getConstraintList(), propertyToInteract->getPropertyType());
    }
    else {
        // no constraints
    }    

    currentState = 4;
}

void CommandLineController::interactWithActionWidget()
{
    qcc::String indent = "";
    std::cout << "*** Interaction with Action Widget ***\n";
    
    int selection;
    string input;

    printBasicWidget(actionToInteract, "Action", "");

    std::cout << "====================================================\n";    
    std::cout << "1 ==> execute action " << std::endl;
    std::cout << "b ==> go back \n";
    std::cout << "\n";
    std::cout << "Enter selection: ";
    getline(cin,input);
    
    if(input.compare("b") == 0 || input.compare("B") == 0)
    {
        currentState = 4;
        return;
    }

    stringstream(input) >> selection;

    if(selection == 1)
    {
        QStatus status = actionToInteract->executeAction();
        //std::cout << "    Action execution " << (status == ER_OK ? "succeeded" : "failed") << std::endl;  
    }
    else 
    {
        std::cout << "!!Unrecognized input, returning to previous screen!! \n";
    }

    currentState = 4;
}

void CommandLineController::shutdown()
{
    if (bus && announceHandler) {
        AnnouncementRegistrar::UnRegisterAnnounceHandler(*bus, *this);
    }

    if (controlPanelService) {
        controlPanelService->shutdownController();
        delete controlPanelService;
    }
    if (controlPanelController) {
        delete controlPanelController;
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
}

void CommandLineController::Announce(unsigned short version, unsigned short port, const char* busName, const ObjectDescriptions& objectDescs,
                                   const AboutData& aboutData)
{
    nearbyDevices.push_back(NearbyDeviceStruct());
    size_t index = nearbyDevices.size() - 1;
    nearbyDevices[index].busName = busName;
    nearbyDevices[index].version = version;
    nearbyDevices[index].port = port;
    nearbyDevices[index].objectDescs = objectDescs;
    nearbyDevices[index].aboutData = aboutData;
}

void CommandLineController::sessionEstablished(ControlPanelDevice* device)
{
    currentCPDevice = device;
    currentState = 2;

    std::cout << "Connection to nearby device successful!" << std::endl;
}

void CommandLineController::sessionLost(ControlPanelDevice* device)
{
    std::cout << "Received sessionLost for device " << device->getDeviceBusName().c_str() << std::endl;
    std::cout << "Sleeping 5 seconds before cleaning up device" << std::endl;
    sleep(5);

    std::vector<qcc::String>::iterator iter;
    iter = find(m_ConnectedDevices.begin(), m_ConnectedDevices.end(), device->getDeviceBusName());
    if (iter != m_ConnectedDevices.end()) {
        m_ConnectedDevices.erase(iter);
    }

    if (m_Controller) {
        QStatus status = m_Controller->deleteControllableDevice(device->getDeviceBusName());
        std::cout << "    Deleting Controllable Device " << (status == ER_OK ? "succeeded" : "failed") << std::endl;
    }
}

void CommandLineController::errorOccured(ControlPanelDevice* device, QStatus status, ControlPanelTransaction transaction,
                                            qcc::String const& error)
{
    std::cout << "Received an error from service for device " << device->getDeviceBusName().c_str() << std::endl;
    std::cout << "    Error Message: " << error.c_str() << std::endl;
}

void CommandLineController::signalPropertiesChanged(ControlPanelDevice* device, Widget* widget)
{
    // not displaying these as it interrupts the visual flow
    //std::cout << "Received PropertiesChanged Signal for Widget " << widget->getWidgetName().c_str() << std::endl;
}

void CommandLineController::signalPropertyValueChanged(ControlPanelDevice* device, Property* property)
{
    // not displaying these as it interrupts the visual flow
    //std::cout << "Received ValueChanged Signal for Widget " << property->getWidgetName().c_str() << std::endl;
    //ControllerUtil::printPropertyValue(property->getPropertyValue(), property->getPropertyType());
}

void CommandLineController::signalDismiss(ControlPanelDevice* device, NotificationAction* notificationAction)
{
    std::cout << "Received Dismiss Signal for NotificationAction" << std::endl;
}

void CommandLineController::getNewValueConstraintRange(ConstraintRange* constraintRange, PropertyType propertyType)
{
        string input;        
        QStatus status = ER_OK;  

        std::cout << "\n";
        std::cout << "Enter a new value from the constraint range: ";
        getline(cin,input);

        switch (propertyType) {
        case UINT16_PROPERTY:
            uint16_t new_uint16;
            stringstream(input) >> new_uint16;
            if( (new_uint16 >= constraintRange->getMinValue().uint16Value) && (new_uint16 <= constraintRange->getMaxValue().uint16Value) )
                status = propertyToInteract->setValue(new_uint16);
            else
                std::cout << "!!Value NOT in constraint range, returning to previous screen!! \n";
            break;

        case INT16_PROPERTY:
            int16_t new_int16;
            stringstream(input) >> new_int16;
            if( (new_int16 >= constraintRange->getMinValue().int16Value) && (new_int16 <= constraintRange->getMaxValue().int16Value) )
                status = propertyToInteract->setValue(new_int16);
            else
                std::cout << "!!Value NOT in constraint range, returning to previous screen!! \n";
            break;

        case UINT32_PROPERTY:
            uint32_t new_uint32;
            stringstream(input) >> new_uint32;
            if( (new_uint32 >= constraintRange->getMinValue().uint32Value) && (new_uint32 <= constraintRange->getMaxValue().uint32Value) )
                status = propertyToInteract->setValue(new_uint32);
            else
                std::cout << "!!Value NOT in constraint range, returning to previous screen!! \n";
            break;

        case INT32_PROPERTY:
            int32_t new_int32;
            stringstream(input) >> new_int32;
            if( (new_int32 >= constraintRange->getMinValue().int32Value) && (new_int32 <= constraintRange->getMaxValue().int32Value) )
                status = propertyToInteract->setValue(new_int32);
            else
                std::cout << "!!Value NOT in constraint range, returning to previous screen!! \n";
            break;

        case UINT64_PROPERTY:
            uint64_t new_uint64;
            stringstream(input) >> new_uint64;
            if( (new_uint64 >= constraintRange->getMinValue().uint64Value) && (new_uint64 <= constraintRange->getMaxValue().uint64Value) )
                status = propertyToInteract->setValue(new_uint64);
            else
                std::cout << "!!Value NOT in constraint range, returning to previous screen!! \n";
            break;

        case INT64_PROPERTY:
            int64_t new_int64;
            stringstream(input) >> new_int64;
            if( (new_int64 >= constraintRange->getMinValue().int64Value) && (new_int64 <= constraintRange->getMaxValue().int64Value) )
                status = propertyToInteract->setValue(new_int64);
            else
                std::cout << "!!Value NOT in constraint range, returning to previous screen!! \n";
            break;

        case DOUBLE_PROPERTY:
            double new_double;
            stringstream(input) >> new_double;
            if( (new_double >= constraintRange->getMinValue().doubleValue) && (new_double <= constraintRange->getMaxValue().doubleValue) )
                status = propertyToInteract->setValue(new_double);
            else
                std::cout << "!!Value NOT in constraint range, returning to previous screen!! \n";
            break;

        default:
            break;
        }

}

void CommandLineController::getNewValueConstraintList(const std::vector<ConstraintList>& constraintList, PropertyType propertyType)
{
        string input;
        string new_string;        
        QStatus status = ER_OK;
        bool validated = false;

        std::cout << "\n";
        std::cout << "Enter a new value from the constraint list: ";
        getline(cin,input);

        switch (propertyType) {
        case UINT16_PROPERTY:
            uint16_t new_uint16;
            stringstream(input) >> new_uint16;

            for (size_t i = 0; i < constraintList.size(); i++) {
                if(new_uint16 == constraintList[i].getConstraintValue().uint16Value) {
                    validated = true;
                    break;
                }               
            }

            if(validated)
                status = propertyToInteract->setValue(new_uint16);
            else
                std::cout << "!!Value NOT in constraint list, returning to previous screen!! \n";
            break;

        case INT16_PROPERTY:
            int16_t new_int16;
            stringstream(input) >> new_int16;

            for (size_t i = 0; i < constraintList.size(); i++) {
                if(new_int16 == constraintList[i].getConstraintValue().int16Value) {
                    validated = true;
                    break;
                }               
            }

            if(validated)
                status = propertyToInteract->setValue(new_int16);
            else
                std::cout << "!!Value NOT in constraint list, returning to previous screen!! \n";
            break;

        case UINT32_PROPERTY:
            uint32_t new_uint32;
            stringstream(input) >> new_uint32;

            for (size_t i = 0; i < constraintList.size(); i++) {
                if(new_uint32 == constraintList[i].getConstraintValue().uint32Value) {
                    validated = true;
                    break;
                }               
            }

            if(validated)
                status = propertyToInteract->setValue(new_uint32);
            else
                std::cout << "!!Value NOT in constraint list, returning to previous screen!! \n";
            break;

        case INT32_PROPERTY:
            int32_t new_int32;
            stringstream(input) >> new_int32;

            for (size_t i = 0; i < constraintList.size(); i++) {
                if(new_int32 == constraintList[i].getConstraintValue().int32Value) {
                    validated = true;
                    break;
                }               
            }

            if(validated)
                status = propertyToInteract->setValue(new_int32);
            else
                std::cout << "!!Value NOT in constraint list, returning to previous screen!! \n";
            break;

        case UINT64_PROPERTY:
            uint64_t new_uint64;
            stringstream(input) >> new_uint64;

            for (size_t i = 0; i < constraintList.size(); i++) {
                if(new_uint64 == constraintList[i].getConstraintValue().uint64Value) {
                    validated = true;
                    break;
                }               
            }

            if(validated)
                status = propertyToInteract->setValue(new_uint64);
            else
                std::cout << "!!Value NOT in constraint list, returning to previous screen!! \n";
            break;

        case INT64_PROPERTY:
            int64_t new_int64;
            stringstream(input) >> new_int64;

            for (size_t i = 0; i < constraintList.size(); i++) {
                if(new_int64 == constraintList[i].getConstraintValue().int64Value) {
                    validated = true;
                    break;
                }               
            }

            if(validated)
                status = propertyToInteract->setValue(new_int64);
            else
                std::cout << "!!Value NOT in constraint list, returning to previous screen!! \n";
            break;

        case DOUBLE_PROPERTY:
            double new_double;
            stringstream(input) >> new_double;

            for (size_t i = 0; i < constraintList.size(); i++) {
                if(new_double == constraintList[i].getConstraintValue().doubleValue) {
                    validated = true;
                    break;
                }               
            }

            if(validated)
                status = propertyToInteract->setValue(new_double);
            else
                std::cout << "!!Value NOT in constraint list, returning to previous screen!! \n";
            break;

        case STRING_PROPERTY:
            stringstream(input) >> new_string;

            for (size_t i = 0; i < constraintList.size(); i++) {
                if(new_string.compare(constraintList[i].getConstraintValue().charValue) == 0) {
                    validated = true;
                    break;
                }               
            }

            if(validated)
                status = propertyToInteract->setValue(new_string.c_str());
            else
                std::cout << "!!Value NOT in constraint list, returning to previous screen!! \n";
            break;

        default:
            break;
        }

}

//////////////////////////////////////////////////////////////////////////////////////////


void CommandLineController::printRootWidget(RootWidget* rootWidget)
{
    if (!rootWidget) {
        return;
    }

    actionsToExecute.clear();
    propertiesToChange.clear();
    dialogsToExecute.clear();

    if (rootWidget->getWidgetType() == CONTAINER) {

        printContainer((Container*)rootWidget, actionsToExecute, dialogsToExecute, propertiesToChange);

    } else if (rootWidget->getWidgetType() == DIALOG) {

        printDialog((Dialog*)rootWidget, "");

    } else {
        std::cout << "RootWidget is of unknown type." << std::endl;
    }
}

void CommandLineController::printContainer(Container* container, std::vector<Action*>& actionsToExecute, std::vector<Dialog*>& dialogsToExecute,
                                    std::vector<Property*>& propertiesToChange, qcc::String const& indent)
{
    printBasicWidget(container, "Container", indent);

    std::vector<Widget*> childWidgets = container->getChildWidgets();
    
    for (size_t i = 0; i < childWidgets.size(); i++) {
        WidgetType widgetType = childWidgets[i]->getWidgetType();
        switch (widgetType) {
        case ACTION:
            printBasicWidget(childWidgets[i], "Action", indent + "  ");
            if ( childWidgets[i]->getIsEnabled() )
            {
                // only allow user to chose if action is enabled
                actionsToExecute.push_back((Action*)childWidgets[i]);
            }
            break;

        case ACTION_WITH_DIALOG:
            printBasicWidget(childWidgets[i], "Action", indent + "  ");
            std::cout << indent.c_str() << "  Printing ChildDialog: " << std::endl;
            printDialog(((ActionWithDialog*)childWidgets[i])->getChildDialog(), indent + "    ");
            dialogsToExecute.push_back(((ActionWithDialog*)childWidgets[i])->getChildDialog());
            break;

        case LABEL:
            printBasicWidget(childWidgets[i], "Label", indent + "  ");
            break;

        case PROPERTY:
            printProperty(((Property*)childWidgets[i]), indent + "  ");
            if ( ((Property*)childWidgets[i])->getIsWritable() )
            {
                // only allow user to chose if property is writable
                propertiesToChange.push_back((Property*)childWidgets[i]);
            }
            break;

        case CONTAINER:
            printContainer(((Container*)childWidgets[i]), actionsToExecute, dialogsToExecute, propertiesToChange, indent + "  ");
            break;

        case DIALOG:
            printDialog(((Dialog*)childWidgets[i]), indent + "  ");
            break;

        case ERROR:
            printErrorWidget(childWidgets[i], indent + "  ");
            break;
        }

        std::cout << std::endl;
    }
}

void CommandLineController::printBasicWidget(Widget* widget, qcc::String const& widgetType, qcc::String const& indent)
{
    std::cout << indent.c_str() << widgetType.c_str() << " name: " << widget->getWidgetName().c_str() << std::endl;
    //std::cout << indent.c_str() << widgetType.c_str() << " version: " << widget->getInterfaceVersion() << std::endl;
    //std::cout << indent.c_str() << widgetType.c_str() << " is " << (widget->getIsSecured() ? "secured" : "not secured") << std::endl;
    //std::cout << indent.c_str() << widgetType.c_str() << " is " << (widget->getIsEnabled() ? "enabled" : "not enabled") << std::endl;
    if (widget->getLabel().size()) {
        std::cout << indent.c_str() << widgetType.c_str() << " label: " << widget->getLabel().c_str() << std::endl;
    }
    //if (widget->getBgColor() != UINT32_MAX) {
    //    std::cout << indent.c_str() << widgetType.c_str() << " bgColor: " << widget->getBgColor() << std::endl;
    //}
    //printHints(widget, widgetType, indent);
}

void CommandLineController::printProperty(Property* property, qcc::String const& indent)
{
    printBasicWidget(property, "Property", indent);
    printPropertyValue(property->getPropertyValue(), property->getPropertyType(), indent);
    std::cout << indent.c_str() << "Property is " << (property->getIsWritable() ? "writable" : "not writable") << std::endl;

    //if (property->getUnitOfMeasure().size()) {
    //    std::cout << indent.c_str() << "Property unitOfMeasure: " << property->getUnitOfMeasure().c_str() << std::endl;
    // }
    if (property->getConstraintRange()) {
        std::cout << indent.c_str() << "Property has ConstraintRange: " << std::endl;
        printConstraintRange(property->getConstraintRange(), property->getPropertyType(), indent + "  ");
    }
    if (property->getConstraintList().size()) {
        std::cout << indent.c_str() << "Property has ConstraintList: " << std::endl;
        printConstraintList(property->getConstraintList(), property->getPropertyType(), indent + "  ");
    }
}

void CommandLineController::printDialog(Dialog* dialog, qcc::String const& indent)
{
    printBasicWidget(dialog, "Dialog", indent);
    std::cout << indent.c_str() << "Dialog message: " << dialog->getMessage().c_str() << std::endl;
    std::cout << indent.c_str() << "Dialog numActions: " << dialog->getNumActions() << std::endl;
    if (dialog->getLabelAction1().size()) {
        std::cout << indent.c_str() << "Dialog Label for Action1: " << dialog->getLabelAction1().c_str() << std::endl;
    }
    if (dialog->getLabelAction2().size()) {
        std::cout << indent.c_str() << "Dialog Label for Action2: " << dialog->getLabelAction2().c_str() << std::endl;
    }
    if (dialog->getLabelAction3().size()) {
        std::cout << indent.c_str() << "Dialog Label for Action3: " << dialog->getLabelAction3().c_str() << std::endl;
    }
}

void CommandLineController::printErrorWidget(Widget* widget, qcc::String const& indent)
{
    std::cout << indent.c_str() << "Received error widget with name: " << widget->getWidgetName().c_str() << std::endl;
    if (widget->getLabel().size()) {
        std::cout << indent.c_str() << "ErrorWidget label: " << widget->getLabel().c_str() << std::endl;
    }
}

void CommandLineController::printConstraintList(const std::vector<ConstraintList>& constraintList, PropertyType propertyType, qcc::String const& indent)
{
    for (size_t i = 0; i < constraintList.size(); i++) {
        std::cout << indent.c_str() << "Display: " << constraintList[i].getDisplay().c_str();
        switch (propertyType) {
        case UINT16_PROPERTY:
            std::cout << " Value: " << constraintList[i].getConstraintValue().uint16Value << std::endl;
            break;

        case INT16_PROPERTY:
            std::cout << " Value: " << constraintList[i].getConstraintValue().int16Value << std::endl;
            break;

        case UINT32_PROPERTY:
            std::cout << " Value: " << constraintList[i].getConstraintValue().uint32Value << std::endl;
            break;

        case INT32_PROPERTY:
            std::cout << " Value: " << constraintList[i].getConstraintValue().int32Value << std::endl;
            break;

        case UINT64_PROPERTY:
            std::cout << " Value: " << constraintList[i].getConstraintValue().uint64Value << std::endl;
            break;

        case INT64_PROPERTY:
            std::cout << " Value: " << constraintList[i].getConstraintValue().int64Value << std::endl;
            break;

        case DOUBLE_PROPERTY:
            std::cout << " Value: " << constraintList[i].getConstraintValue().doubleValue << std::endl;
            break;

        case STRING_PROPERTY:
            std::cout << " Value: " << constraintList[i].getConstraintValue().charValue << std::endl;
            break;

        default:
            std::cout << " Value: " << "ConstraintList is unknown property type" << std::endl;
            break;
        }
    }
}

void CommandLineController::printConstraintRange(ConstraintRange* constraintRange, PropertyType propertyType, qcc::String const& indent)
{
    switch (propertyType) {
    case UINT16_PROPERTY:
        std::cout << indent.c_str() << "MinValue: " << constraintRange->getMinValue().uint16Value << std::endl;
        std::cout << indent.c_str() << "MaxValue: " << constraintRange->getMaxValue().uint16Value << std::endl;
        std::cout << indent.c_str() << "IncrementValue: " << constraintRange->getIncrementValue().uint16Value << std::endl;
        break;

    case INT16_PROPERTY:
        std::cout << indent.c_str() << "MinValue: " << constraintRange->getMinValue().int16Value << std::endl;
        std::cout << indent.c_str() << "MaxValue: " << constraintRange->getMaxValue().int16Value << std::endl;
        std::cout << indent.c_str() << "IncrementValue: " << constraintRange->getIncrementValue().int16Value << std::endl;
        break;

    case UINT32_PROPERTY:
        std::cout << indent.c_str() << "MinValue: " << constraintRange->getMinValue().uint32Value << std::endl;
        std::cout << indent.c_str() << "MaxValue: " << constraintRange->getMaxValue().uint32Value << std::endl;
        std::cout << indent.c_str() << "IncrementValue: " << constraintRange->getIncrementValue().uint32Value << std::endl;
        break;

    case INT32_PROPERTY:
        std::cout << indent.c_str() << "MinValue: " << constraintRange->getMinValue().int32Value << std::endl;
        std::cout << indent.c_str() << "MaxValue: " << constraintRange->getMaxValue().int32Value << std::endl;
        std::cout << indent.c_str() << "IncrementValue: " << constraintRange->getIncrementValue().int32Value << std::endl;
        break;

    case UINT64_PROPERTY:
        std::cout << indent.c_str() << "MinValue: " << constraintRange->getMinValue().uint64Value << std::endl;
        std::cout << indent.c_str() << "MaxValue: " << constraintRange->getMaxValue().uint64Value << std::endl;
        std::cout << indent.c_str() << "IncrementValue: " << constraintRange->getIncrementValue().uint64Value << std::endl;
        break;

    case INT64_PROPERTY:
        std::cout << indent.c_str() << "MinValue: " << constraintRange->getMinValue().int64Value << std::endl;
        std::cout << indent.c_str() << "MaxValue: " << constraintRange->getMaxValue().int64Value << std::endl;
        std::cout << indent.c_str() << "IncrementValue: " << constraintRange->getIncrementValue().int64Value << std::endl;
        break;

    case DOUBLE_PROPERTY:
        std::cout << indent.c_str() << "MinValue: " << constraintRange->getMinValue().doubleValue << std::endl;
        std::cout << indent.c_str() << "MaxValue: " << constraintRange->getMaxValue().doubleValue << std::endl;
        std::cout << indent.c_str() << "IncrementValue: " << constraintRange->getIncrementValue().doubleValue << std::endl;
        break;

    default:
        std::cout << indent.c_str() << "ConstraintRange is unknown property type" << std::endl;
        break;
    }
}

void CommandLineController::printPropertyValue(PropertyValue propertyValue, PropertyType propertyType, qcc::String const& indent)
{
    switch (propertyType) {
    case UINT16_PROPERTY:
        std::cout << indent.c_str() << "Property is a UINT16 Property." << std::endl;
        std::cout << indent.c_str() << "Property Value: " << propertyValue.uint16Value << std::endl;
        break;

    case INT16_PROPERTY:
        std::cout << indent.c_str() << "Property is a INT16 Property." << std::endl;
        std::cout << indent.c_str() << "Property Value: " << propertyValue.int16Value << std::endl;
        break;

    case UINT32_PROPERTY:
        std::cout << indent.c_str() << "Property is a UINT32 Property." << std::endl;
        std::cout << indent.c_str() << "Property Value: " << propertyValue.uint32Value << std::endl;
        break;

    case INT32_PROPERTY:
        std::cout << indent.c_str() << "Property is a INT32 Property." << std::endl;
        std::cout << indent.c_str() << "Property Value: " << propertyValue.int32Value << std::endl;
        break;

    case UINT64_PROPERTY:
        std::cout << indent.c_str() << "Property is a UINT64 Property." << std::endl;
        std::cout << indent.c_str() << "Property Value: " << propertyValue.uint64Value << std::endl;
        break;

    case INT64_PROPERTY:
        std::cout << indent.c_str() << "Property is a INT64 Property." << std::endl;
        std::cout << indent.c_str() << "Property Value: " << propertyValue.int64Value << std::endl;
        break;

    case DOUBLE_PROPERTY:
        std::cout << indent.c_str() << "Property is a DOUBLE Property." << std::endl;
        std::cout << indent.c_str() << "Property Value: " << propertyValue.doubleValue << std::endl;
        break;

    case STRING_PROPERTY:
        std::cout << indent.c_str() << "Property is a STRING Property." << std::endl;
        std::cout << indent.c_str() << "Property Value: " << propertyValue.charValue << std::endl;
        break;

    case BOOL_PROPERTY:
        std::cout << indent.c_str() << "Property is a BOOL Property." << std::endl;
        std::cout << indent.c_str() << "Property Value: " << (propertyValue.boolValue ? "true" : "false") << std::endl;
        break;

    case DATE_PROPERTY:
        std::cout << indent.c_str() << "Property is a Date Property." << std::endl;
        std::cout << indent.c_str() << "Property Value: " << propertyValue.dateValue->getDay() << "/" <<
        propertyValue.dateValue->getMonth() << "/" <<
        propertyValue.dateValue->getYear() << std::endl;
        break;

    case TIME_PROPERTY:
        std::cout << indent.c_str() << "Property is a Time Property." << std::endl;
        std::cout << indent.c_str() << "Property Value: " << propertyValue.timeValue->getHour() << ":" <<
        propertyValue.timeValue->getMinute() << ":" <<
        propertyValue.timeValue->getSecond() << std::endl;
        break;

    default:
        std::cout << indent.c_str() << "Property is unknown property type" << std::endl;
        break;
    }
}


