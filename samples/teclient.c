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

#include <assert.h>
#include "teclient.h"

#define PROTOVER 1

#define VARINT 0
#define LENGTHDELIM 2
#define FIXED32 5
#define FIXED64 1

#define fieldtag(fieldnum, type) ((fieldnum <<3) | type)

/* encoded field numbers for update fields. */
#define FIELD_UVERSION  fieldtag(1, VARINT)
#define FIELD_UMFGID    fieldtag(2, VARINT)
#define FIELD_UMODEL    fieldtag(3, LENGTHDELIM)
#define FIELD_UDEVID    fieldtag(4, LENGTHDELIM)
#define FIELD_UMODELVER fieldtag(5, LENGTHDELIM)
#define FIELD_UDEFAULT  fieldtag(7, LENGTHDELIM)
#define FIELD_UEVENT    fieldtag(8, LENGTHDELIM)
#define FIELD_UTIMESTAMP fieldtag(15,VARINT)

/* encoded field numbers for event fields */
#define FIELD_ENAME             fieldtag(1, LENGTHDELIM)
#define FIELD_ETIMESTAMP        fieldtag(2, VARINT)
#define FIELD_ESEQUENCE         fieldtag(4, VARINT)
#define FIELD_EKV               fieldtag(15, LENGTHDELIM)

/* encoded field numbers for kv fields */
#define FIELD_KVNAME    fieldtag(1, LENGTHDELIM)
#define FIELD_KVSVAL    fieldtag(2, LENGTHDELIM)
#define FIELD_KVI32VAL  fieldtag(3, VARINT)
#define FIELD_KVI64VAL  fieldtag(6, VARINT)
#define FIELD_KVFLOATVAL  fieldtag(4, FIXED32)
#define FIELD_KVDOUBLEVAL  fieldtag(5, FIXED64)

#if TE_TIMESTAMP_BITS == 64
    #define TIMESTAMP_WIRELENGTH_FUNC wirelength_int64
    #define TIMESTAMP_WRITE_FUNC write_int64
#else
    #define TIMESTAMP_WIRELENGTH_FUNC wirelength_int32
    #define TIMESTAMP_WRITE_FUNC write_int32
#endif


/* The kv object contains some scratch space.  These are symbolic names
 * for the array indices.
 */
#define KVLENGTH _scratch[0]
#define KVNAMELENGTH _scratch[1]
#define KVSVALLENGTH _scratch[2]


#if TE_ALLOW_REALLOC
/* assure_space implementation for the reallocing buffer manager.
 */
teErrType realloc_assure_space(teUpdateState *statep, unsigned needed)
{
    int32_t size;
    int need_realloc = 0;
    char *newbuf;

    size = statep->buf_size;
    if (0 == size) {
	size = 1024;
        need_realloc = 1;
    }

    while (size-statep->used < needed) {
        size <<= 1;
        need_realloc = 1;
    }

    if (need_realloc) {
        newbuf = (char *) TE_REALLOC((void*)statep->buf, size);
        if (!newbuf)
            return TE_ERR_ALLOC;

        statep->buf = newbuf;
        statep->buf_size = size;
    }

    return TE_SUCCESS;

}
#endif

/* implementation for assure_space for the fixed buffer BufferManager. */
teErrType fixed_assure_space(teUpdateState *statep, unsigned needed)
{
    if (statep->used + needed > statep->buf_size)
        return TE_ERR_ALLOC;
    else
        return TE_SUCCESS;
}

/* implementation for write_byte for the realloc and fixed buffer BufferManagers
 */
void buffer_write_byte(teUpdateState *statep, char byte)
{
    ((char*)statep->buf)[statep->used++] = byte;
    TE_SUCCESS;
}

/* implementation for write_bytes for the realloc and fixed buffer BufferManagers
 */
void buffer_write_bytes(teUpdateState *statep, const char *bytes, unsigned n)
{
    memcpy(statep->buf+statep->used, bytes, n);
    statep->used += n;
}

/* implementation vtable for the fixed-sized buffer manager. */
static teBufferManager _fixed = {
    fixed_assure_space,
    buffer_write_byte,
    buffer_write_bytes
};
const teBufferManager *teFixedBufferManager = &_fixed;

#if TE_ALLOW_REALLOC
/* implementation vtable for the reallocing buffer manager. */
static teBufferManager _reallocator = {
    realloc_assure_space,
    buffer_write_byte,
    buffer_write_bytes
};
const teBufferManager *teReallocBufferManager = &_reallocator;
#endif


/* This is macro is the guts of write_int64, write_int32.
 * uval is an unsigned variable holding the value.
 */
#define WRITE_VARINT(statep, uval) \
        char b ; \
        do { \
            b = uval & 0x7f ; \
            uval = uval >> 7 ; \
            if (uval) { b |= 0x80 ; } \
            statep->mgr->write_byte(statep, b) ; \
        } while (uval) ; \
        return TE_SUCCESS

/* this macro is used to count the number of bytes in
 * the wire protocol to represent an unsigned value.
 */
