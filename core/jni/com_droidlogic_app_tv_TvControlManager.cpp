#define LOG_TAG "Tv-JNI"
#include <utils/Log.h>

#include "tvcmd.h"
#include <jni.h>
#include <utils/Vector.h>
#include <binder/IMemory.h>
#include <binder/Parcel.h>
#include <binder/MemoryHeapBase.h>
#include <binder/MemoryBase.h>
#include <core/SkBitmap.h>
#include "android_util_Binder.h"
#include "android_os_Parcel.h"
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "TvServerHidlClient.h"

using namespace android;

struct fields_t {
    jfieldID context;
    jmethodID post_event;
};

static fields_t fields;
static Mutex sLock;
class JNITvContext: public TvListener {
public:
    JNITvContext(JNIEnv *env, jobject weak_this, jclass clazz, const sp<TvServerHidlClient> &tv);
    ~JNITvContext()
    {
        release();
    }
    virtual void notify(const tv_parcel_t &parcel);
    void addCallbackBuffer(JNIEnv *env, jbyteArray cbb);
    sp<TvServerHidlClient> getTv()
    {
        Mutex::Autolock _l(mLock);
        return mTv;
    }
    void release();
    Parcel *mExtParcel;
    SkBitmap *pSubBmp;//for UI subtitle Bitmap
    sp<MemoryBase> mSubMemBase;//for subtitle shar memory to tvapi
private:
    jobject     mTvJObjectWeak;     // weak reference to java object
    jclass      mTvJClass;          // strong reference to java class
    sp<TvServerHidlClient> mTv;                // strong reference to native object
    Mutex       mLock;

    Vector<jbyteArray> mCallbackBuffers;    // Global reference application managed byte[]
    bool mManualBufferMode;                 // Whether to use application managed buffers.
    bool mManualTvCallbackSet;              // Whether the callback has been set, used to reduce unnecessary calls to set the callback.
    JNIEnv *mEnv;
};

#define CAPTURE_VIDEO 0
#define CAPTURE_GRAPHICS 1

#define CAPTURE_MAX_BITMAP_W 1920
#define CAPTURE_MAX_BITMAP_H 1080

sp<TvServerHidlClient> get_native_tv(JNIEnv *env, jobject thiz, JNITvContext **pContext)
{
    sp<TvServerHidlClient> tv = nullptr;
    Mutex::Autolock _l(sLock);
    JNITvContext *context = reinterpret_cast<JNITvContext *>(env->GetLongField(thiz, fields.context));
    if (context != NULL) {
        tv = context->getTv();
    }
    if (tv == 0) {
        jclass clazz = env->FindClass("java/lang/RuntimeException");
        env->ThrowNew(clazz, "Method called after release()");
        env->DeleteLocalRef(clazz);
    }

    if (pContext != NULL) *pContext = context;
    return tv;
}

JNITvContext::JNITvContext(JNIEnv *env, jobject weak_this, jclass clazz, const sp<TvServerHidlClient> &tv)
{
    mTvJObjectWeak = env->NewGlobalRef(weak_this);
    mTvJClass = (jclass)env->NewGlobalRef(clazz);
    mTv = tv;
    ALOGD("tvjni----------------------JNITvContext::JNITvContext(");
    mManualBufferMode = false;
    mManualTvCallbackSet = false;
    pSubBmp = NULL;
    mSubMemBase = NULL;
    mEnv = env;
    //mExtParcel = parcelForJavaObject(env, ext_parcel);
}

void JNITvContext::release()
{
    ALOGD("release");
    Mutex::Autolock _l(mLock);
    if (mEnv != NULL) {
        if (mTvJObjectWeak != NULL) {
            mEnv->DeleteGlobalRef(mTvJObjectWeak);
            mTvJObjectWeak = NULL;
        }
        if (mTvJClass != NULL) {
            mEnv->DeleteGlobalRef(mTvJClass);
            mTvJClass = NULL;
        }
        if (pSubBmp != NULL)
            pSubBmp = NULL;
    }
    mEnv = NULL;
    mTv.clear();
}

