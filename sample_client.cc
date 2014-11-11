/**
 * @file
 * @brief  Sample implementation of an analytics client
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2009-2014, AllSeen Alliance. All rights reserved.
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

#include <qcc/String.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/version.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/Status.h>
#include "Analytics.h"

using namespace std;
using namespace qcc;
using namespace ajn;

/** Static top level message bus object */
static BusAttachment* g_msgBus = NULL;

/*constants*/
static const char* INTERFACE_NAME = "org.alljoyn.Analytics.tellient" ;
static const char* SERVICE_NAME = "org.alljoyn.Analytics.tellient" ;
static const char* SERVICE_PATH = "/analytics";
static const SessionPort SERVICE_PORT = 25;

static bool s_joinComplete = false;
static SessionId s_sessionId = 0;

static volatile sig_atomic_t s_interrupt = false;

static void SigIntHandler(int sig)
{
    s_interrupt = true;
}

/** AllJoynListener receives discovery events from AllJoyn */
class MyBusListener : public BusListener, public SessionListener {
  public:
    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        if (0 == strcmp(name, SERVICE_NAME)) {
            printf("FoundAdvertisedName(name='%s', prefix='%s')\n", name, namePrefix);

            /* We found a remote bus that is advertising the analytics service's well-known name so connect to it. */
            /* Since we are in a callback we must enable concurrent callbacks before calling a synchronous method. */
            g_msgBus->EnableConcurrentCallbacks();
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
            QStatus status = g_msgBus->JoinSession(name, SERVICE_PORT, this, s_sessionId, opts);
            if (ER_OK == status) {
                printf("JoinSession SUCCESS (Session id=%d).\n", s_sessionId);
            } else {
                printf("JoinSession failed (status=%s).\n", QCC_StatusText(status));
            }
        }
        s_joinComplete = true;
    }

    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
    {
        if (newOwner && (0 == strcmp(busName, SERVICE_NAME))) {
            printf("NameOwnerChanged: name='%s', oldOwner='%s', newOwner='%s'.\n",
                   busName,
                   previousOwner ? previousOwner : "<none>",
                   newOwner ? newOwner : "<none>");
        }
    }
};


/** Start the message bus, report the result to stdout, and return the result status. */
QStatus StartMessageBus(void)
{
    QStatus status = g_msgBus->Start();

    if (ER_OK == status) {
        printf("BusAttachment started.\n");
    } else {
        printf("BusAttachment::Start failed.\n");
    }

    return status;
}

/** Handle the connection to the bus, report the result to stdout, and return the result status. */
QStatus ConnectToBus(void)
{
    QStatus status = g_msgBus->Connect();

    if (ER_OK == status) {
        printf("BusAttachment connected to '%s'.\n", g_msgBus->GetConnectSpec().c_str());
    } else {
        printf("BusAttachment::Connect('%s') failed.\n", g_msgBus->GetConnectSpec().c_str());
    }

    return status;
}

/** Register a bus listener in order to get discovery indications and report the event to stdout. */
void RegisterBusListener(void)
{
    /* Static bus listener */
    static MyBusListener s_busListener;

    g_msgBus->RegisterBusListener(s_busListener);
    printf("BusListener Registered.\n");
}

/** Begin discovery on the well-known name of the service to be called, report the result to
   stdout, and return the result status. */
QStatus FindAdvertisedName(void)
{
    /* Begin discovery on the well-known name of the service to be called */
    QStatus status = g_msgBus->FindAdvertisedName(SERVICE_NAME);

    if (status == ER_OK) {
        printf("org.alljoyn.Bus.FindAdvertisedName ('%s') succeeded.\n", SERVICE_NAME);
    } else {
        printf("org.alljoyn.Bus.FindAdvertisedName ('%s') failed (%s).\n", SERVICE_NAME, QCC_StatusText(status));
    }

    return status;
}

/** Wait for join session to complete, report the event to stdout, and return the result status. */
QStatus WaitForJoinSessionCompletion(void)
{
    unsigned int count = 0;

    while (!s_joinComplete && !s_interrupt) {
        if (0 == (count++ % 10)) {
            printf("Waited %u seconds for JoinSession completion.\n", count / 10);
        }

#ifdef _WIN32
        Sleep(100);
#else
        usleep(100 * 1000);
#endif
    }

    return s_joinComplete && !s_interrupt ? ER_OK : ER_ALLJOYN_JOINSESSION_REPLY_CONNECT_FAILED;
}

