//=========================================================================
// Copyright (C) 2012 The Elastos Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//=========================================================================

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _ELASTOS_DROID_WEBKIT_WEBVIEW_CHROMIUM_NET_ANDROIDKEYSTORE_H_
#define _ELASTOS_DROID_WEBKIT_WEBVIEW_CHROMIUM_NET_ANDROIDKEYSTORE_H_

#include "elastos/droid/ext/frameworkext.h"
#include <elastos/core/Object.h>

// package org.chromium.net;
// import org.chromium.base.CalledByNative;
// import org.chromium.base.JNINamespace;

namespace Elastos {
namespace Droid {
namespace Webkit {
namespace Webview {
namespace Chromium {
namespace Net {

class ElastosPrivateKey;

/**
  * Specifies all the dependencies from the native OpenSSL engine on an Elastos KeyStore.
  */
// @JNINamespace("net::android")
class ElastosKeyStore
    : public Object
{
public:
    /**
      * Returns the public modulus of a given RSA private key as a byte
      * buffer.
      * This can be used by native code to convert the modulus into
      * an OpenSSL BIGNUM object. Required to craft a custom native RSA
      * object where RSA_size() works as expected.
      *
      * @param key A PrivateKey instance, must implement RSAKey.
      * @return A byte buffer corresponding to the modulus. This is
      * big-endian representation of a BigInteger.
      */
    // @CalledByNative
    virtual CARAPI_(AutoPtr< ArrayOf<Byte> >) GetRSAKeyModulus(
        /* [in] */ ElastosPrivateKey* key) = 0;

    /**
      * Returns the 'Q' parameter of a given DSA private key as a byte
      * buffer.
      * This can be used by native code to convert it into an OpenSSL BIGNUM
      * object where DSA_size() works as expected.
      *
      * @param key A PrivateKey instance. Must implement DSAKey.
      * @return A byte buffer corresponding to the Q parameter. This is
      * a big-endian representation of a BigInteger.
      */
    // @CalledByNative
    virtual CARAPI_(AutoPtr< ArrayOf<Byte> >) GetDSAKeyParamQ(
        /* [in] */ ElastosPrivateKey* key) = 0;

    /**
      * Returns the 'order' parameter of a given ECDSA private key as a
      * a byte buffer.
      * @param key A PrivateKey instance. Must implement ECKey.
      * @return A byte buffer corresponding to the 'order' parameter.
      * This is a big-endian representation of a BigInteger.
      */
    // @CalledByNative
    virtual CARAPI_(AutoPtr< ArrayOf<Byte> >) GetECKeyOrder(
        /* [in] */ ElastosPrivateKey* key) = 0;

    /**
      * Returns the encoded data corresponding to a given PrivateKey.
      * Note that this will fail for platform keys on Android 4.0.4
      * and higher. It can be used on 4.0.3 and older platforms to
      * route around the platform bug described below.
      * @param key A PrivateKey instance
      * @return encoded key as PKCS#8 byte array, can be null.
      */
    // @CalledByNative
    virtual CARAPI_(AutoPtr< ArrayOf<Byte> >) GetPrivateKeyEncodedBytes(
        /* [in] */ ElastosPrivateKey* key) = 0;

    /**
      * Sign a given message with a given PrivateKey object. This method
      * shall only be used to implement signing in the context of SSL
      * client certificate support.
      *
      * The message will actually be a hash, computed by OpenSSL itself,
      * depending on the type of the key. The result should match exactly
      * what the vanilla implementations of the following OpenSSL function
      * calls do:
      *
      *  - For a RSA private key, this should be equivalent to calling
      *    RSA_private_encrypt(..., RSA_PKCS1_PADDING), i.e. it must
      *    generate a raw RSA signature. The message must be either a
      *    combined, 36-byte MD5+SHA1 message digest or a DigestInfo
      *    value wrapping a message digest.
      *
      *  - For a DSA and ECDSA private keys, this should be equivalent to
      *    calling DSA_sign(0,...) and ECDSA_sign(0,...) respectively. The
      *    message must be a hash and the function shall compute a direct
      *    DSA/ECDSA signature for it.
      *
      * @param key The PrivateKey handle.
      * @param message The message to sign.
      * @return signature as a byte buffer.
      *
      * Important: Due to a platform bug, this function will always fail on
      *            Android < 4.2 for RSA PrivateKey objects. See the
      *            getOpenSSLHandleForPrivateKey() below for work-around.
      */
    // @CalledByNative
    virtual CARAPI_(AutoPtr< ArrayOf<Byte> >) RawSignDigestWithPrivateKey(
        /* [in] */ ElastosPrivateKey* key,
        /* [in] */ ArrayOf<Byte>* message) = 0;

    /**
      * Return the type of a given PrivateKey object. This is an integer
      * that maps to one of the values defined by org.chromium.net.PrivateKeyType,
      * which is itself auto-generated from net/android/private_key_type_list.h
      * @param key The PrivateKey handle
      * @return key type, or PrivateKeyType.INVALID if unknown.
      */
    // @CalledByNative
    virtual CARAPI_(Int32) GetPrivateKeyType(
        /* [in] */ ElastosPrivateKey* key) = 0;

    /**
      * Return the system EVP_PKEY handle corresponding to a given PrivateKey
      * object.
      *
      * This shall only be used when the "NONEwithRSA" signature is not
      * available, as described in rawSignDigestWithPrivateKey(). I.e.
      * never use this on Android 4.2 or higher.
      *
      * This can only work in Android 4.0.4 and higher, for older versions
      * of the platform (e.g. 4.0.3), there is no system OpenSSL EVP_PKEY,
      * but the private key contents can be retrieved directly with
      * the getEncoded() method.
      *
      * This assumes that the target device uses a vanilla AOSP
      * implementation of its java.security classes, which is also
      * based on OpenSSL (fortunately, no OEM has apperently changed to
      * a different implementation, according to the Android team).
      *
      * Note that the object returned was created with the platform version
      * of OpenSSL, and _not_ the one that comes with Chromium. Whether the
      * object can be used safely with the Chromium OpenSSL library depends
      * on differences between their actual ABI / implementation details.
      *
      * To better understand what's going on below, please refer to the
      * following source files in the Android 4.0.4 and 4.1 source trees:
      * libcore/luni/src/main/java/org/apache/harmony/xnet/provider/jsse/OpenSSLRSAPrivateKey.java
      * libcore/luni/src/main/native/org_apache_harmony_xnet_provider_jsse_NativeCrypto.cpp
      *
      * @param key The PrivateKey handle.
      * @return The EVP_PKEY handle, as a 32-bit integer (0 if not available)
      */
    // @CalledByNative
    virtual CARAPI_(Int64) GetOpenSSLHandleForPrivateKey(
        /* [in] */ ElastosPrivateKey* key) = 0;

    /**
      * Called when the native OpenSSL engine no longer needs access to the underlying key.
      */
    // @CalledByNative
    virtual CARAPI ReleaseKey(
        /* [in] */ ElastosPrivateKey* key) = 0;

    static CARAPI_(void*) ElaElastosKeyStoreCallback_Init();

private:
    static CARAPI_(AutoPtr<ArrayOf<Byte> >) GetRSAKeyModulus(
        /* [in] */ IInterface* obj,
        /* [in] */ IInterface* key);

    static CARAPI_(AutoPtr<ArrayOf<Byte> >) GetDSAKeyParamQ(
        /* [in] */ IInterface* obj,
        /* [in] */ IInterface* key);

    static CARAPI_(AutoPtr<ArrayOf<Byte> >) GetECKeyOrder(
        /* [in] */ IInterface* obj,
        /* [in] */ IInterface* key);

    static CARAPI_(AutoPtr<ArrayOf<Byte> >) GetPrivateKeyEncodedBytes(
        /* [in] */ IInterface* obj,
        /* [in] */ IInterface* key);

    static CARAPI_(AutoPtr<ArrayOf<Byte> >) RawSignDigestWithPrivateKey(
        /* [in] */ IInterface* obj,
        /* [in] */ IInterface* key,
        /* [in] */ ArrayOf<Byte>* message);

    static CARAPI_(Int32) GetPrivateKeyType(
        /* [in] */ IInterface* obj,
        /* [in] */ IInterface* key);

    static CARAPI_(Int64) GetOpenSSLHandleForPrivateKey(
        /* [in] */ IInterface* obj,
        /* [in] */ IInterface* key);

    static CARAPI_(void) ReleaseKey(
        /* [in] */ IInterface* obj,
        /* [in] */ IInterface* key);
};

} // namespace Net
} // namespace Chromium
} // namespace Webview
} // namespace Webkit
} // namespace Droid
} // namespace Elastos

#endif // _ELASTOS_DROID_WEBKIT_WEBVIEW_CHROMIUM_NET_ANDROIDKEYSTORE_H_

