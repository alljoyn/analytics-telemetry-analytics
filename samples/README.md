The files in the sample directory provide a Linux implementation of the `org.alljoyn.analytics.AnalyticsEventAgent`. `sample_service.c` adds all the AllJoyn boilerplate, including security, to make a complete service using an instance of the AnalyticsBusObject defined in `../inc/Analytics.h`. `sample_client.c` provides a simple test of the service.

A brief summary of the files:

* `EcdheKeyXListener.h` - Implements ECDHE PSK authentication. A production implementation may want to replace this with a different authentication mechanism.
* `sample_client.cc` - A simple client-side test of the analytics interface.
* `sample_service.cc` - A simple server-side example of a analytics service provider, using the AnalyticsBusObject defined in `../inc/Analytics.h`.
* `TellientAnalytics.cc` - Vendor-specific implementation of the AnalyticsDeviceObject and AnalyticsDeviceObject::Factory from `Analytics.h`. This implementation converts the AllJoyn data to Google protocol buffer format.
* `teclient.c` - Core utility functions for converting event data into Google protocol buffer format. This is a hand-rolled implementation to minimize object code size.
* `TellientSampleHttp.cc` - A simple HTTP client, using libcurl, for posting protobuf data to a server.
* `update.proto` - The protocol buffer definition implemented by teclient.c.

To build, run make.

To execute, start the AllJoyn router and `sample_server`. Run `sample_client` to test the `sample_server` implementation. curl will fail to post the data unless the `post_url` defined in `sample_client` specifies a live server.
