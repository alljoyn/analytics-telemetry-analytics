/**
 * @file
 * @brief Sample implementation of Tellient analytics service
 */

/******************************************************************************
 * Copyright (c) 2014-2015, AllSeen Alliance. All rights reserved.
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
#include <signal.h>
#include <stdio.h>
#include <vector>

#include <qcc/platform.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/AboutObj.h>
#include "TellientAnalytics.h"
#include "ECDHEKeyXListener.h"

using namespace std;
using namespace qcc;
using namespace ajn;

/*constants*/
static const char* INTERFACE_NAME = "org.alljoyn.analytics.AnalyticsEventAgent";

/*
 * The service path may be used to help distinguish between multiple
 * instances of services advertising the Analytics interface. A trusted
 * relationship between client and service can be created by adding a
 * AuthListener such as the example ECDHEKeyXListener implemented here.
 */
static const char* SERVICE_PATH = "/analytics/example";

/* The app id must is a UUID. Generate a new one for your own analytics service implementation. */
static const uint8_t MY_APP_ID[] = {
    0x01, 0x8e, 0xc2, 0x19,
    0x52, 0x9d, 0x49, 0x75,
    0xbd, 0x6b, 0xe7, 0x06,
    0x88, 0x5a, 0x4e, 0x80
};

/* Likewise, this should be a unique identifier for the device running the service*/
static const qcc::String MY_DEVICE_ID = "this device id";

/* About assigned service port for About service */
static const ajn::SessionPort ASSIGNED_SERVICE_PORT = 900;

static const qcc::String DEFAULT_LANGUAGE = "en";

static volatile sig_atomic_t s_interrupt = false;

static void SigIntHandler(int sig)
{
    s_interrupt = true;
}

class MySessionPortListener : public SessionPortListener {
    public:
        bool AcceptSessionJoiner(ajn::SessionPort sessionPort, const char* joiner, const ajn::SessionOpts& opts)
        {
            if (sessionPort != ASSIGNED_SERVICE_PORT) {
                printf("Rejecting join attempt on unexpected session port %d.\n", sessionPort);
                return false;
            }
            printf("Accepting join session request from %s (opts.proximity=%x, opts.traffic=%x, opts.transports=%x).\n",
                    joiner, opts.proximity, opts.traffic, opts.transports);
            return true;
        }
        void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner)
        {
            printf("Session Joined SessionId = %u\n", id);
        }
};

static QStatus FillAboutPropertyStoreImplData(AboutData& aboutData)
{
    QStatus status = ER_OK;
    status = aboutData.SetDeviceId(MY_DEVICE_ID.c_str());
    if (status != ER_OK) {
        return status;
    }
    status = aboutData.SetAppId(MY_APP_ID, 16);
    if (status != ER_OK) {
        return status;
    }

    status = aboutData.SetAppName("Analytics service example");
    if (status != ER_OK) {
        return status;
    }

    status = aboutData.SetModelNumber("Wxfy388i");
    if (status != ER_OK) {
        return status;
    }

    status = aboutData.SetDateOfManufacture("2199-10-01");
    if (status != ER_OK) {
        return status;
    }

    status = aboutData.SetSoftwareVersion("12.20.44 build 44454");
    if (status != ER_OK) {
        return status;
    }

    status = aboutData.SetHardwareVersion("355.499. b");
    if (status != ER_OK) {
        return status;
    }

    status = aboutData.SetDeviceName("My device name", "en");
    if (status != ER_OK) {
        return status;
    }

    status = aboutData.SetDescription("This is an Alljoyn Application");
    if (status != ER_OK) {
        return status;
    }

    status = aboutData.SetManufacturer("Company");
    if (status != ER_OK) {
        return status;
    }

    status = aboutData.SetSupportUrl("http://www.alljoyn.org");
    if (status != ER_OK) {
        return status;
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

int main(int argc, char**argv, char**envArg)
{
    QStatus status = ER_OK;

    /* Install SIGINT handler so Ctrl + C deallocates memory properly */
    signal(SIGINT, SigIntHandler);

    printf("AllJoyn Library version: %s.\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s.\n", ajn::GetBuildInfo());

    BusAttachment bus("Analytics Service Example", true);

    status = bus.Start();
    if (ER_OK != status) {
        printf("Start of BusAttachment failed (%s).\n", QCC_StatusText(status));
        return EXIT_FAILURE;
    }

    status = bus.EnablePeerSecurity("ALLJOYN_ECDHE_PSK", new ECDHEKeyXListener());
    if (ER_OK != status) {
        printf("EnablePeerSecurity failed (%s).\n", QCC_StatusText(status));
        return EXIT_FAILURE;
    }

    status = bus.Connect();
    if (ER_OK != status) {
        printf("Failed to connect daemon (%s)\n", QCC_StatusText(status));
        return EXIT_FAILURE;
    }

    MySessionPortListener sessionPortListener;
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    SessionPort sp = ASSIGNED_SERVICE_PORT;
    status = bus.BindSessionPort(sp, opts, sessionPortListener);
    if (ER_OK != status) {
        printf("Failed to BindSessionPort (%s)", QCC_StatusText(status));
        return EXIT_FAILURE;
    }

    AboutData aboutData("en");

    status = FillAboutPropertyStoreImplData(aboutData);
    if (ER_OK != status) {
        printf("Failed to set aboutData (%s)\n", QCC_StatusText(status));
        return EXIT_FAILURE;
    }

    TellientDevFactory devFactory;
    AnalyticsBusObject testObj(bus, &devFactory, SERVICE_PATH, INTERFACE_NAME);

    status = testObj.Initialize();
    if (ER_OK != status) {
        printf("Failed to initialize testObj (%s)\n", QCC_StatusText(status));
        return EXIT_FAILURE;
    }

    status = bus.RegisterBusObject(testObj);
    if (ER_OK != status) {
        printf("Failed to register testObj (%s)\n", QCC_StatusText(status));
        return EXIT_FAILURE;
    }

    AboutObj aboutObj(bus);

    status = aboutObj.Announce(ASSIGNED_SERVICE_PORT, aboutData);
    if (ER_OK != status) {
        printf("aboutObj.Announce failed (%s)\n", QCC_StatusText(status));
        return EXIT_FAILURE;
    }
    printf("up and running\n");

    /* Perform the service asynchronously until the user signals for an exit. */
    if (ER_OK == status) {
        WaitForSigInt();
    }

    return 0;
}
