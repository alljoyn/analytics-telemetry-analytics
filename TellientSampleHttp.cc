#include "TellientAnalytics.h"

#include <curl/curl.h>

/*
 * This is a simple, synchronous cURL-based implementation of the cloud 
 * POST functionality needed by the Tellient analytics implementation.  
 * This will need to be rewritten with appropriate libraries for your
 * application environment.
 *
 * Once this method returns ER_OK, the TellientAnalytics object may reclaim the
 * buffer at any time, so an asynchronous implementation will need to
 * copy the buffer before returning.
 */

QStatus TellientAnalyticsDeviceObject::SendToCloud(qcc::String &post_url, 
    size_t nbytes, void *buffer) 
{
    CURL *request = curl_easy_init() ;
    if (!request) {
	return ER_FAIL ;
    }

    curl_easy_setopt(request, CURLOPT_URL, post_url.c_str()) ;
    curl_easy_setopt(request, CURLOPT_POST, 1) ;
    curl_easy_setopt(request, CURLOPT_POSTFIELDS, buffer) ;
    curl_easy_setopt(request, CURLOPT_POSTFIELDSIZE, nbytes) ;
    curl_easy_setopt(request, CURLOPT_VERBOSE, 1) ;

    struct curl_slist *chunk = NULL ;
    chunk = curl_slist_append(chunk, "Content-type: application/x-protobuf") ;
    curl_easy_setopt(request, CURLOPT_HTTPHEADER, chunk) ;

    if (CURLE_OK != curl_easy_perform(request)) {
	return ER_FAIL ;
    }

    curl_easy_cleanup(request) ;
    curl_slist_free_all(chunk) ;
    return ER_OK ;
}