// connect to tv service
static void com_droidlogic_app_tv_TvControlManager_native_setup(JNIEnv *env, jobject thiz, jobject weak_this)
{
    ALOGD("com_droidlogic_app_tv_TvControlManager_native_setup.");

    sp<TvServerHidlClient> tv = TvServerHidlClient::connect(CONNECT_TYPE_EXTEND);
    if (tv == nullptr) {
        ALOGE("fail to connect to tv service");
        return;
    }

    // make sure tv amlogic is alive
    //if (tv->getStatus() != NO_ERROR) {
    //    jniThrowException(env, "java/lang/RuntimeException", "Tv initialization failed!");
    //    return;
    //}

    jclass clazz = env->GetObjectClass(thiz);
    if (clazz == NULL) {
        ALOGE("Can't find com/droidlogic/app/tv/TvControlManager!");
        return;
    }

    sp<JNITvContext> context = new JNITvContext(env, weak_this, clazz, tv);
    context->incStrong(thiz);
    tv->setListener(context);

    env->SetLongField(thiz, fields.context, (long)context.get());
}

static void com_droidlogic_app_tv_TvControlManager_release(JNIEnv *env, jobject thiz)
{
    // TODO: Change to LOGE
    JNITvContext *context = NULL;
    {
        Mutex::Autolock _l(sLock);
        context = reinterpret_cast<JNITvContext *>(env->GetLongField(thiz, fields.context));

        // Make sure we do not attempt to callback on a deleted Java object.
        env->SetLongField(thiz, fields.context, 0);
    }

    ALOGD("release tv");

    // clean up if release has not been called before
    if (context != NULL) {
        sp<TvServerHidlClient> tv = context->getTv();
        context->release();
        ALOGD("native_release: context=%p tv=%p", context, tv.get());

        // clear callbacks
        if (tv != NULL) {
            //tv->setPreviewCallbackFlags(FRAME_CALLBACK_FLAG_NOOP);
            tv->disconnect();
        }

        // remove context to prevent further Java access
        context->decStrong(thiz);
    }
}

void JNITvContext::notify(const tv_parcel_t &parcel)
{
    /*
    // VM pointer will be NULL if object is released
    Mutex::Autolock _l(mLock);
    if (mTvJObjectWeak == NULL) {
        ALOGW("callback on dead tv object");
        return;
    }
    if (msgType == SUBTITLE_UPDATE_CALLBACK) {
        if (pSubBmp) {
            //SkAutoLockPixels alp(*pSubBmp);
            char *pDst = (char *) pSubBmp->getPixels();
            char *pBuf = (char *) mSubMemBase->pointer();
            for (int i = 0; i < pSubBmp->width() * pSubBmp->height() * 4; i++) {
                pDst[i] = pBuf[i];
            }
            pSubBmp->notifyPixelsChanged();
        }
    }

    JNIEnv *env = AndroidRuntime::getJNIEnv();

    jobject jParcel = createJavaParcelObject(env);
    if (jParcel != NULL) {
        Parcel *nativeParcel = parcelForJavaObject(env, jParcel);
        nativeParcel->write(p.data(), p.dataSize());
        env->CallStaticVoidMethod(mTvJClass, fields.post_event, mTvJObjectWeak, msgType, jParcel);
        env->DeleteLocalRef(jParcel);
    }
    */
}


void JNITvContext::addCallbackBuffer(JNIEnv *env, jbyteArray cbb)
{
    if (cbb != NULL) {
        Mutex::Autolock _l(mLock);
        jbyteArray callbackBuffer = (jbyteArray)env->NewGlobalRef(cbb);
        mCallbackBuffers.push(cbb);
        ALOGD("Adding callback buffer to queue, %d total", mCallbackBuffers.size());
    } else {
        ALOGE("Null byte array!");
    }
}

static struct parcel_offsets_t
{
    jclass clazz;
    jfieldID mNativePtr;
    jmethodID obtain;
    jmethodID recycle;
} gParcelOffsets;

