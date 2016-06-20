
#include "elastos/droid/phone/CallerInfoCache.h"

namespace Elastos {
namespace Droid {
namespace Phone {


ECode CallerInfoCache::CacheEntry::ToString(
    /* [out] */ String* str)
{
    VALIDATE_NOT_NULL(str)

    StringBuilder sb;
    sb += "ringtone: ";
    sb += mCustomRingtone;
    sb += ", ";
    sb += mSendToVoicemail;
    *str = sb.ToString();
    return NOERROR;
}

ECode CallerInfoCache::CacheAsyncTask::AcquireWakeLockAndExecute()
{
    // Prepare a separate partial WakeLock than what PhoneApp has so to avoid
    // unnecessary conflict.
    AutoPtr<IInterface> obj;
    mContext->GetSystemService(IContext::POWER_SERVICE, (IInterface**)&obj);
    AutoPtr<IPowerManager> pm = IPowerManager::Probe(obj);
    pm->NewWakeLock(IPowerManager::PARTIAL_WAKE_LOCK, LOG_TAG, (IPowerManagerWakeLock**)&mWakeLock);
    mWakeLock->Acquire();
    return Execute();
}

ECode CallerInfoCache::CacheAsyncTask::DoInBackground(
    /* [in] */ ArrayOf<IInterface*>* params,
    /* [out] */ IInterface** result)
{
    VALIDATE_NOT_NULL(result)

    if (DBG) Log(String("Start refreshing cache."));
    RefreshCacheEntry();
    *result = NULL;
    return NOERROR;
}

ECode CallerInfoCache::CacheAsyncTask::OnPostExecute(
    /* [in] */ IInterface* result)
{
    if (VDBG) Log(String("CacheAsyncTask#onPostExecute()"));
    AsyncTask::OnPostExecute(result);
    return ReleaseWakeLock();
}

ECode CallerInfoCache::CacheAsyncTask::OnCancelled(
    /* [in] */ IInterface* result)
{
    if (VDBG) Log(String("CacheAsyncTask#onCanceled()"));
    AsyncTask::OnCancelled(result);
    return ReleaseWakeLock();
}

ECode CallerInfoCache::CacheAsyncTask::ReleaseWakeLock()
{
    Boolean res;
    if (mWakeLock != NULL && (mWakeLock->IsHeld(&res), res)) {
        return mWakeLock->ReleaseLock();
    }
    return NOERROR;
}

const Int32 CallerInfoCache::MESSAGE_UPDATE_CACHE = 0;

const String CallerInfoCache::LOG_TAG("CallerInfoCache");// = CallerInfoCache.class.getSimpleName();

static Boolean initDBG()
{
    AutoPtr<ISystemProperties> helper;
    CSystemProperties::AcquireSingleton((ISystemProperties**)&helper);
    Int32 value;
    helper->GetInt32(String("ro.debuggable"), 0, &value);
    Boolean result = (PhoneGlobals::DBG_LEVEL >= 1) && (value == 1);
    return result;
}

const Boolean CallerInfoCache::DBG = initDBG();
const Boolean CallerInfoCache::VDBG = FALSE;

static AutoPtr<ArrayOf<String> > initPROJECTION()
{
    AutoPtr<ArrayOf<String> > array = ArrayOf<String>::Alloc(4);
    array->Set(0, IData::DATA1);              // 0
    array->Set(1, IPhone::NORMALIZED_NUMBER); // 1
    array->Set(2, IData::CUSTOM_RINGTONE);    // 2
    array->Set(3, IData::SEND_TO_VOICEMAIL);  // 3
}

const AutoPtr<ArrayOf<String> > CallerInfoCache::PROJECTION = initPROJECTION();

const Int32 CallerInfoCache::INDEX_NUMBER            = 0;
const Int32 CallerInfoCache::INDEX_NORMALIZED_NUMBER = 1;
const Int32 CallerInfoCache::INDEX_CUSTOM_RINGTONE   = 2;
const Int32 CallerInfoCache::INDEX_SEND_TO_VOICEMAIL = 3;

static String initSELECTION()
{
    StringBuilder sb;
    sb += "(";
    sb += "(";
    sb += IData::CUSTOM_RINGTONE;
    sb += " IS NOT NULL OR ";
    sb += IData::SEND_TO_VOICEMAIL;
    sb += "=1)";
    sb += " AND ";
    sb += IData::DATA1;
    sb += " IS NOT NULL)";
    return sb.ToString();
}

const String CallerInfoCache::SELECTION = initSELECTION();

AutoPtr<CallerInfoCache> CallerInfoCache::Init(
    /* [in] */ IContext* context)
{
    if (DBG) Log(String("init()"));
    AutoPtr<CallerInfoCache> cache = new CallerInfoCache(context);
    // The first cache should be available ASAP.
    cache->StartAsyncCache();
    return cache;
}

CallerInfoCache::CallerInfoCache(
    /* [in] */ IContext* context)
    : mContext(context)
{
    CHashMap::New((IHashMap**)&mNumberToEntry);
}

ECode CallerInfoCache::StartAsyncCache()
{
    if (DBG) Log(String("startAsyncCache"));

    if (mCacheAsyncTask != NULL) {
        Logger::W(LOG_TAG, "Previous cache task is remaining.");
        mCacheAsyncTask->Cancel(TRUE);
    }
    mCacheAsyncTask = new CacheAsyncTask();
    return mCacheAsyncTask->AcquireWakeLockAndExecute();
}


void CallerInfoCache::RefreshCacheEntry()
{
    if (VDBG) Log(String("refreshCacheEntry() started"));

    // There's no way to know which part of the database was updated. Also we don't want
    // to block incoming calls asking for the cache. So this method just does full query
    // and replaces the older cache with newer one. To refrain from blocking incoming calls,
    // it keeps older one as much as it can, and replaces it with newer one inside a very small
    // synchronized block.

    AutoPtr<ICursor> cursor;
    //try
    {
        AutoPtr<IContentResolver> resolver;
        FAIL_GOTO(mContext->GetContentResolver((IContentResolver**)&resolver), FINALLY)
        FAIL_GOTO(resolver->Query(ICallable::CONTENT_URI,
                PROJECTION, SELECTION, NULL, NULL, (ICursor**)&cursor), FINALLY)
        if (cursor != NULL) {
            // We don't want to block real in-coming call, so prepare a completely fresh
            // cache here again, and replace it with older one.
            Int32 count;
            FAIL_GOTO(cursor->GetCount(&count), FINALLY)
            AutoPtr<IHashMap> newNumberToEntry;
            CHashMap::New(count, (IHashMap**)&newNumberToEntry);

            Boolean res;
            while (cursor->MoveToNext(&res), res) {
                String number;
                FAIL_GOTO(cursor->GetString(INDEX_NUMBER, &number), FINALLY)
                String normalizedNumber;
                FAIL_GOTO(cursor->GetString(INDEX_NORMALIZED_NUMBER, &normalizedNumber), FINALLY)
                if (normalizedNumber.IsNull()) {
                    // There's no guarantee normalized numbers are available every time and
                    // it may become null sometimes. Try formatting the original number.
                    normalizedNumber = PhoneNumberUtils::NormalizeNumber(number);
                }
                String customRingtone;
                FAIL_GOTO(cursor->GetString(INDEX_CUSTOM_RINGTONE, &customRingtone), FINALLY)
                Int32 num;
                FAIL_GOTO(cursor->GetInt32(INDEX_SEND_TO_VOICEMAIL, &num), FINALLY)
                Boolean sendToVoicemail = num == 1;

                if (PhoneNumberUtils::IsUriNumber(number)) {
                    // SIP address case
                    PutNewEntryWhenAppropriate(
                            newNumberToEntry, number, customRingtone, sendToVoicemail);
                }
                else {
                    // PSTN number case
                    // Each normalized number may or may not have full content of the number.
                    // Contacts database may contain +15001234567 while a dialed number may be
                    // just 5001234567. Also we may have inappropriate country
                    // code in some cases (e.g. when the location of the device is inconsistent
                    // with the device's place). So to avoid confusion we just rely on the last
                    // 7 digits here. It may cause some kind of wrong behavior, which is
                    // unavoidable anyway in very rare cases..
                    Int32 length = normalizedNumber->GetLength();
                    String key = length > 7
                            ? normalizedNumber.Substring(length - 7, length)
                                    : normalizedNumber;
                    PutNewEntryWhenAppropriate(
                            newNumberToEntry, key, customRingtone, sendToVoicemail);
                }
            }

            if (VDBG) {
                Int32 size;
                newNumberToEntry->GetSize(&size);
                Logger::D(LOG_TAG, "New cache size: %d",size);
                // for (Entry<String, CacheEntry> entry : newNumberToEntry.entrySet()) {
                //     Log.d(LOG_TAG, "Number: " + entry.getKey() + " -> " + entry.getValue());
                // }
            }

            mNumberToEntry = newNumberToEntry;

            if (DBG) {
                StringBuilder sb;
                sb += "Caching entries are done. Total: ";
                Int32 size;
                newNumberToEntry->GetSize(&size);
                sb += size;
                Log(sb.ToString());
            }
        }
        else {
            // Let's just wait for the next refresh..
            //
            // If the cursor became null at that exact moment, probably we don't want to
            // drop old cache. Also the case is fairly rare in usual cases unless acore being
            // killed, so we don't take care much of this case.
            Logger::W(LOG_TAG, "cursor is null");
        }
    } //finally {
FINALLY:
    if (cursor != NULL) {
        cursor->Close();
    }
    //}

    if (VDBG) Log(String("refreshCacheEntry() ended"));
}

void CallerInfoCache::PutNewEntryWhenAppropriate(
    /* [in] */ IHashMap* newNumberToEntry,
    /* [in] */ const String& numberOrSipAddress,
    /* [in] */ const String& customRingtone,
    /* [in] */ Boolean sendToVoicemail)
{
    AutoPtr<ICharSequence> cs = CoreUtil::Convert(numberOrSipAddress);
    Boolean res;
    if (newNumberToEntry->ContainsKey(TO_IINTERFACE(cs), &res), res) {
        // There may be duplicate entries here and we should prioritize
        // "send-to-voicemail" flag in any case.
        AutoPtr<IInterface> obj;
        newNumberToEntry->Get(TO_IINTERFACE(cs), (IInterface**)&obj);
        AutoPtr<CacheEntry> entry = (CacheEntry*)IObject::Probe(obj);
        if (!entry->mSendToVoicemail && sendToVoicemail) {
            AutoPtr<CacheEntry> entry = new CacheEntry(customRingtone, sendToVoicemail);
            newNumberToEntry->Put(TO_IINTERFACE(cs), TO_IINTERFACE(entry));
        }
    }
    else {
        AutoPtr<CacheEntry> entry = new CacheEntry(customRingtone, sendToVoicemail);
        newNumberToEntry.put(TO_IINTERFACE(cs), TO_IINTERFACE(entry));
    }
    return;
}

AutoPtr<CacheEntry> CallerInfoCache::GetCacheEntry(
    /* [in] */ const String& number)
{
    if (mNumberToEntry == NULL) {
        // Very unusual state. This implies the cache isn't ready during the request, while
        // it should be prepared on the boot time (i.e. a way before even the first request).
        Logger::W(LOG_TAG, "Fallback cache isn't ready.");
        return NULL;
    }

    AutoPtr<CacheEntry> entry;
    if (PhoneNumberUtils::IsUriNumber(number)) {
        if (VDBG) {
            StringBuilder sb;
            sb += "Trying to lookup ";
            sb += number;
            Log(sb.ToString());
        }

        AutoPtr<ICharSequence> cs = CoreUtil::Convert(number);
        AutoPtr<IInterface> obj;
        mNumberToEntry->Get(TO_IINTERFACE(cs), (IInterface**)&obj);
        entry = (CacheEntry*)IObject::Probe(obj);
    }
    else {
        String normalizedNumber = PhoneNumberUtils::NormalizeNumber(number);
        Int32 length = normalizedNumber.GetLength();
        String key = (length > 7 ? normalizedNumber.Substring(length - 7, length)
                        : normalizedNumber);
        if (VDBG) {
            StringBuilder sb;
            sb += "Trying to lookup ";
            sb += key;
            Log(sb.ToString());
        }

        AutoPtr<ICharSequence> cs = CoreUtil::Convert(key);
        AutoPtr<IInterface> obj;
        mNumberToEntry->Get(TO_IINTERFACE(cs), (IInterface**)&obj);
        entry = (CacheEntry*)IObject::Probe(obj);
    }
    if (VDBG) {
        StringBuilder sb;
        sb += "Obtained ";
        sb += TO_CSTR(entry);
        Log(sb.ToString());
    }
    return entry;
}

void CallerInfoCache::Log(
    /* [in] */ const String& msg)
{
    Logger::D(LOG_TAG, msg);
}

} // namespace Phone
} // namespace Droid
} // namespace Elastos