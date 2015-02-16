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

#include <stdio.h>
#include <qcc/platform.h>
#include "ECDHEKeyXListener.h"

bool ECDHEKeyXListener::RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds)
{
    printf("RequestCredentials for authenticating peer name %s using mechanism %s authCount %d\n", authPeer, authMechanism, authCount);
    if (strcmp(authMechanism, "ALLJOYN_ECDHE_PSK") == 0) {
        /*
         * Solicit the Pre shared secret
         */
        if ((credMask & AuthListener::CRED_USER_NAME) == AuthListener::CRED_USER_NAME) {
            printf("RequestCredentials received psk ID %s\n", creds.GetUserName().c_str());
        }
        /*
         * Based on the pre shared secret id, the application can retrieve
         * the pre shared secret from storage or from the end user.
         * In this example, the pre shared secret is a hard coded string
         */
        qcc::String psk("123456");
        creds.SetPassword(psk);
        creds.SetExpiration(100);  /* set the master secret expiry time to 100 seconds */
        return true;
    }
    return false;
}

void ECDHEKeyXListener::AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success) {
    printf("AuthenticationComplete %s %s\n", authMechanism, success ? "successful" : "failed");
}