#define WIRELENGTH_VARINT(uval) \
    unsigned count = 0 ; \
    do { \
        count++ ; \
        uval >>=7 ; \
    } while (uval) ; \
    return count;

static teErrType write_uint32(teUpdateState *statep, uint32_t value)
{
    WRITE_VARINT(statep, value);
}

#if TE_INCLUDE_INT64
static teErrType write_uint64(teUpdateState *statep, uint64_t value)
{
    WRITE_VARINT(statep, value);
}
#endif

static teErrType write_sint32(teUpdateState *statep, int32_t value)
{
    uint32_t uval = (uint32_t)((value<<1) ^ (value >> 31));
    return write_uint32(statep, uval);
}

#if TE_INCLUDE_INT64
static teErrType write_sint64(teUpdateState *statep, int64_t value)
{
    uint64_t uval = (uint64_t)((value<<1) ^ (value >> 63));
    return write_uint64(statep, uval);
}
#endif

static teErrType write_int32(teUpdateState *statep, int32_t value)
{
    uint32_t uval = (uint32_t)(value);
    return write_uint32(statep, uval);
}

#if TE_INCLUDE_INT64
static teErrType write_int64(teUpdateState *statep, int64_t value)
{
    uint64_t uval = (uint64_t)value;
    return write_uint64(statep, uval);
}
#endif


static unsigned int wirelength_uint32(uint32_t value)
{
    WIRELENGTH_VARINT(value);
}

#if TE_INCLUDE_INT64
static unsigned int wirelength_uint64(uint64_t value)
{
    WIRELENGTH_VARINT(value);
}
#endif

static unsigned int wirelength_sint32(int32_t value)
{
    uint32_t uval = (uint32_t)((value<<1) ^ (value >> 31));
    return wirelength_uint32(uval);
}

#if TE_INCLUDE_INT64
static unsigned int wirelength_sint64(int64_t value)
{
    uint64_t uval = (uint64_t)((value<<1) ^ (value >> 31));
    return wirelength_uint64(uval);
}
#endif

static unsigned int wirelength_int32(int32_t value)
{
    uint32_t uval = (uint32_t)(value);
    return wirelength_uint32(uval);
}

#if TE_INCLUDE_INT64
static unsigned int wirelength_int64(int64_t value)
{
    uint64_t uval = (uint64_t)value;
    return wirelength_uint64(uval);
}
#endif

teErrType te_init_update(
    teUpdateState *statep,
    const teBufferManager *mgr,
    void *buffer,
    unsigned buf_size,
    int32_t manufacturer_id,
    const char *model)
{
    int len_model;
    statep->mgr = mgr;
    statep->buf = buffer;
    statep->buf_size = buf_size;
    statep->used = 0;
    statep->hadError = 0;

    len_model = strlen(model);
    if (TE_SUCCESS != statep->mgr->assure_space(statep, len_model + 40))
        return TE_ERR_ALLOC;

    write_int32(statep, FIELD_UVERSION);
    write_int32(statep, PROTOVER);

    write_int32(statep, FIELD_UMFGID);
    write_int32(statep, manufacturer_id);

    write_int32(statep, FIELD_UMODEL);
    write_int32(statep, len_model);
    statep->mgr->write_bytes(statep, model, len_model);

    assert(statep->used < statep->buf_size);

    return TE_SUCCESS;
}


/* Calculates the wire size of a teKeyValue, not including the size header.
 * _scratch[0] will be set to the total size.
 * _scratch[1] will be set to the byte length of the key string.
 * _scratch[2] will be set to the byte length of the value string, if any.
 */
static void precalc_kv_size(teKeyValue *kv)
{
    int vallen ;   /* length of value part */

    kv->KVNAMELENGTH = strlen(kv->name);
    kv->KVLENGTH = 1+ kv->KVNAMELENGTH + wirelength_int32(kv->KVNAMELENGTH);
    switch(kv->type) {
        case TE_STRING:
            kv->KVSVALLENGTH = strlen(kv->value.stringval);
            vallen = kv->KVSVALLENGTH + wirelength_int32(kv->KVSVALLENGTH);
            break;
        case TE_I32:
            vallen = wirelength_sint32(kv->value.i32val);
            break;
#if TE_INCLUDE_FLOATING
        case TE_FLOAT:
            vallen = 4;
            break;
        case TE_DOUBLE:
            vallen = 8;
            break;
#endif
#if TE_INCLUDE_INT64
        case TE_I64:
            vallen = wirelength_sint64(kv->value.i64val);
            break;
    }
#endif

    kv->KVLENGTH += 1 + vallen;
}

/* used for writing floats and doubles */
static void write_endian_bytes(teUpdateState *statep, const void *p, int nbytes)
{
    const char *cp = (const char*)p;
#if TE_LITTLE_ENDIAN
    while(nbytes--) {
        statep->mgr->write_byte(statep, *cp++);
    }
#elif TE_BIG_ENDIAN
    int i;
    for (i = nbytes-1 ; i >= 0 ; i--) {
        statep->mgr->write_byte(statep, cp[i]);
    }
#else
    #error must define TE_LITTLE_ENDIAN or TE_BIG_ENDIAN
#endif
}