static Parcel* mParcelForJavaObject(JNIEnv* env, jobject obj)
{
    if (obj) {
        Parcel* p = (Parcel*)env->GetLongField(obj, gParcelOffsets.mNativePtr);
        if (p != NULL) {
            return p;
        }

        jclass clazz = env->FindClass("java/lang/IllegalStateException");
        env->ThrowNew(clazz, "Parcel has been finalized!");
        env->DeleteLocalRef(clazz);
    }
    return NULL;
}


static jint com_droidlogic_app_tv_TvControlManager_processCmd(JNIEnv *env, jobject thiz, jobject pObj, jobject rObj)
{
    sp<TvServerHidlClient> tv = get_native_tv(env, thiz, NULL);
    if (tv == 0) return -1;

    Parcel *p = mParcelForJavaObject(env, pObj);
    //jclass clazz;
    //clazz = env->FindClass("android/os/Parcel");
    //LOG_FATAL_IF(clazz == NULL, "Unable to find class android.os.Parcel");


    //jmethodID mConstructor = env->GetMethodID(clazz, "<init>", "(I)V");
    //jobject replayobj = env->NewObject(clazz, mConstructor, 0);
    Parcel *r = mParcelForJavaObject(env, rObj);
    return tv->processCmd(*p, r);
    //if ( != NO_ERROR) {
    //    jniThrowException(env, "java/lang/RuntimeException", "StartTv failed");
    //    return -1;
    // }
    //return 0;
}

static void com_droidlogic_app_tv_TvControlManager_addCallbackBuffer(JNIEnv *env, jobject thiz, jbyteArray bytes)
{
    JNITvContext *context = reinterpret_cast<JNITvContext *>(env->GetLongField(thiz, fields.context));

    ALOGD("addCallbackBuffer");
    if (context != NULL) {
        context->addCallbackBuffer(env, bytes);
    }
}

static void com_droidlogic_app_tv_TvControlManager_reconnect(JNIEnv *env, jobject thiz)
{
    sp<TvServerHidlClient> tv = get_native_tv(env, thiz, NULL);
    if (tv == nullptr) return;

    tv->reconnect();
}

static void com_droidlogic_app_tv_TvControlManager_lock(JNIEnv *env, jobject thiz)
{
    sp<TvServerHidlClient> tv = get_native_tv(env, thiz, NULL);
    if (tv == 0) return;

    ALOGD("lock");

    //if (tv->lock() != NO_ERROR) {
    //    jniThrowException(env, "java/lang/RuntimeException", "lock failed");
    //}
}

static void com_droidlogic_app_tv_TvControlManager_unlock(JNIEnv *env, jobject thiz)
{
    sp<TvServerHidlClient> tv = get_native_tv(env, thiz, NULL);
    if (tv == 0) return;

    ALOGD("unlock");

    //if (tv->unlock() != NO_ERROR) {
    //    jniThrowException(env, "java/lang/RuntimeException", "unlock failed");
    //}
}

static void com_droidlogic_app_tv_TvControlManager_create_subtitle_bitmap(JNIEnv *env, jobject thiz, jobject bmpobj)
{
    ALOGD("create subtitle bmp");
    JNITvContext *context = reinterpret_cast<JNITvContext *>(env->GetLongField(thiz, fields.context));
    sp<TvServerHidlClient> tv = get_native_tv(env, thiz, NULL);
    if (tv == 0) return;

    //get skbitmap
    jclass bmp_clazz;
    jfieldID skbmp_fid;
    jlong hbmp;
    bmp_clazz = env->FindClass("android/graphics/Bitmap");
    skbmp_fid = env->GetFieldID(bmp_clazz, "mNativePtr", "J");
    hbmp = env->GetLongField(bmpobj, skbmp_fid);

    context->pSubBmp = reinterpret_cast<SkBitmap *>(hbmp);
    env->DeleteLocalRef(bmp_clazz);

    //alloc share mem
    sp<MemoryHeapBase> MemHeap = new MemoryHeapBase(context->pSubBmp->width()*context->pSubBmp->height() * 4, 0, "subtitle bmp");
    ALOGD("heap id = %d", MemHeap->getHeapID());
    if (MemHeap->getHeapID() < 0) {
        return;
    }
    //context->pSubBmp->lockPixels();
    context->mSubMemBase = new MemoryBase(MemHeap, 0, context->pSubBmp->width()*context->pSubBmp->height() * 4);


    //send share mem to server
    //tv->createSubtitle(context->mSubMemBase);
    return;
}

