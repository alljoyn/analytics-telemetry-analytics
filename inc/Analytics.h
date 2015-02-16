/******************************************************************************
 *
 *
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

#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <qcc/Debug.h>
#include <qcc/String.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/MsgArg.h>
#include <alljoyn/version.h>
#include <map>

#define QCC_MODULE "ALLJOYN_ANALYTICS_SERVICE"


/* Pure virtual interface to be implemented by analytics vendor. */
class AnalyticsDeviceObject {
    public:
        /* public methods of the analytics interface, to be implemented. */

        virtual QStatus SubmitEvent(const char **errMsg, const char *name,
                size_t count, const ajn::MsgArg *kvs, uint64_t timestamp = 0) = 0;
        virtual QStatus SetVendorData(const char **errMsg, size_t count,
                const ajn::MsgArg *) = 0;
        virtual QStatus SetDeviceData(const char **errMsg, size_t count,
                const ajn::MsgArg *) = 0;
        virtual void RequestDelivery() {}

        /*
         * called by the bus object when no more method calls are expected.
         * the AnalyticsDeviceObject is expected to flush any buffered data
         * to the cloud service and free itself.  After calling this,
         * the AnalyticsBusObject is no longer responsible for freeing this
         * object.
         */
        virtual void Shutdown() {
            RequestDelivery();
            delete this;
        }

        virtual ~AnalyticsDeviceObject() {};

        /*
         * An AnalyticsDeviceObject::Factory is passed to the constructor of
         * the bus object to tell it how to make the appropriate
         * AnalyticsDeviceObject.  The bus object will use this factory to
         * constuct one device object for each client device connecting to
         * the interface.
         */

        class Factory {
            public:
                virtual AnalyticsDeviceObject *Construct() = 0;
                virtual void Destroy(AnalyticsDeviceObject *ado) { delete ado; }
                virtual ~Factory() {}
        };
};


class AnalyticsBusObject : public ajn::BusObject, ajn::BusListener {

    public:

        /*
         * Utility method to Construct and install the appropriate
         * InterfaceDescription for the analytics interface.  Useful
         * in both a client and a service implementor.
         */

        static QStatus CreateInterface(ajn::BusAttachment &bus, const char *ifname)
        {
            using namespace ajn;
            QStatus status = ER_OK;
            InterfaceDescription *iface = NULL;
            if (!bus.GetInterface(ifname)) {
                status = bus.CreateInterface(ifname, iface, AJ_IFC_SECURITY_REQUIRED);
                if (status != ER_OK) {
                    return status;
                }
                if (!iface) {
                    return ER_BUS_CANNOT_ADD_INTERFACE;
                }

                status = iface->AddMethod("SetVendorData", "a{sv}", NULL, "values", 0);
                if (status != ER_OK) {
                    return status;
                }
                status = iface->AddMethod("SetDeviceData", "a{sv}", NULL, "values", 0);
                if (status != ER_OK) {
                    return status;
                }
                status = iface->AddMethod("RequestDelivery", NULL,NULL, "", 0);
                if (status != ER_OK) {
                    return status;
                }

                status = iface->AddMethod("SubmitEvent","stua{sv}",NULL, "name,timestamp,sequence,values", 0);
                if (status != ER_OK) {
                    return status;
                }
                iface->Activate();
            }
            return ER_OK;
        }


        AnalyticsBusObject(ajn::BusAttachment &bus, AnalyticsDeviceObject::Factory *factory, const char *path, const char *ifname) :
            BusObject(path),
            factory(factory),
            bus(bus),
            ifName(ifname)
        {
        }

        virtual ~AnalyticsBusObject();

        QStatus Initialize()
        {
            QStatus status = ER_OK;
            QCC_DbgTrace(("AnalyticsService::%s", __FUNCTION__));

            CreateInterface(bus, ifName.c_str());
            const ajn::InterfaceDescription* intf = bus.GetInterface(ifName.c_str());
            assert(intf);

            status = AddInterface(*intf, ANNOUNCED);
            if (status != ER_OK) {
                return status;
            }
            const ajn::BusObject::MethodEntry methodEntries[] = {
                { intf->GetMember("SubmitEvent"),
                    static_cast<ajn::MessageReceiver::MethodHandler>(
                            &AnalyticsBusObject::SubmitEvent)
                },
                { intf->GetMember("RequestDelivery"),
                    static_cast<ajn::MessageReceiver::MethodHandler>(
                            &AnalyticsBusObject::RequestDelivery)
                },
                { intf->GetMember("SetDeviceData"),
                    static_cast<ajn::MessageReceiver::MethodHandler>(
                            &AnalyticsBusObject::SetVendorDataOrDeviceData)
                },
                { intf->GetMember("SetVendorData"),
                    static_cast<ajn::MessageReceiver::MethodHandler>(
                            &AnalyticsBusObject::SetVendorDataOrDeviceData)
                },
            };
            status = AddMethodHandlers(methodEntries, sizeof(methodEntries)/sizeof(ajn::BusObject::MethodEntry));
            return status;
        }

        /* Used to detect session disconnect and handle clean up */
        void NameOwnerChanged(const char *, const char *, const char *);

    private:

        /*
         * method to supply vendor-specific data (api keys, etc)
         * method to supply device identification data
         */
        void SetVendorDataOrDeviceData(const ajn::InterfaceDescription::Member*, ajn::Message &msg);

        /* method to request immediate delivery to analytics vendor. */
        void RequestDelivery(const ajn::InterfaceDescription::Member*, ajn::Message &msg);

        /* method to log an analytics event. */
        void SubmitEvent(const ajn::InterfaceDescription::Member*, ajn::Message &msg);

        /*
         * internal method to look up the object based on the bus name of the
         * device that called this method, or Construct one if needed.
         */
        AnalyticsDeviceObject *MakeOrFindDev(ajn::Message &msg);

        AnalyticsDeviceObject::Factory *factory;

        std::map<std::string,AnalyticsDeviceObject *> devMap;

        ajn::BusAttachment &bus;

        qcc::String ifName;

};

#endif
