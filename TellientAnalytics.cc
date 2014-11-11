#include "TellientAnalytics.h"
#include "string.h"

#ifndef MAX_EVENT_KEYS
#define MAX_EVENT_KEYS 32 
#endif

#define XSTRINGIFY(x) #x
#define  STRINGIFY(x) XSTRINGIFY(x) 

using namespace ajn ;


uint32_t TellientAnalyticsDeviceObject::globalEventCount = 0 ;

QStatus TellientAnalyticsDeviceObject::SetVendorData(const char **err, size_t count, const ajn::MsgArg *kv )
{
    if (haveVendorData) {
	*err = "SetVendorData can only be called once" ;
	return ER_FAIL;
    }


    const char *post_url = NULL ;
    const char *model = NULL ;
    int32_t manufacturer_id = 0 ;

    for (size_t i = 0 ; i < count ; i++) {
	const char *key ;
	int32_t x ;
	const char *s ;

	if (ER_OK == kv[i].Get("{si}", &key, &x)) {
	    if (0==strcmp(key, "manufacturer_id")) {
		manufacturer_id = x ;
	    }
	} else if (ER_OK == kv[i].Get("{ss}", &key, &s)) {
	    if (0==strcmp(key, "model")) {
		model = s ;
	    } else if (0==strcmp(key, "post_url")) {
		post_url = s ;
	    }
	}
    }

    if (!manufacturer_id) {
	*err = "missing manufacturer_id" ;
	return ER_BAD_ARG_1 ;
    } 
    if (!model) {
	*err = "missing model" ;
	return ER_BAD_ARG_1 ;
    }
    if (!post_url) {
	*err = "missing post_url" ;
	return ER_BAD_ARG_1 ;
    }

    SetVendorData(manufacturer_id, post_url, model) ;

    return ER_OK ;
}

static QStatus argToKV(const char **err, const MsgArg *arg, teKeyValue *kv)
{
    const int ebsm = ER_BUS_SIGNATURE_MISMATCH ;

    if (ebsm != arg->Get("{si}", &kv->name, &kv->value.i32val)) {
	kv->type = TE_I32 ;
    } else if (ebsm != arg->Get("{ss}", &kv->name, &kv->value.stringval)) {
	kv->type = TE_STRING ;
    } else if (ebsm != arg->Get("{sx}", &kv->name, &kv->value.i64val)) {
	kv->type = TE_I64 ;
#if TE_INCLUDE_FLOATING
    } else if (ebsm != arg->Get("{sd}", &kv->name, &kv->value.doubleval)) {
	kv->type = TE_DOUBLE ;
#endif
    } else {
	*err = "Invalid argument type (not i,s,x"
#if TE_INCLUDE_FLOATING
						 ",d"
#endif
						     ")" ;
	return ER_BAD_ARG_1 ;
    }

    return ER_OK ;
}

QStatus TellientAnalyticsDeviceObject::SetDeviceData(const char **err, size_t count, const ajn::MsgArg *args )
{
    if (!haveVendorData) {
	*err = "must call SetVendorData first" ;
	return ER_FAIL;
    }

    deviceData.resize(count) ;
    for (size_t i = 0 ; i < count ; i++) {
	deviceData[i] = args[i] ;   // copy and stabilize.
    }

    return ER_OK ;
}

QStatus TellientAnalyticsDeviceObject::WriteDeviceData(const char **err)
{

    for (size_t i = 0 ; i < deviceData.size() ; ++i) {
	teKeyValue kv ;

	QStatus status = argToKV(err, &deviceData[i], &kv) ;
	if ( status != ER_OK )
	    continue ;

	if (TE_SUCCESS != te_add_defaults(updateState, 1, &kv)) {
	    *err = "out of memory" ;
	    return ER_OUT_OF_MEMORY;
	}
    }

    wroteDeviceData = true ;

    return ER_OK ;
}


QStatus TellientAnalyticsDeviceObject::SubmitEvent(
    const char **err, const char *name, 
    size_t count, const ajn::MsgArg *args, uint64_t timestamp) 
{
    if (!haveVendorData) {
	*err = "must call SetVendorData first" ;
	return ER_FAIL;
    }

    if (count > MAX_EVENT_KEYS) {
	*err = "too many event keys (max " STRINGIFY(MAX_EVENT_KEYS) ")" ;
	return ER_OUT_OF_MEMORY ;
    }

    if (!updateState) {
	wroteDeviceData = false ;
	updateState = new teUpdateState() ;
	if (!updateState ||
	    TE_SUCCESS != te_init_update(updateState, teReallocBufferManager, 
		NULL, 0, manufacturer_id, model.c_str()) 
	) {
	    FreeUpdateState() ;
	    *err = "out of memory" ;
	    return ER_OUT_OF_MEMORY;
	}
    }

    if (!wroteDeviceData) {
	QStatus status = WriteDeviceData(err) ;
	if (status != ER_OK)
	    return status ;
    }

    teKeyValue kv[MAX_EVENT_KEYS] ;

    for (size_t i = 0 ; i < count ; i++) {

	QStatus status = argToKV(err, &args[i], &kv[i]) ;
	if (ER_OK != status) {
	    return ER_BAD_ARG_1 ;
	}

	if ( status != ER_OK )
	    return status ;

    }

    if (TE_SUCCESS != te_add_event(updateState, name, timestamp, 0, count, kv)
    ) {
	*err = "out of memory" ;
	return ER_OUT_OF_MEMORY;
    }

    eventCount++ ;
    globalEventCount++ ;

    return ER_OK ;
}


void TellientAnalyticsDeviceObject::RequestDelivery() 
{
    if (eventCount == 0) {
	return ;
    }

    QStatus status = SendToCloud(postUrl, updateState->used, updateState->buf) ;
    if (ER_OK == status) {
	FreeUpdateState() ;
    }
}


void TellientAnalyticsDeviceObject::SendIfFull() 
{
    if (!updateState) {
	return ;  // no update to send.
    }
    
    if ( ( TE_DEVICE_SOFT_CAP_BYTES && 
	    updateState->used > TE_DEVICE_SOFT_CAP_BYTES )
    ) {
	QStatus status = SendToCloud(postUrl, updateState->used, updateState->buf) ;
	if (ER_OK == status) {
	    FreeUpdateState() ;
	}
    }
}
