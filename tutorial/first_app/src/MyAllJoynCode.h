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

#include <alljoyn/Status.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/SessionPortListener.h>
#include <alljoyn/SessionListener.h>
#include <qcc/String.h>
#include <alljoyn/about/AboutClient.h>
#include <alljoyn/about/AnnouncementRegistrar.h>
#include <alljoyn/about/AboutServiceApi.h>
#include <alljoyn/about/AboutPropertyStoreImpl.h>

#include <stdio.h>

#include "MyFirstBusObject.h"

#ifndef _MY_ALLJOYN_CODE_
#define _MY_ALLJOYN_CODE_

class MyAllJoynCode :
    public ajn::services::AnnounceHandler,
    public ajn::SessionPortListener,
    public ajn::SessionListener {
        public:
            /**
             * Construct a MyAllJoynCode object
             *
             */
            MyAllJoynCode();

            /**
             * Destructor
             */
            ~MyAllJoynCode();

            /**
             * Setup AllJoyn, creating the objects needed and registering listeners.
             *
             * @param appName  This value is provided to the BusAttachment constructor to name the application
             *
             */
            void initialize(const char* appName);

            /**
             * Join an AllJoyn session.
             *
             * @param uniqueName   The busName/uniqueName name to join
             * @param port         The port value that the remote side has bound a session
             *
             */
            ajn::SessionId joinSession(qcc::String uniqueName, short port);

            /**
             * Leave an AllJoyn session.
             *
             * @param sessionId   The ID of the session to leave
             *
             */
            void leaveSession(ajn::SessionId sessionId);

            /**
             * Get the list of friendly names to unique name map.
             * This is filled in by the applications found via the About feature service discovery.
             * @return the list of friendly names to unique names
             */
            const std::map<qcc::String, qcc::String>& listNearby() { return mBusFriendlyMap; };

            /**
             * Get the list of the unique names in the hosted session
             * @return the list of the unique names in the hosted session
             */
            const std::vector<qcc::String>& listHostedSessionMembers() { return mHostedSessionMembers; };

            /**
             * Get the list of unique names in the remote session
             * @return the list of unique names in the remote session
             */
            const std::vector<qcc::String>& listRemoteSessionMembers() { return mRemoteSessionMembers; };
            
            /**
             * Get the friendly name for a uniqueName which was collected from the About handler
             * @param uniqueName   name that identifies the remote application 
             * @return friendly name that maps to uniqueName
             */
            qcc::String getFriendlyName(qcc::String uniqueName) const;

            /**
             * Get the session port needed to join a unique name
             * @param uniqueName   name that identifies the remote application 
             * @return the session port that was found via About
             */
            short getSessionPort(qcc::String uniqueName) const;

            /**
             * Get the hosted sessionId
             * @return the sessionId that was set when someone joined for the first time
             */
            ajn::SessionId getHostSessionId() const;

            /**
             * Free up and release the objects used
             */
            void shutdown();

            /* Methods for using the BusObject to interact off application with other applications */
            QStatus doTell(qcc::String uniqueName, qcc::String thought, int sessionId, qcc::String& ret);

            QStatus doShare(qcc::String thought, int sessionId);

            QStatus doBroadcast(qcc::String thought);

        private: 
            /* From About */
            void Announce(unsigned short version, unsigned short port, const char* busName,
                    const ajn::services::AboutClient::ObjectDescriptions& objectDescs,
                    const ajn::services::AboutClient::AboutData& aboutData);

            /* From SessionPortListener */

            virtual bool AcceptSessionJoiner(ajn::SessionPort sessionPort,
                    const char* joiner, const ajn::SessionOpts& opts);

            virtual void SessionJoined(ajn::SessionPort sessionPort, ajn::SessionId id, const char* joiner);

            /* From SessionListener */
            virtual void SessionLost(ajn::SessionId sessionId);

            virtual void SessionMemberAdded(ajn::SessionId sessionId, const char*uniqueName);

            virtual void SessionMemberRemoved(ajn::SessionId sessionId, const char*uniqueName);

            const char* getDeviceId();
            const char* getAppId();
            std::vector<qcc::String> createAndRegisterBusObject();

        private:
            ajn::BusAttachment* mBusAttachment;

            /** Map to keep track of the name to display and the name that AllJoyn uses */
            std::map<qcc::String, qcc::String> mBusFriendlyMap;
            std::map<qcc::String, short> mBusPortMap;

            ajn::SessionId mHostedSessionId;
            std::vector<qcc::String> mHostedSessionMembers;
            std::vector<qcc::String> mRemoteSessionMembers;


            MyFirstBusObject* mMyFirstBusObject;
            ajn::services::AboutPropertyStoreImpl* mAboutData;
    };

#endif //_MY_ALLJOYN_CODE_

