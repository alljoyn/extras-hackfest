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

#include "MyAllJoynCode.h"

using namespace ajn;
using namespace qcc;
using namespace services;

MyAllJoynCode::MyAllJoynCode()
: mBusAttachment(NULL), AnnounceHandler(), mMyFirstBusObject(NULL), mAboutData(NULL)
{ };

MyAllJoynCode::~MyAllJoynCode() {
    shutdown();
};

void MyAllJoynCode::initialize(const char* appName) {
    QStatus status = ER_OK;

    /* Initialize AllJoyn only once */
    if (!mBusAttachment) {
        /*
         * All communication through AllJoyn begins with a BusAttachment.
         *
         * A BusAttachment needs a name. The actual name is unimportant except for internal
         * security. As a default we use the class name as the name.
         *
         * By default AllJoyn does not allow communication between devices (i.e. bus to bus
         * communication).  The second argument must be set to Receive to allow
         * communication between devices.
         */
        mBusAttachment = new BusAttachment("MyFirstApplication", true);
        /* Start the msg bus */
        if (ER_OK == status) {
            status = mBusAttachment->Start();
        } else {
            printf("BusAttachment::Start failed\n");
        }
        /* Connect to the daemon */
        if (ER_OK == status) {
            status = mBusAttachment->Connect();
            if (ER_OK != status) {
                printf("BusAttachment Connect failed.\n");
            }
        }
        printf("Created BusAttachment and connected.\n");

        /* Bind a session port so that we can accept incomming join requests */
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, true, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        SessionPort sp = SESSION_PORT_ANY; 
        status = mBusAttachment->BindSessionPort(sp, opts, *this);
        if (ER_OK != status) {
            printf("Failed to bind session port\n");
        }

        /* Create About data so that we can advertise */
        mAboutData = new AboutPropertyStoreImpl();
        // a platform-specific unique device id - ex. could be the Mac address
        // use a random value for this application, this should persist in a comercial application
        mAboutData->setDeviceId(getDeviceId());
        mAboutData->setDeviceName("MyDeviceName");
        // the globally unique identifier for the application - recommend to use an online GUID generator to create
        // use a random value for this application, this should persist in a comercial application
        mAboutData->setAppId(getAppId());
        std::vector<qcc::String> languages(1);
        languages[0] = "en";
        mAboutData->setSupportedLangs(languages);
        mAboutData->setDefaultLang("en");
        mAboutData->setAppName(appName);
        mAboutData->setModelNumber("Tutorial5000");
        mAboutData->setDateOfManufacture("8/15/2014");
        mAboutData->setSoftwareVersion("1.0 build 1");
        mAboutData->setAjSoftwareVersion(ajn::GetVersion());
        mAboutData->setHardwareVersion("N/A");
        mAboutData->setDescription("This is the my first AllJoyn Application!", "en");
        mAboutData->setManufacturer("Company", "Me");
        mAboutData->setSupportUrl("http://www.allseenalliance.org");

        /* Initialize the About feature Service side */
        AboutServiceApi::Init(*mBusAttachment, *mAboutData);

        /* Register the port with About feature that was set when BindSession called */
        status = AboutServiceApi::getInstance()->Register(sp);
        /* Register the About feature with AllJoyn */
        status = mBusAttachment->RegisterBusObject(*AboutServiceApi::getInstance());

        /* Create and Register the developers BusObjects */
        std::vector<qcc::String> interfacesUsed = createAndRegisterBusObject();

        /* Now create a list of the interfaces used so that we can find just applications that use these interfaces */
        printf("Here going to create interface list\n");
        int len = interfacesUsed.size();
        const char* interfaces[len];
        for( int i = 0; i < len; i++) {
           interfaces[i] = interfacesUsed[i].c_str();
        }

        /*
         * Register this class to receive AllJoyn About feature annoucements from the services we care about.
         * This performs service level discovery
         */
        AnnouncementRegistrar::RegisterAnnounceHandler(*mBusAttachment, *this, interfaces, len);

        /* With the objects registered and everything setup we Annouce to tell the world that we exist  */
        status = AboutServiceApi::getInstance()->Announce();
        if (ER_OK != status) {
            printf("Failed to addMatch for sessionless signals: %s\n", QCC_StatusText(status));
        }
    }
}

