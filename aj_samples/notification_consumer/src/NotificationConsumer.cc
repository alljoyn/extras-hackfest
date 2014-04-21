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

#include <algorithm>
#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <sstream>
#include <vector>
#include <alljoyn/about/AnnouncementRegistrar.h>
#include <alljoyn/notification/Notification.h>
#include <alljoyn/notification/NotificationReceiver.h>
#include <alljoyn/notification/NotificationService.h>
#include <alljoyn/notification/NotificationText.h>
#include <alljoyn/PasswordManager.h>
#include <alljoyn/services_common/LogModulesNames.h>
#include <alljoyn/Status.h>


using namespace ajn;
using namespace services;
using namespace qcc;

class NotificationReceiverImpl;

NotificationService* notificationService = 0;
NotificationReceiverImpl* notificationReceiver = 0;
ajn::BusAttachment* busAttachment = 0;
static volatile sig_atomic_t s_interrupt = false;

void cleanup()
{
    std::cout << "cleanup() - start" << std::endl;
    if (notificationService) {
        notificationService->shutdown();
    }
    if (notificationReceiver) {
        delete notificationReceiver;
    }
    if (busAttachment) {
        delete busAttachment;
    }
    std::cout << "cleanup() - end" << std::endl;
}

void signal_callback_handler(int32_t signum)
{
    std::cout << "got signal_callback_handler" << std::endl;
    cleanup();
    s_interrupt = true;
    std::cout << "Goodbye!" << std::endl;
    exit(signum);
}

bool WaitForSigInt(int32_t sleepTime) {
    if (s_interrupt == false) {
#ifdef _WIN32
        Sleep(100);
#else
        sleep(sleepTime);
#endif
        return false;
    }
    return true;
}

/**
 * Class that will receive Notifications. Implements NotificationReceiver
 * Receives list of applications to filter by and will only display notifications
 * from those applications
 */
class NotificationReceiverImpl : public ajn::services::NotificationReceiver {
  public:
    enum NotificationAction {
        ACTION_NOTHING,
        ACTION_DISMISS
    };
    /**
     * Constructor
     * @param wait to external notification action
     */
    NotificationReceiverImpl(bool waitForExternalNotificationAction = true);

    /**
     * Destructor
     */
    ~NotificationReceiverImpl();

    /**
     * Receive - function that receives a notification
     * @param notification
     */
    void Receive(ajn::services::Notification const& notification);

    /**
     * receive a list of applications to filter by and set the filter
     * @param listOfApps
     */
    void setApplications(qcc::String const& listOfApps);

    /**
     * receive Dismiss signal
     * @param message id
     * @param application id
     */
    void Dismiss(const int32_t msgId, const qcc::String appId);

    /**
     * Get notification action
     * @return NotificationAction
     */
    NotificationAction GetNotificationAction();

    /**
     * Set notification action
     * This method is called from a free thread to set an action and to release the blocked thread (At NotificationReceiverImpl::Receive(...)),
     * that received the notification and waiting to the action decision.
     * @param NotificationAction
     */
    void SetNotificationAction(NotificationAction notificationAction);

  private:
    /**
     * vector of applications to filter by
     */
    std::vector<qcc::String> m_Applications;
    /**
     * action to do after getting notification
     */
    NotificationAction m_NotificationAction;
    /**
     * locks for the condition according to 'pthread_cond_t' declaration.
     */
    pthread_mutex_t m_Lock;
    /**
     * thread condition
     * Blocking the notification receiving thread in case m_WaitForExternalNotificationAction is true, until SetNotificationAction() will be called.
     */
    pthread_cond_t m_Condition;
    /**
     * Wait to external notification action
     * If true - external thread will need to call to SetNotificationAction() to unblock the thread that received the notification.
     * If false - a normal standard input will block the thread that received the notification until the user will decide what to do with the notification.
     */
    bool m_WaitForExternalNotificationAction;
};

NotificationReceiverImpl::NotificationReceiverImpl(bool waitForExternalNotificationAction) :
    m_NotificationAction(ACTION_NOTHING), m_WaitForExternalNotificationAction(waitForExternalNotificationAction) {

    if (m_WaitForExternalNotificationAction) {
        pthread_mutex_init(&m_Lock, NULL);
        pthread_cond_init(&m_Condition, NULL);
    }
}

NotificationReceiverImpl::~NotificationReceiverImpl() {

    if (m_WaitForExternalNotificationAction) {

        pthread_mutex_lock(&m_Lock);
        pthread_cond_signal(&m_Condition);
        pthread_mutex_unlock(&m_Lock);

        pthread_cond_destroy(&m_Condition);
        pthread_mutex_destroy(&m_Lock);
    }
}

