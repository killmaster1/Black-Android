#include "branding.h"
#include <jni.h>
#include <SDL3/SDL_system.h>
namespace branding {
bool WaitForStartupCredits() {
    auto* env=static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    auto activity=static_cast<jobject>(SDL_GetAndroidActivity());
    if (!env || !activity) return false;
    jclass cls=env->GetObjectClass(activity);
    jmethodID method=env->GetMethodID(cls,"waitForStartupCredits","()Z");
    bool approved=method && env->CallBooleanMethod(activity,method)==JNI_TRUE;
    if (env->ExceptionCheck()) { env->ExceptionClear(); approved=false; }
    env->DeleteLocalRef(cls);env->DeleteLocalRef(activity);
    return approved;
}
}