const char* MyAllJoynCode::getDeviceId() {
    srand (time(NULL)); 
    qcc::String id = "";
    for(int i = 0; i < 22; i++) {
        /* Rand val from 0-9 */
        id += '0' + rand() % 10;
    }
    return id.c_str();
}

const char* MyAllJoynCode::getAppId() {
srand (time(NULL)); 
    qcc::String id = "";
    for(int i = 0; i < 30; i++) {
        /* Rand val from 0-9 */
        id += '0' + rand() % 10;
    }
    return id.c_str();
}

std::vector<qcc::String> MyAllJoynCode::createAndRegisterBusObject()
{
    /**
     * Here is where we add the objects we wish to expose.
     * A developer would modify this section to add different BusObjects.
     */
    mMyFirstBusObject = new MyFirstBusObject(*mBusAttachment);

    /* Now register the object with AllJoyn and About */
    QStatus status;
    status = mBusAttachment->RegisterBusObject(*mMyFirstBusObject);
    if (ER_OK != status) {
        printf("Could not register the BusObject with the BusAttachment\n");
    }

    std::vector<qcc::String> interfaces;
    for( int i = 0; i < mMyFirstBusObject->getNumberOfInterfaces(); i++) {
        interfaces.push_back(mMyFirstBusObject->getInterfaceName(i));
    }
    status = AboutServiceApi::getInstance()->AddObjectDescription(mMyFirstBusObject->GetPath(), interfaces);
    if (ER_OK != status) {
        printf("Error returned by AddObjectDescription (%s).\n", QCC_StatusText(status));
    }

    return interfaces;
}

SessionId MyAllJoynCode::joinSession(qcc::String uniqueName, short port)
{
    //Clear the previous list of members
    mRemoteSessionMembers.clear();
    //Create opts and make the join session AllJoyn API call
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, true, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    SessionId joinedSessionId;
    QStatus status = mBusAttachment->JoinSession(uniqueName.c_str(), port, this, joinedSessionId, opts);
    if( ER_OK == status) {
        printf("Connected to %s on port %d with sessionId %d\n", uniqueName.c_str(), port, joinedSessionId);
    } else {
        printf("Failed to connect to %s on port %d (%x)\n", uniqueName.c_str(), port, status);
    }
    return joinedSessionId;
}

void MyAllJoynCode::leaveSession(SessionId sessionId)
{
    QStatus status = mBusAttachment->LeaveSession(sessionId); 
    if( ER_OK == status) {
        printf("Left session %d\n", sessionId);
    } else {
        printf("Failed to leave session %d (%x)\n", sessionId, status);
    }
    mRemoteSessionMembers.clear();
}

qcc::String MyAllJoynCode::getFriendlyName(qcc::String uniqueName) const
{
    std::map<String,String>::const_iterator it;
    for (it = mBusFriendlyMap.begin(); it != mBusFriendlyMap.end(); ++it) {
        if(uniqueName.compare(it->first) == 0) {
            return it->second; 
        }
    }
    return "ERROR";
}

short MyAllJoynCode::getSessionPort(qcc::String uniqueName) const
{
    std::map<String,short>::const_iterator it;
    for (it = mBusPortMap.begin(); it != mBusPortMap.end(); ++it) {
        if(uniqueName.compare(it->first) == 0) {
            return it->second; 
        }
    }
    return 0;
}

ajn::SessionId MyAllJoynCode::getHostSessionId() const
{
    return mHostedSessionId;
}

void MyAllJoynCode::shutdown()
{

}

QStatus MyAllJoynCode::doTell(qcc::String uniqueName, qcc::String thought, int sessionId, qcc::String& ret)
{
    return mMyFirstBusObject->doTell(uniqueName, thought, sessionId, ret);
}

