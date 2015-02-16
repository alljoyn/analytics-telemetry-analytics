/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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

#ifndef ECDHEKEYXLISTENER_H_
#define ECDHEKEYXLISTENER_H_

#include <alljoyn/AuthListener.h>

/*
 * This is the local implementation of an AuthListener.  ECDHEKeyXListener is
 * designed to only handle PSK ECDHE Key Exchange Authentication requests.
 *
 * If any authMechanism is used other than ECDHE Key Exchange, authentication
 * will fail.
 */
class ECDHEKeyXListener : public ajn::AuthListener {
  public:
    ECDHEKeyXListener() {}

    bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds);

    void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success);

};

#endif /* ECDHEKEYXLISTENER_H_ */