//-------------------------------------------------

static JNINativeMethod camMethods[] = {
    {
        "native_setup",
        "(Ljava/lang/Object;)V",
        (void *)com_droidlogic_app_tv_TvControlManager_native_setup
    },
    {
        "native_release",
        "()V",
        (void *)com_droidlogic_app_tv_TvControlManager_release
    },
    {
        "processCmd",
        "(Landroid/os/Parcel;Landroid/os/Parcel;)I",
        (void *)com_droidlogic_app_tv_TvControlManager_processCmd
    },
    /*
    {
        "addCallbackBuffer",
        "([B)V",
        (void *)com_droidlogic_app_tv_TvControlManager_addCallbackBuffer
    },
    */
    {
        "reconnect",
        "()V",
        (void *)com_droidlogic_app_tv_TvControlManager_reconnect
    },
    {
        "lock",
        "()V",
        (void *)com_droidlogic_app_tv_TvControlManager_lock
    },
    {
        "unlock",
        "()V",
        (void *)com_droidlogic_app_tv_TvControlManager_unlock
    },
    {
        "native_create_subtitle_bitmap",
        "(Ljava/lang/Object;)V",
        (void *)com_droidlogic_app_tv_TvControlManager_create_subtitle_bitmap
    },
};

struct field {
    const char *class_name;
    const char *field_name;
    const char *field_type;
    jfieldID   *jfield;
};

static int find_fields(JNIEnv *env, field *fields, int count)
{
    for (int i = 0; i < count; i++) {
        field *f = &fields[i];
        jclass clazz = env->FindClass(f->class_name);
        if (clazz == NULL) {
            ALOGE("Can't find %s", f->class_name);
            return -1;
        }

        jfieldID field = env->GetFieldID(clazz, f->field_name, f->field_type);
        if (field == NULL) {
            ALOGE("Can't find %s.%s", f->class_name, f->field_name);
            return -1;
        }

        *(f->jfield) = field;
    }

    return 0;
}

// Get all the required offsets in java class and register native functions
int register_com_droidlogic_app_tv_TvControlManager(JNIEnv *env)
{
    field fields_to_find[] = {
        { "com/droidlogic/app/tv/TvControlManager", "mNativeContext", "J", &fields.context }
    };

    ALOGD("register_com_droidlogic_app_tv_TvControlManager.");

    if (find_fields(env, fields_to_find, NELEM(fields_to_find)) < 0)
        return -1;

    jclass clazz = env->FindClass("com/droidlogic/app/tv/TvControlManager");
    fields.post_event = env->GetStaticMethodID(clazz, "postEventFromNative", "(Ljava/lang/Object;ILandroid/os/Parcel;)V");
    if (fields.post_event == NULL) {
        ALOGE("Can't find com/droidlogic/app/tv/TvControlManager.postEventFromNative");
        return -1;
    }

    // Register native functions
    if ((env->RegisterNatives(clazz, camMethods, NELEM(camMethods))) <= 0) {
        env->DeleteLocalRef(clazz);
        ALOGE("RegisterNatives failed for '%s'\n", "TvControlManager");
        return -1;
    }

    env->DeleteLocalRef(clazz);
    return 0;
}


jint JNI_OnLoad(JavaVM *vm, void *reserved __unused)
{
    JNIEnv *env = NULL;
    jint result = -1;

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("ERROR: GetEnv failed\n");
        goto bail;
    }

    assert(env != NULL);
    register_com_droidlogic_app_tv_TvControlManager(env);

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;
bail:
    return result;
}