QStatus MyAllJoynCode::doShare(qcc::String thought, int sessionId)
{
    mMyFirstBusObject->doShare(thought, sessionId);
}

QStatus MyAllJoynCode::doBroadcast(qcc::String thought)
{
    mMyFirstBusObject->doBroadcast(thought);
}

void MyAllJoynCode::Announce(unsigned short version, unsigned short port, const char* busName,
                             const ObjectDescriptions& objectDescs,
                                                          const AboutData& aboutData)
{
    if(mBusAttachment->GetUniqueName().compare(busName) == 0) {
        printf("Found myself(%s)\n", busName);
        return;
    }
    printf("Found about application with busName, port %s, %d\n", busName, port);
    /* For now lets just assume everything has events and actions and join */
    char* friendlyName = NULL;
    for (AboutClient::AboutData::const_iterator it = aboutData.begin(); it != aboutData.end(); ++it) {
        qcc::String key = it->first;
        ajn::MsgArg value = it->second;
        if (value.typeId == ALLJOYN_STRING) {
            if (key.compare("DeviceName") == 0) {
                friendlyName = ::strdup(value.v_string.str);
                mBusFriendlyMap.insert(std::pair<qcc::String, qcc::String>(busName, friendlyName));
            }
            printf("(Announce handler) aboutData (key, val) (%s, %s)\n", key.c_str(), value.v_string.str);
        }
    }
    mBusPortMap.insert(std::pair<qcc::String, short>(busName, port)); 
    printf("Friendly Name: %s\n", friendlyName);
      
}

/**
 * AcceptSessionJoiner - Receive request to join session and decide whether to accept it or not
 * @param sessionPort - the port of the request
 * @param joiner - the name of the joiner
 * @param opts - the session options
 * @return true/false
 */
bool MyAllJoynCode::AcceptSessionJoiner(ajn::SessionPort sessionPort, const char* joiner, const ajn::SessionOpts& opts)
{
    printf("Accepting JoinSessionRequest from %s (opts.proximity= %d, opts.traffic= %d, opts.transports=%d).\n", joiner, opts.proximity, opts.traffic, opts.transports);
    return true;
}

void MyAllJoynCode::SessionJoined(ajn::SessionPort sessionPort, ajn::SessionId id, const char* joiner)
{
    printf("%s joined with sessionId %d\n", joiner, id);
    mHostedSessionId = id;
    mBusAttachment->SetSessionListener(id,this); 
    mHostedSessionMembers.push_back(joiner);
}

/* From SessionListener */
void MyAllJoynCode::SessionLost(ajn::SessionId sessionId)
{
    printf("SessionLost %d\n",sessionId);
}

void MyAllJoynCode::SessionMemberAdded(ajn::SessionId sessionId, const char*uniqueName)
{
    if (sessionId == mHostedSessionId)
    {
        mHostedSessionMembers.push_back(uniqueName);
        printf("%s joined your session\n", uniqueName);
    } else {
        mRemoteSessionMembers.push_back(uniqueName);
        printf("%s joined the remote session you are connected to\n", uniqueName);
    }
}

void MyAllJoynCode::SessionMemberRemoved(ajn::SessionId sessionId, const char*uniqueName)
{
    if (sessionId == mHostedSessionId)
    {
        for (int i=0; i<mHostedSessionMembers.size(); i++) {
            if(mHostedSessionMembers[i].compare(uniqueName) == 0) {
                mHostedSessionMembers.erase(mHostedSessionMembers.begin()+i);
                return;
            }
        }
        printf("%s left your session\n", uniqueName);
    } else {
        for (int i=0; i<mRemoteSessionMembers.size(); i++) {
            if(mRemoteSessionMembers[i].compare(uniqueName) == 0) {
                mRemoteSessionMembers.erase(mRemoteSessionMembers.begin()+i);
                return;
            }
        }
        printf("%s left the remote session you are connected to\n", uniqueName);
    }

}
