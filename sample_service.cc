/**
 * @file
 * @brief Sample implementation of Tellient analytics service
 */

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
#include <qcc/platform.h>

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <vector>

#include "TellientAnalytics.h"

using namespace std;
using namespace qcc;
using namespace ajn;

TellientDevFactory devFactory ;

/*constants*/
static const char* INTERFACE_NAME = "org.alljoyn.Analytics.tellient" ;
static const char* SERVICE_NAME = "org.alljoyn.Analytics.tellient"; 
static const char* SERVICE_PATH = "/analytics";
static const SessionPort SERVICE_PORT = 25;

static volatile sig_atomic_t s_interrupt = false;

static void SigIntHandler(int sig)
{
    s_interrupt = true;
}

class MyBusListener : public BusListener, public SessionPortListener {
    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
    {
        if (newOwner && (0 == strcmp(busName, SERVICE_NAME))) {
            printf("NameOwnerChanged: name=%s, oldOwner=%s, newOwner=%s.\n",
                   busName,
                   previousOwner ? previousOwner : "<none>",
                   newOwner ? newOwner : "<none>");
        }
    }
    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
    {
        if (sessionPort != SERVICE_PORT) {
            printf("Rejecting join attempt on unexpected session port %d.\n", sessionPort);
            return false;
        }
        printf("Accepting join session request from %s (opts.proximity=%x, opts.traffic=%x, opts.transports=%x).\n",
               joiner, opts.proximity, opts.traffic, opts.transports);
        return true;
    }
};

/** The bus listener object. */
static MyBusListener s_busListener;

/** Top level message bus object. */
static BusAttachment* s_msgBus = NULL;


/** Register the bus object and connect, report the result to stdout, and return the status code. */
QStatus RegisterBusObject(BusObject* obj)
{
    QStatus status = s_msgBus->RegisterBusObject(*obj);

    if (ER_OK == status) {
        printf("RegisterBusObject succeeded.\n");
    } else {
        printf("RegisterBusObject failed (%s).\n", QCC_StatusText(status));
    }

    return status;
}

/** Connect, report the result to stdout, and return the status code. */
QStatus ConnectBusAttachment(void)
{
    QStatus status = s_msgBus->Connect();

    if (ER_OK == status) {
        printf("Connect to '%s' succeeded.\n", s_msgBus->GetConnectSpec().c_str());
    } else {
        printf("Failed to connect to '%s' (%s).\n", s_msgBus->GetConnectSpec().c_str(), QCC_StatusText(status));
    }

    return status;
}

/** Start the message bus, report the result to stdout, and return the status code. */
QStatus StartMessageBus(void)
{
    QStatus status = s_msgBus->Start();

    if (ER_OK == status) {
        printf("BusAttachment started.\n");
    } else {
        printf("Start of BusAttachment failed (%s).\n", QCC_StatusText(status));
    }

    return status;
}

/** Create the session, report the result to stdout, and return the status code. */
QStatus CreateSession(TransportMask mask)
{
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, mask);
    SessionPort sp = SERVICE_PORT;
    QStatus status = s_msgBus->BindSessionPort(sp, opts, s_busListener);

    if (ER_OK == status) {
        printf("BindSessionPort succeeded.\n");
    } else {
        printf("BindSessionPort failed (%s).\n", QCC_StatusText(status));
    }

    return status;
}

/** Advertise the service name, report the result to stdout, and return the status code. */
QStatus AdvertiseName(TransportMask mask)
{
    QStatus status = s_msgBus->AdvertiseName(SERVICE_NAME, mask);

    if (ER_OK == status) {
        printf("Advertisement of the service name '%s' succeeded.\n", SERVICE_NAME);
    } else {
        printf("Failed to advertise name '%s' (%s).\n", SERVICE_NAME, QCC_StatusText(status));
    }

    return status;
}

/** Request the service name, report the result to stdout, and return the status code. */
QStatus RequestName(void)
{
    const uint32_t flags = DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE;
    QStatus status = s_msgBus->RequestName(SERVICE_NAME, flags);

    if (ER_OK == status) {
        printf("RequestName('%s') succeeded.\n", SERVICE_NAME);
    } else {
        printf("RequestName('%s') failed (status=%s).\n", SERVICE_NAME, QCC_StatusText(status));
    }

    return status;
}

/** Wait for SIGINT before continuing. */
void WaitForSigInt(void)
{
    while (s_interrupt == false) {
#ifdef _WIN32
        Sleep(100);
#else
        usleep(100 * 1000);
#endif
    }
}

/** Main entry point */
int main(int argc, char** argv, char** envArg)
{
    printf("AllJoyn Library version: %s.\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s.\n", ajn::GetBuildInfo());

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    QStatus status = ER_OK;

    /* Create message bus */
    s_msgBus = new BusAttachment("myApp", true);

    if (!s_msgBus) {
        status = ER_OUT_OF_MEMORY;
    }

    if (ER_OK == status) {
        status = AnalyticsBusObject::CreateInterface(*s_msgBus, INTERFACE_NAME);
    }

    if (ER_OK == status) {
        s_msgBus->RegisterBusListener(s_busListener);
    }

    if (ER_OK == status) {
        status = StartMessageBus();
    }

    AnalyticsBusObject testObj(*s_msgBus, &devFactory, SERVICE_PATH, INTERFACE_NAME);

    if (ER_OK == status) {
        status = RegisterBusObject(&testObj);
    }

    if (ER_OK == status) {
        status = ConnectBusAttachment();
    }

    /*
     * Advertise this service on the bus.
     * There are three steps to advertising this service on the bus.
     * 1) Request a well-known name that will be used by the client to discover
     *    this service.
     * 2) Create a session.
     * 3) Advertise the well-known name.
     */
    if (ER_OK == status) {
        status = RequestName();
    }

    const TransportMask SERVICE_TRANSPORT_TYPE = TRANSPORT_ANY;

    if (ER_OK == status) {
        status = CreateSession(SERVICE_TRANSPORT_TYPE);
    }

    if (ER_OK == status) {
        status = AdvertiseName(SERVICE_TRANSPORT_TYPE);
    }

    /* Perform the service asynchronously until the user signals for an exit. */
    if (ER_OK == status) {
        WaitForSigInt();
    }

    /* Clean up msg bus */
    delete s_msgBus;
    s_msgBus = NULL;

    printf("Basic service exiting with status 0x%04x (%s).\n", status, QCC_StatusText(status));

    return (int) status;
}