/** make method calls and report to stdout */
QStatus MakeMethodCalls(void)
{
    ProxyBusObject remoteObj(*g_msgBus, SERVICE_NAME, SERVICE_PATH, s_sessionId);
    const InterfaceDescription* alljoynTestIntf = g_msgBus->GetInterface(INTERFACE_NAME);

    assert(alljoynTestIntf);
    remoteObj.AddInterface(*alljoynTestIntf);

    Message reply(*g_msgBus);

    MsgArg args[5] ;
    MsgArg kv[10] ;
    MsgArg variant ;
    QStatus status ;

    // call SetVendorData method.  It takes one parameter, an array of
    // string-variant pairs.  For tellient, this list must include
    // manufacturer_id, post_url, and device model.
    variant.Set("i", 1337) ;
    kv[0].Set("{sv}", "manufacturer_id", &variant) ;
    kv[0].Stabilize() ;

    variant.Set("s", "http://localhost/teupdate") ;
    kv[1].Set("{sv}", "post_url", &variant) ;
    kv[1].Stabilize() ;

    variant.Set("s", "bass-o-matic") ;
    kv[2].Set("{sv}", "model", &variant) ;
    kv[2].Stabilize() ;

    args[0].Set("a{sv}", 3, kv) ;

    status = remoteObj.MethodCall(SERVICE_NAME, "SetVendorData", args, 1, reply, 5000);

    const char *err ;

    if (ER_OK == status) {
        printf("SetVendorData success\n") ;
    } else {
	err = reply->GetErrorDescription().c_str() ;
        printf("SetVendorData failed with %s.\n",err) ;
	return status ;
    }

    // call SetDeviceData method.  This defines device data that is the
    // same for all events.  Typically this will include model version
    // strings, etc.

    variant.Set("s", "102") ;
    kv[0].Set("{sv}", "modelVer", &variant) ;
    kv[0].Stabilize() ;

    variant.Set("s", "Jasper") ;
    kv[1].Set("{sv}", "dogname", &variant) ;
    kv[1].Stabilize() ;

    args[0].Set("a{sv}", 2, kv) ;

    status = remoteObj.MethodCall(SERVICE_NAME, "SetDeviceData", args, 1, reply, 5000);

    if (ER_OK == status) {
        printf("SetDeviceData success\n") ;
    } else {
	err = reply->GetErrorDescription().c_str() ;
        printf("SetDeviceData failed with %s.\n", err) ;
	return status ;
    }

    // send a few events.

    uint32_t sequence = 0 ;
    for (int i = 0 ; i < 3 ; i++) {
	args[0].Set("s", "fakeeventname") ;
	args[1].Set("t", 0LL) ;  // timestamp 
	args[2].Set("u", sequence++) ; 

	variant.Set("s", "shiny") ;
	kv[0].Set("{sv}", "description", &variant) ;
	kv[0].Stabilize() ;

	variant.Set("i", 98) ;
	kv[1].Set("{sv}", "temperature", &variant) ;
	kv[1].Stabilize() ;

	args[3].Set("a{sv}", 2, kv) ;

	status = remoteObj.MethodCall(SERVICE_NAME, "SubmitEvent", args, 4, reply, 5000);
	if (ER_OK == status) {
	    printf("%s success\n", "SubmitEvent") ;
	} else {
	    err = reply->GetErrorDescription().c_str() ;
	    printf("SetVendorData failed with %s.\n", err) ;
	    return status ;
	}
    }

    // request submission to service.

    status = remoteObj.MethodCall(SERVICE_NAME, "RequestDelivery", args, 0, reply, 5000);

    if (ER_OK == status) {
        printf("RequestDelivery success\n") ;
    } else {
	err = reply->GetErrorDescription().c_str() ;
        printf("RequestDelivery failed with %s.\n", err) ;
	return status ;
    }


    return ER_OK ;
}

/** Main entry point */
int main(int argc, char** argv, char** envArg)
{
    printf("AllJoyn Library version: %s.\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s.\n", ajn::GetBuildInfo());

    /* Install SIGINT handler. */
    signal(SIGINT, SigIntHandler);

    QStatus status = ER_OK;

    /* Create message bus. */
    g_msgBus = new BusAttachment("myApp", true);

    /* This test for NULL is only required if new() behavior is to return NULL
     * instead of throwing an exception upon an out of memory failure.
     */
    if (!g_msgBus) {
        status = ER_OUT_OF_MEMORY;
    }

    if (ER_OK == status) {
        status = AnalyticsBusObject::CreateInterface(*g_msgBus, INTERFACE_NAME);
    }

    if (ER_OK == status) {
        status = StartMessageBus();
    }

    if (ER_OK == status) {
        status = ConnectToBus();
    }

    if (ER_OK == status) {
        RegisterBusListener();
        status = FindAdvertisedName();
    }

    if (ER_OK == status) {
        status = WaitForJoinSessionCompletion();
    }

    if (ER_OK == status) {
        status = MakeMethodCalls();
    }

    /* Deallocate bus */
    delete g_msgBus;
    g_msgBus = NULL;

    printf("client exiting with status 0x%04x (%s).\n", status, QCC_StatusText(status));

    return (int) status;
}
