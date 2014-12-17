/******************************************************************************
 *
 *
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

/* realloc function to use. */
#define TE_REALLOC realloc

#ifndef TE_ALLOW_REALLOC
#define TE_ALLOW_REALLOC 1
#endif


/* need to include something that defines realloc, or whatever
 * TE_REALLOC points at.
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
#ifndef TE_INCLUDE_FLOATING
#define TE_INCLUDE_FLOATING 1
#endif

/* If you don't need 64 bit ints, set this to 0.  This can save significant
 * code size on small microcontrollers.
 */
#ifndef TE_INCLUDE_INT64
#define TE_INCLUDE_INT64 1
#endif

/* Set this to the number of bits for the timestamp field.  This must
 * be either 64 or 32, and if 64 TE_INCLUDE_INT64 must be 1.
 * 64 bit timestamps are in milliseconds, and 32 bit timestamps are in
 * seconds.
 */
#define TE_TIMESTAMP_BITS 64
