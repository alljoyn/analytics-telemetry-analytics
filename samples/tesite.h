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

/*
 * this file contains settings for tellient code that may need to be customized
 * for your environment.
 */

/* realloc function to use. */
#define TE_REALLOC realloc

#ifndef TE_ALLOW_REALLOC
#define TE_ALLOW_REALLOC 1
#endif


/* need to include something that defines realloc, or whatever
 * TE_REALLOC points at.  You can skip this if you set TE_ALLOW_REALLOC to 0.
 */
#if TE_ALLOW_REALLOC
#include <stdlib.h>
#endif

/* need to typedef int64_t, int32_t, uint64_t, and uint32_t */
#include <stdint.h>

/* need a working memcpy */
#include <string.h>


/* Set one of the following to 1, appropriate for your CPU architecture */
#define TE_BIG_ENDIAN 0
#define TE_LITTLE_ENDIAN 1

/* If you want to send floats or doubles in your messages, set to 1.
 * the format requires valid IEEE floating point values.
 */
#define TE_INCLUDE_FLOATING 1


/**************************************
 *
 * alljoyn component specific settings.
 *
 **************************************/

/*
 * The TelliantAnalyticsDeviceObject will start attempting to push any
 * batched-up event data when this many bytes have accumulated.
 */
#define TE_DEVICE_SOFT_CAP_BYTES 16384

/*
 * If this is >0, the device object will stop accepting events
 * when this many bytes are accumulated.
 */
#define TE_DEVICE_HARD_CAP_BYTES 0


/* batch events for up to this long. */
#define TE_DEVICE_BATCH_MAX_SECONDS 600



