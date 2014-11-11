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

#include "Analytics.h"

using namespace ajn ;
#include <stdio.h>

AnalyticsDeviceObject *AnalyticsBusObject::MakeOrFindDev(ajn::Message &msg)
{
    AnalyticsDeviceObject *dev = devMap[msg->GetSender()] ;
    if (dev) {
	return dev ;
    }

    dev = factory->Construct() ;
    devMap[msg->GetSender()] = dev ;

    return dev ;
}

AnalyticsBusObject::~AnalyticsBusObject()
{
    std::map<std::string,AnalyticsDeviceObject *>::iterator it ;
    for (it = devMap.begin() ; it != devMap.end() ; ++it) {
	it->second->Shutdown() ;
    }
    bus.UnregisterBusListener(*this) ;
}

void AnalyticsBusObject::SetVendorDataOrDeviceData(const ajn::InterfaceDescription::Member *member, Message &msg)
{
    AnalyticsDeviceObject *dev= MakeOrFindDev(msg) ;
    if (!dev) {
	MethodReply(msg, (const MsgArg*)NULL, 0) ;
	return ;
    }

    const MsgArg *arg0 = msg->GetArg(0) ;
    const MsgArg *entries ;
    size_t asize ;
    if (!arg0 || ER_OK != arg0->Get("a{sv}", &asize, &entries)) {
	MethodReply(msg, QCC_StatusText(ER_BAD_ARG_1), "expecting a{sv}") ;
	return ;
    }

    const char *err ;
    QStatus status ;
    if (member->name == "SetVendorData") {
	status = dev->SetVendorData(&err, asize, entries) ;
    } else {
	status = dev->SetDeviceData(&err, asize, entries) ;
    } if (status == ER_OK) {
	MethodReply(msg, (MsgArg*)NULL, 0) ;
    } else {
	MethodReply(msg, QCC_StatusText(status), err) ;
    }
}

void AnalyticsBusObject::RequestDelivery(const InterfaceDescription::Member *, Message &msg)
{
    AnalyticsDeviceObject *dev= MakeOrFindDev(msg) ;
    if (!dev) {
	MethodReply(msg, (MsgArg*)NULL, 0) ;
	return ;
    }
    dev->RequestDelivery() ;
    MethodReply(msg, (MsgArg*)NULL, 0) ;
}

void AnalyticsBusObject::SubmitEvent(const InterfaceDescription::Member *, Message &msg)
{
    AnalyticsDeviceObject *dev = MakeOrFindDev(msg) ;
    if (!dev) {
	MethodReply(msg, (MsgArg*)NULL, 0) ;
	return ;
    }

    const char *name ;
    uint64_t timestamp ;
    uint32_t sequence ;
    size_t asize ;
    const MsgArg *kvs ;

    QStatus status = msg->GetArgs("stua{sv}", &name, &timestamp, 
	    &sequence, &asize, &kvs) ;
    if (ER_OK != status) {
	MethodReply(msg, status) ;
	return ;
    }

    const char *err ;
    status = dev->SubmitEvent(&err, name, asize, kvs, timestamp) ;

    if (status == ER_OK) {
	MethodReply(msg, (MsgArg*)NULL, 0) ;
    } else {
	MethodReply(msg, QCC_StatusText(status), err) ;
    }
}

void AnalyticsBusObject::NameOwnerChanged(const char *busName, 
    const char *previousOwner, const char *newOwner) 
{
    AnalyticsDeviceObject *dev = devMap[busName] ;
    if (!dev) {
	return ;
    }

    dev->Shutdown() ;
    devMap.erase(busName) ;
}
