/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.internal.telephony.uicc;

/**
 * {@hide}
 */
public interface IsimRecords {

    /**
     * Return the IMS private user Identity (IMPI).
     * Returns NULL if the IMPI hasn't been loaded or isn't present on the ISIM.
     * @return the IMS private user identity string, or NULL if not available
     */
    String GetIsimImpi();

    /**
     * Return the IMS home network domain name.
     * Returns NULL if the IMS domain hasn't been loaded or isn't present on the ISIM.
     * @return the IMS home network domain name, or NULL if not available
     */
    String GetIsimDomain();

    /**
     * Return an array of IMS public user Identities (IMPU).
     * Returns NULL if the IMPU hasn't been loaded or isn't present on the ISIM.
     * @return an array of IMS public user identity strings, or NULL if not available
     */
    String[] GetIsimImpu();

    /**
     * Returns the IMS Service Table (IST) that was loaded from the ISIM.
     * @return IMS Service Table or NULL if not present or not loaded
     */
    String GetIsimIst();

    /**
     * Returns the IMS Proxy Call Session Control Function(PCSCF) that were loaded from the ISIM.
     * @return an array of  PCSCF strings with one PCSCF per string, or NULL if
     *      not present or not loaded
     */
    String[] GetIsimPcscf();

    /**
     * Returns the response of ISIM Authetification through RIL.
     * Returns NULL if the Authentification hasn't been successed or isn't present iphonesubinfo.
     * @return the response of ISIM Authetification, or NULL if not available
     */
    String GetIsimChallengeResponse(String nonce);
}