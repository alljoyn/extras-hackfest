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

#include "ControlPanelGenerated.h"
#include <alljoyn/controlpanel/LanguageSets.h>
#include <alljoyn/controlpanel/Label.h>
#include "../provided/ControlPanelProvided.h"



using namespace ajn;
using namespace services;

bool ControlPanelGenerated::languageSetsDone = false;
ControlPanelControlleeUnit* ControlPanelGenerated::myDeviceUnit = 0;
ControlPanel* ControlPanelGenerated::myDeviceRootContainerControlPanel = 0;
Container* ControlPanelGenerated::myDeviceRootContainer = 0;
Container* ControlPanelGenerated::myDeviceMessageContainer = 0;
MyDeviceSelected_message* ControlPanelGenerated::myDeviceSelected_message = 0;
MyDevicePrintMessage* ControlPanelGenerated::myDevicePrintMessage = 0;
Container* ControlPanelGenerated::myDeviceCounterContainer = 0;
MyDeviceCounterValueStringProperty* ControlPanelGenerated::myDeviceCounterValueStringProperty = 0;


#define CHECK(x) if ((status = x) != ER_OK) { return status; }

void ControlPanelGenerated::PrepareLanguageSets()
{
    if (languageSetsDone) {
        return;
    }

    LanguageSet myDeviceMyLanguages("myDeviceMyLanguages");
    myDeviceMyLanguages.addLanguage("en");
    LanguageSets::add(myDeviceMyLanguages.getLanguageSetName(), myDeviceMyLanguages);

    languageSetsDone = true;
}

