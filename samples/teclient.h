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

#ifndef TECLIENT_H
#define TECLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tesite.h"

#if TE_TIMESTAMP_BITS == 64
#define TE_TIMESTAMP_TYPE uint64_t
#else
#define TE_TIMESTAMP_TYPE uint32_t
#endif

    struct teUpdateState;

    typedef enum {
        TE_SUCCESS=0,
        TE_ERR_ALLOC=1,
        TE_ERR_UNKNOWN=999
    } teErrType;

    typedef enum {
        TE_STRING=2,
        TE_I32=3,
        TE_FLOAT=4,
        TE_DOUBLE=5,
        TE_I64=6
    } teDataType;


    /* a teBufferManager describes how to add bytes to an update.  There
     * are a couple built in ones provided below (teFixedBuffermanager and
     * teReallocBufferManager.  If you want to write your update directly to
     * a socket or to a file, you could implement your own versions of these
     * functions.  You could treat statep->buf as a FILE*, for example.
     */
    typedef struct {
        /* assure_space is expected to make sure there is enough room in the
         * buffer to add the given number of bytes.  It should return TE_SUCCESS
         * unless no more space can be allocated.
         */
        teErrType (*assure_space)(struct teUpdateState *statep, unsigned bytes_needed);

        /* write_byte and write_bytes add bytes to an update.
         * They are expected to append the specified bytes to the buffer,
         * and keep the buf, buf_size, and used members of statep up-to-date.
         * These functions will only be called when assure_space has indicated
         * that it is safe to do so, and are not expected to fail.  However,
         * implementations can indicate failure using statep->hadError, which
         * may be checked later by your application to make sure the update is
         * complete.
         */
        void (*write_byte)(struct teUpdateState *statep, char byte);
        void (*write_bytes)(struct teUpdateState *statep,const char *src,unsigned bytes);
    } teBufferManager;

    extern const teBufferManager *teFixedBufferManager;
    extern const teBufferManager *teReallocBufferManager;

    typedef struct teUpdateState {
        const teBufferManager *mgr;
        void *buf;
        /* total current capacity of current buffer. */
        int32_t buf_size;
        /* number of bytes in the update so far. */
        int32_t used;
        /* Available for use by a custom teBufferManager */
        int hadError;
    } teUpdateState;

    typedef struct teKeyValue {
        const char *name;
        teDataType type;
        union {
            int32_t i32val;
            int64_t i64val;
#if TE_INCLUDE_FLOATING
            float floatval;
            double doubleval;
#endif
            const char *stringval;
        } value;

        /* used internally by the library. */
        int _scratch[3];
    } teKeyValue;

    teErrType te_init_update( teUpdateState *statep, const teBufferManager *mgr,
            void *buf, unsigned buf_size,
            int32_t manufacturer_id, const char *model);


    teErrType te_set_device_id(teUpdateState *statep, const char *);
    teErrType te_set_modelver(teUpdateState *statep, const char *);
    teErrType te_set_timestamp(teUpdateState *statep, TE_TIMESTAMP_TYPE);

    teErrType te_add_defaults(teUpdateState *statep,int num_keys,teKeyValue kv[]);

    teErrType te_add_event(teUpdateState *statep, const char *name,
            TE_TIMESTAMP_TYPE timestamp,
            int num_keys, teKeyValue kv[]);

#ifdef __cplusplus
}
#endif

#endif