/* write out a kv submessage.  Assumes the lengths
 * have been precalculated by precalc_kv_size.
 */
static void write_kv(teUpdateState *statep, teKeyValue *kv)
{

    write_int32(statep, FIELD_KVNAME);
    write_int32(statep, kv->KVNAMELENGTH);
    statep->mgr->write_bytes(statep, kv->name, kv->KVNAMELENGTH);

    switch(kv->type) {
        case TE_STRING:
            write_int32(statep, FIELD_KVSVAL);
            write_int32(statep, kv->KVSVALLENGTH);
            statep->mgr->write_bytes(statep, kv->value.stringval,
                        kv->KVSVALLENGTH);
            break;
        case TE_I32:
            write_int32(statep, FIELD_KVI32VAL);
            write_sint32(statep, kv->value.i32val);
            break;
#if TE_INCLUDE_FLOATING
        case TE_FLOAT:
            write_int32(statep, FIELD_KVFLOATVAL);
            write_endian_bytes(statep, (void*)&kv->value.floatval, 4);
            break;
        case TE_DOUBLE:
            write_int32(statep, FIELD_KVDOUBLEVAL);
            write_endian_bytes(statep, (void*)&kv->value.doubleval, 8);
            break;
#endif
#if TE_INCLUDE_INT64
        case TE_I64:
            write_int32(statep, FIELD_KVI64VAL);
            write_sint64(statep, kv->value.i64val);
            break;
#endif
    }
}


teErrType te_add_event(teUpdateState *statep, const char *name,
       TE_TIMESTAMP_TYPE timestamp, int num_keys, teKeyValue kv[])
{
    unsigned kv_length;
    unsigned event_length = 0;
    unsigned name_length;
    int i;

    for (i = 0 ; i < num_keys ; i++) {
        precalc_kv_size(&kv[i]);

        kv_length = kv[i].KVLENGTH;

        /* one byte for here-comes-a-kv + length kv+length of length of kv.*/
        event_length += 1 + kv_length + wirelength_int32(kv_length);
    }

    /* event_length now contains the number of wire bytes for the key/values. */

    name_length = strlen(name);

    event_length += 1 + name_length + wirelength_int32(name_length);

    if (timestamp)
        event_length += 1 + TIMESTAMP_WIRELENGTH_FUNC(timestamp);

    if (TE_SUCCESS != statep->mgr->assure_space(statep, event_length + 12) )
        return TE_ERR_ALLOC;

    /* we know the buffer is big enough, so start writing to it. */

    write_int32(statep, FIELD_UEVENT);
    write_int32(statep, event_length);

    write_int32(statep, FIELD_ENAME);
    write_int32(statep, name_length);
    statep->mgr->write_bytes(statep, name, name_length);

    if (timestamp) {
        write_int32(statep, FIELD_ETIMESTAMP);
        TIMESTAMP_WRITE_FUNC(statep, timestamp);
    }

    for (i = 0 ; i < num_keys ; i++) {
        write_int32(statep, FIELD_EKV);
        write_int32(statep, kv[i].KVLENGTH);
        write_kv(statep, &kv[i]);
    }

    return TE_SUCCESS;
}

static teErrType write_stringfield(teUpdateState *statep, int field, const char *val)
{
    int len = strlen(val);

    if (TE_SUCCESS != statep->mgr->assure_space(statep, len + 7) )
        return TE_ERR_ALLOC;

    write_int32(statep, field);
    write_int32(statep, len);
    statep->mgr->write_bytes(statep, val, len);
    return TE_SUCCESS;
}

teErrType te_set_device_id(teUpdateState *statep, const char *val)
{
    return write_stringfield(statep, FIELD_UDEVID, val);
}

teErrType te_set_modelver(teUpdateState *statep, const char *val)
{
    return write_stringfield(statep, FIELD_UMODELVER, val);
}


teErrType te_set_timestamp(teUpdateState *statep, TE_TIMESTAMP_TYPE val)
{
    if (TE_SUCCESS != statep->mgr->assure_space(statep, 11))
        return TE_ERR_ALLOC;
    write_int32(statep, FIELD_UTIMESTAMP);
    TIMESTAMP_WRITE_FUNC(statep, val);
    return TE_SUCCESS;
}


teErrType te_add_defaults(teUpdateState *statep,int num_keys,teKeyValue kv[])
{
    int len;
    int i;

    len = 0;
    for (i = 0 ; i < num_keys ; i++) {
        precalc_kv_size(&kv[i]);

        len += 1 + kv[i].KVLENGTH + wirelength_int32(kv[i].KVLENGTH);
    }

    if (TE_SUCCESS != statep->mgr->assure_space(statep,len)) {
        return TE_ERR_ALLOC;
    }

    for (i = 0 ; i < num_keys ; i++) {
        write_int32(statep, FIELD_UDEFAULT);
        write_int32(statep, kv[i].KVLENGTH);
        write_kv(statep, &kv[i]);
    }

    return TE_SUCCESS;
}