QStatus ControlPanelGenerated::PrepareWidgets(ControlPanelControllee*& controlPanelControllee)
{
    if (controlPanelControllee) {
        return ER_BAD_ARG_1;
    }

    PrepareLanguageSets();

    QStatus status = ER_OK;
    controlPanelControllee = new ControlPanelControllee();

    myDeviceUnit = new ControlPanelControlleeUnit("MyDevice");
    CHECK(controlPanelControllee->addControlPanelUnit(myDeviceUnit));

    myDeviceRootContainerControlPanel = ControlPanel::createControlPanel(LanguageSets::get("myDeviceMyLanguages"));
    if (!myDeviceRootContainerControlPanel)
        return ER_FAIL;
    CHECK(myDeviceUnit->addControlPanel(myDeviceRootContainerControlPanel));

    myDeviceRootContainer = new Container("rootContainer", NULL);
    CHECK(myDeviceRootContainerControlPanel->setRootWidget(myDeviceRootContainer));

    myDeviceRootContainer->setEnabled(true);
    myDeviceRootContainer->setIsSecured(false);
    myDeviceRootContainer->setBgColor(0x200);

    std::vector<qcc::String> myDeviceRootContainerlabelVec;
    myDeviceRootContainerlabelVec.push_back("Sample Control Panel");
    myDeviceRootContainer->setLabels(myDeviceRootContainerlabelVec);

    std::vector<uint16_t> myDeviceRootContainerHintsVec;
    myDeviceRootContainerHintsVec.push_back(VERTICAL_LINEAR);
    myDeviceRootContainer->setHints(myDeviceRootContainerHintsVec);

    myDeviceMessageContainer = new Container("messageContainer", myDeviceRootContainer);
    CHECK(myDeviceRootContainer->addChildWidget(myDeviceMessageContainer));

    myDeviceMessageContainer->setEnabled(true);
    myDeviceMessageContainer->setIsSecured(false);
    myDeviceMessageContainer->setBgColor(0x200);

    std::vector<qcc::String> myDeviceMessageContainerlabelVec;
    myDeviceMessageContainerlabelVec.push_back("Message Printing Container");
    myDeviceMessageContainer->setLabels(myDeviceMessageContainerlabelVec);

    std::vector<uint16_t> myDeviceMessageContainerHintsVec;
    myDeviceMessageContainerHintsVec.push_back(VERTICAL_LINEAR);
    myDeviceMessageContainer->setHints(myDeviceMessageContainerHintsVec);

    myDeviceSelected_message = new MyDeviceSelected_message("selected_message", myDeviceMessageContainer, UINT16_PROPERTY);
    CHECK(myDeviceMessageContainer->addChildWidget(myDeviceSelected_message));

    myDeviceSelected_message->setEnabled(true);
    myDeviceSelected_message->setIsSecured(false);
    myDeviceSelected_message->setWritable(true);
    CHECK(myDeviceSelected_message->setGetValue(getCurrentMessage));
    myDeviceSelected_message->setBgColor(0x500);

    std::vector<qcc::String> myDeviceSelected_messagelabelVec;
    myDeviceSelected_messagelabelVec.push_back("Message To Print");
    myDeviceSelected_message->setLabels(myDeviceSelected_messagelabelVec);

    std::vector<uint16_t> myDeviceSelected_messageHintsVec;
    myDeviceSelected_messageHintsVec.push_back(SPINNER);
    myDeviceSelected_message->setHints(myDeviceSelected_messageHintsVec);

    std::vector<ConstraintList> myDeviceSelected_messageConstraintListVec(3);

    std::vector<qcc::String> myDeviceSelected_messageDisplay1Vec;
    myDeviceSelected_messageDisplay1Vec.push_back("Message One");
    myDeviceSelected_messageConstraintListVec[0].setDisplays(myDeviceSelected_messageDisplay1Vec);
    myDeviceSelected_messageConstraintListVec[0].setConstraintValue((uint16_t)1);

    std::vector<qcc::String> myDeviceSelected_messageDisplay2Vec;
    myDeviceSelected_messageDisplay2Vec.push_back("Message Two");
    myDeviceSelected_messageConstraintListVec[1].setDisplays(myDeviceSelected_messageDisplay2Vec);
    myDeviceSelected_messageConstraintListVec[1].setConstraintValue((uint16_t)2);

    std::vector<qcc::String> myDeviceSelected_messageDisplay3Vec;
    myDeviceSelected_messageDisplay3Vec.push_back("Message Three");
    myDeviceSelected_messageConstraintListVec[2].setDisplays(myDeviceSelected_messageDisplay3Vec);
    myDeviceSelected_messageConstraintListVec[2].setConstraintValue((uint16_t)3);

    myDeviceSelected_message->setConstraintList(myDeviceSelected_messageConstraintListVec);

    myDevicePrintMessage = new MyDevicePrintMessage("printMessage", myDeviceMessageContainer);
    CHECK(myDeviceMessageContainer->addChildWidget(myDevicePrintMessage));

    myDevicePrintMessage->setEnabled(true);
    myDevicePrintMessage->setIsSecured(false);
    myDevicePrintMessage->setBgColor(0x400);

    std::vector<qcc::String> myDevicePrintMessagelabelVec;
    myDevicePrintMessagelabelVec.push_back("Print Message");
    myDevicePrintMessage->setLabels(myDevicePrintMessagelabelVec);

    std::vector<uint16_t> myDevicePrintMessageHintsVec;
    myDevicePrintMessageHintsVec.push_back(ACTIONBUTTON);
    myDevicePrintMessage->setHints(myDevicePrintMessageHintsVec);

    myDeviceCounterContainer = new Container("counterContainer", myDeviceRootContainer);
    CHECK(myDeviceRootContainer->addChildWidget(myDeviceCounterContainer));

    myDeviceCounterContainer->setEnabled(true);
    myDeviceCounterContainer->setIsSecured(false);
    myDeviceCounterContainer->setBgColor(0x200);

    std::vector<qcc::String> myDeviceCounterContainerlabelVec;
    myDeviceCounterContainerlabelVec.push_back("Counter Container");
    myDeviceCounterContainer->setLabels(myDeviceCounterContainerlabelVec);

    std::vector<uint16_t> myDeviceCounterContainerHintsVec;
    myDeviceCounterContainerHintsVec.push_back(HORIZONTAL_LINEAR);
    myDeviceCounterContainer->setHints(myDeviceCounterContainerHintsVec);

    myDeviceCounterValueStringProperty = new MyDeviceCounterValueStringProperty("counterValueStringProperty", myDeviceCounterContainer, STRING_PROPERTY);
    CHECK(myDeviceCounterContainer->addChildWidget(myDeviceCounterValueStringProperty));

    myDeviceCounterValueStringProperty->setEnabled(true);
    myDeviceCounterValueStringProperty->setIsSecured(false);
    myDeviceCounterValueStringProperty->setWritable(false);
    CHECK(myDeviceCounterValueStringProperty->setGetValue(getCounterValueString));
    myDeviceCounterValueStringProperty->setBgColor(0x500);

    std::vector<qcc::String> myDeviceCounterValueStringPropertylabelVec;
    myDeviceCounterValueStringPropertylabelVec.push_back("Counter Value:");
    myDeviceCounterValueStringProperty->setLabels(myDeviceCounterValueStringPropertylabelVec);

    std::vector<uint16_t> myDeviceCounterValueStringPropertyHintsVec;
    myDeviceCounterValueStringPropertyHintsVec.push_back(TEXTVIEW);
    myDeviceCounterValueStringProperty->setHints(myDeviceCounterValueStringPropertyHintsVec);

    return status;
}

void ControlPanelGenerated::Shutdown()
{
    if (myDeviceUnit) {
        delete (myDeviceUnit);
        myDeviceUnit = 0;
    }
    if (myDeviceRootContainerControlPanel) {
        delete (myDeviceRootContainerControlPanel);
        myDeviceRootContainerControlPanel = 0;
    }
    if (myDeviceRootContainer) {
        delete (myDeviceRootContainer);
        myDeviceRootContainer = 0;
    }
    if (myDeviceMessageContainer) {
        delete (myDeviceMessageContainer);
        myDeviceMessageContainer = 0;
    }
    if (myDeviceSelected_message) {
        delete (myDeviceSelected_message);
        myDeviceSelected_message = 0;
    }
    if (myDevicePrintMessage) {
        delete (myDevicePrintMessage);
        myDevicePrintMessage = 0;
    }
    if (myDeviceCounterContainer) {
        delete (myDeviceCounterContainer);
        myDeviceCounterContainer = 0;
    }
    if (myDeviceCounterValueStringProperty) {
        delete (myDeviceCounterValueStringProperty);
        myDeviceCounterValueStringProperty = 0;
    }

}
