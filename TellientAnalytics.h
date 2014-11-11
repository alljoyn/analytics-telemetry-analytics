#ifndef TELLIANTANALYTICS_H
#define TELLIANTANALYTICS_H

#include "Analytics.h"
#include <vector>

extern "C" {
#include "teclient.h"
} ;

// Tellient implementation of the Analytics device object.

class TellientAnalyticsDeviceObject : public AnalyticsDeviceObject {
  public:
    TellientAnalyticsDeviceObject() 
    {
	updateState = NULL ;
	haveVendorData = false ;
	wroteDeviceData = false ;
	eventCount = 0 ;
    }

    // these are the required methods for AnalyticsDeviceObject.
    virtual QStatus SubmitEvent(const char **errMsg, const char *name, 
	size_t count, const ajn::MsgArg *kvs, uint64_t timestamp = 0) ;
    virtual QStatus SetVendorData(const char **errMsg, size_t count, 
	const ajn::MsgArg *) ;
    virtual QStatus SetDeviceData(const char **errMsg, size_t count, 
	const ajn::MsgArg *) ;
    virtual void RequestDelivery() ;

    void SetVendorData(int32_t manufacturer_id, const char *post_url, 
                  const char *model) 
    {
	this->manufacturer_id = manufacturer_id ;
	this->postUrl = post_url ;
	this->model = model ;
	this->haveVendorData = true ;
    }

    virtual ~TellientAnalyticsDeviceObject() 
    {
	FreeUpdateState() ;
    }

  private:

    void FreeUpdateState() {
	if (updateState) {
	    free(updateState->buf) ;
	    delete updateState ;
	    globalEventCount -= eventCount ;
	    eventCount = 0 ;
	    wroteDeviceData = false ;
	    updateState = NULL ;
	}
    }

    // internal method to write device data into the output buffer.
    QStatus WriteDeviceData(const char **err) ;

    // utility method to POST the protobuf to the cloud service.
    // When this method returns ER_OK, the buffer can be reclaimed.
    static QStatus SendToCloud(qcc::String &url, size_t nbytes, void *buffer) ;

    // method to send batched data to the cloud if limits are reached,
    // such as maximum number of events, maximum bytes, etc.
    void SendIfFull() ;

    teUpdateState *updateState ;

    // vendor data
    bool haveVendorData ;
    int manufacturer_id ;
    qcc::String model ;
    qcc::String postUrl ;

    bool wroteDeviceData ;
    std::vector<ajn::MsgArg> deviceData ;

    uint32_t eventCount ;  // number of events batched up

    static uint32_t globalEventCount ;  // number of events batched up (all devices)
} ;

class TellientDevFactory : public AnalyticsDeviceObject::Factory {
  public:
    virtual AnalyticsDeviceObject *Construct() 
    {
	return new TellientAnalyticsDeviceObject() ;
    }
    virtual void Destroy(AnalyticsDeviceObject *x) 
    {
	delete x ;
    } ;

    ~TellientDevFactory() {} 
} ;

#endif
