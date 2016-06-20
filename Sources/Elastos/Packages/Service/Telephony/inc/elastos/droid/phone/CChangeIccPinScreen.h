#ifndef  __ELASTOS_DROID_PHONE_CCHANGEICCPINSCREEN_H__
#define  __ELASTOS_DROID_PHONE_CCHANGEICCPINSCREEN_H__

#include "_Elastos_Droid_Phone_CChangeIccPinScreen.h"
#include "elastos/droid/ext/frameworkext.h"

namespace Elastos {
namespace Droid {
namespace Phone {

/**
 * "Change ICC PIN" UI for the Phone app.
 */
CarClass(CChangeIccPinScreen)
    , public Activity
    , public IChangeIccPinScreen
{
private:
    class MyHandler
        : public Handler
    {
    public:
        TO_STRING_IMPL("CChangeIccPinScreen::MyHandler")

        MyHandler(
            /* [in] */ CChangeIccPinScreen* host);

        // @Override
        CARAPI HandleMessage(
            /* [in] */ IMessage* msg);

    private:
        CChangeIccPinScreen* mHost;
    };

    class MyViewOnClickListener
        : public Object
        , public IViewOnClickListener
    {
    public:
        TO_STRING_IMPL("CChangeIccPinScreen::MyViewOnClickListener")

        CAR_INTERFACE_DECL()

        MyViewOnClickListener(
            /* [in] */ CChangeIccPinScreen* host)
            : mHost(host)
        {}

        CARAPI OnClick(
            /* [in] */ IView* v);

    private:
        CChangeIccPinScreen* mHost;
    };

    class MyRunnable
        : public Runnable
    {
    public:
        TO_STRING_IMPL("CChangeIccPinScreen::MyRunnable")

        MyRunnable(
            /* [in] */ CChangeIccPinScreen* host)
            : mHost(host)
        {}

        CARAPI Run();

    private:
        CChangeIccPinScreen* mHost;
    };

    class MyRunnable2
        : public Runnable
    {
    public:
        TO_STRING_IMPL("CChangeIccPinScreen::MyRunnable2")

        MyRunnable2(
            /* [in] */ CChangeIccPinScreen* host)
            : mHost(host)
        {}

        CARAPI Run();

    private:
        CChangeIccPinScreen* mHost;
    };

public:
    CAR_INTERFACE_DECL()

    CAR_OBJECT_DECL();

    CChangeIccPinScreen();

    CARAPI constructor();

    //@Override
    CARAPI OnCreate();

private:
    CARAPI_(void) ResolveIntent();

    CARAPI_(void) Reset();

    CARAPI_(Int32) ValidateNewPin(
        /* [in] */ const String& p1,
        /* [in] */ const String& p2);

    CARAPI_(void) HandleResult(
        /* [in] */ AsyncResult ar);

    CARAPI_(void) DisplayPUKAlert();

    CARAPI_(void) ShowConfirmation();

    CARAPI_(void) Log(
        /* [in] */ const String& msg);

private:
    static const String LOG_TAG;
    static const Boolean DBG;

    static const Int32 EVENT_PIN_CHANGED;

    enum EntryState {
        ES_PIN,
        ES_PUK
    };

    EntryState mState;

    static const Int32 NO_ERROR;
    static const Int32 PIN_MISMATCH;
    static const Int32 PIN_INVALID_LENGTH;

    static const Int32 MIN_PIN_LENGTH;
    static const Int32 MAX_PIN_LENGTH;

    AutoPtr<IPhone> mPhone;
    Boolean mChangePin2;
    AutoPtr<ITextView> mBadPinError;
    AutoPtr<ITextView> mMismatchError;
    AutoPtr<IEditText> mOldPin;
    AutoPtr<IEditText> mNewPin1;
    AutoPtr<IEditText> mNewPin2;
    AutoPtr<IEditText> mPUKCode;
    AutoPtr<IButton> mButton;
    AutoPtr<IButton> mPUKSubmit;
    AutoPtr<IScrollView> mScrollView;

    AutoPtr<ILinearLayout> mIccPUKPanel;

    AutoPtr<IHandler> mHandler;

    AutoPtr<IViewOnClickListener> mClicked;

    AutoPtr<IAlertDialog> mPUKAlert;
};

} // namespace Phone
} // namespace Droid
} // namespace Elastos


#endif // __ELASTOS_DROID_PHONE_CCHANGEICCPINSCREENP_H__