void NotificationReceiverImpl::Receive(Notification const& notification) {

    qcc::String appName = notification.getAppName();
    // If applications list is empty or the name exists in the filter list then print the notification
    if ((m_Applications.size() == 0) || (find(m_Applications.begin(), m_Applications.end(), appName) !=  m_Applications.end())) {
        std::cout << "******************** Begin New Message Received ********************" << std::endl;
        std::cout << "Message Id: " << notification.getMessageId() << std::endl;
        std::cout << "Device Id: " << notification.getDeviceId() << std::endl;
        std::cout << "Device Name: " << notification.getDeviceName() << std::endl;
        std::cout << "App Id: " << notification.getAppId() << std::endl;
        std::cout << "App Name: " << notification.getAppName() << std::endl;
        std::cout << "Sender BusName: " << notification.getSenderBusName() << std::endl;
        std::cout << "Message Type " << notification.getMessageType() << " " << MessageTypeUtil::getMessageTypeString(notification.getMessageType()).c_str() << std::endl;
        std::cout << "Notification version: " << notification.getVersion() << std::endl;


        // get vector of text messages and iterate through it
        std::vector<NotificationText> vecMessages = notification.getText();

        for (std::vector<NotificationText>::const_iterator vecMessage_it = vecMessages.begin(); vecMessage_it != vecMessages.end(); ++vecMessage_it) {
            std::cout << "Language: " << vecMessage_it->getLanguage().c_str() << "  Message: " << vecMessage_it->getText().c_str() << std::endl;
        }

        // Print out any other parameters sent in
        std::cout << "Other parameters included:" << std::endl;
        std::map<qcc::String, qcc::String> customAttributes = notification.getCustomAttributes();

        for (std::map<qcc::String, qcc::String>::const_iterator customAttributes_it = customAttributes.begin(); customAttributes_it != customAttributes.end(); ++customAttributes_it) {
            std::cout << "Custom Attribute Key: " << customAttributes_it->first.c_str() << "  Custom Attribute Value: " << customAttributes_it->second.c_str() << std::endl;
        }

        if (notification.getRichIconUrl()) {
            std::cout << "Rich Content Icon Url: " << notification.getRichIconUrl() << std::endl;
        }

        // get vector of audio content and iterate through it
        std::vector<RichAudioUrl> richAudioUrl = notification.getRichAudioUrl();

        if (!richAudioUrl.empty()) {
            std::cout << "******************** Begin Rich Audio Content ********************" << std::endl;
            for (std::vector<RichAudioUrl>::const_iterator vecAudio_it = richAudioUrl.begin(); vecAudio_it != richAudioUrl.end(); ++vecAudio_it) {
                std::cout << "Language: " << vecAudio_it->getLanguage().c_str() << "  Audio Url: " << vecAudio_it->getUrl().c_str() << std::endl;
            }
            std::cout << "******************** End Rich Audio Content ********************" << std::endl;

        }

        if (notification.getRichIconObjectPath()) {
            std::cout << "Rich Content Icon Object Path: " << notification.getRichIconObjectPath() << std::endl;
        }

        if (notification.getRichAudioObjectPath()) {
            std::cout << "Rich Content Audio Object Path: " << notification.getRichAudioObjectPath() << std::endl;
        }

        if (notification.getControlPanelServiceObjectPath()) {
            std::cout << "ControlPanelService object path: " << notification.getControlPanelServiceObjectPath() << std::endl;
        }

        if (notification.getOriginalSender()) {
            std::cout << "OriginalSender: " << notification.getOriginalSender() << std::endl;
        }


        std::cout << "******************** End New Message Received ********************" << std::endl << std::endl;

        Notification nonConstNotification(notification);
        // Simply dismiss the notification after printing to console - do not require any user input
        nonConstNotification.dismiss();
    }
    std::cout << "End handling notification!!!" << std::endl;
}

void NotificationReceiverImpl::setApplications(qcc::String const& listOfApps) {
    std::istringstream iss(listOfApps.c_str());
    std::string singleAppName;
    while (std::getline(iss, singleAppName, ';')) {
        m_Applications.push_back(singleAppName.c_str());
    }
}

void NotificationReceiverImpl::Dismiss(const int32_t msgId, const qcc::String appId)
{
    std::cout << "Got NotificationReceiverImpl::Dismiss with msgId=" << msgId << " appId=" << appId.c_str() << std::endl;
}

NotificationReceiverImpl::NotificationAction NotificationReceiverImpl::GetNotificationAction()
{
    return m_NotificationAction;
}

void NotificationReceiverImpl::SetNotificationAction(NotificationReceiverImpl::NotificationAction notificationAction)
{
    if (m_WaitForExternalNotificationAction) {
        pthread_mutex_lock(&m_Lock);
        m_NotificationAction = notificationAction;
        pthread_cond_signal(&m_Condition);
        pthread_mutex_unlock(&m_Lock);
    }
}

int main()
{
    // Allow CTRL+C to end application
    signal(SIGINT, signal_callback_handler);

    // Initialize Service object
    notificationService = NotificationService::getInstance();

    // change loglevel to debug:
    QCC_SetDebugLevel(logModules::NOTIFICATION_MODULE_LOG_NAME, logModules::ALL_LOG_LEVELS);

    notificationReceiver = new NotificationReceiverImpl(false);
    busAttachment = new BusAttachment("NotificationConsumer", true);

    /* Start the BusAttachment */
    QStatus status = busAttachment->Start();
    if (status != ER_OK) {
        delete busAttachment;
        busAttachment = NULL;
    }

    status = busAttachment->Connect();
    if (status != ER_OK) {
        delete busAttachment;
        busAttachment = NULL;
    }
    if (busAttachment == NULL) {
        std::cout << "Could not initialize BusAttachment." << std::endl;
        cleanup();
        return EXIT_FAILURE;
    }

    status = notificationService->initReceive(busAttachment, notificationReceiver);
    if (status != ER_OK) {
        std::cout << "Could not initialize receiver." << std::endl;
        cleanup();
        return EXIT_FAILURE;
    }

    status = busAttachment->AddMatch("sessionless='t',type='error'");
    if (status != ER_OK) {
        std::cout << "Could not add sessionless match." << std::endl;
        cleanup();
        return EXIT_FAILURE;
    }

    std::cout << "\n### Waiting for notifications.\n" << std::endl;

    int32_t sleepTime = 5;
    while (!WaitForSigInt(sleepTime)) ;

    return 0;
}
