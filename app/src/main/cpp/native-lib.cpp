#include <jni.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>

static jobject fake_class = NULL;

static constexpr uint32_t kAccPublic =       0x0001;  // class, field, method, ic
static constexpr uint32_t kAccPrivate =      0x0002;  // field, method, ic
static constexpr uint32_t kAccNative =       0x0100;  // method

void hookJavaMethod(JNIEnv *env, const char *className,
                    const char *funcName,
                    const char *funcSig,
                    void *handleFunc,
                    jclass fake_class,
                    bool isStatic,
                    int apiLevel) {
    jclass clazz = env->FindClass(className);
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    jmethodID methodID = NULL;
    if (isStatic) {
        methodID = env->GetStaticMethodID(clazz, funcName, funcSig);
    } else {
        methodID = env->GetMethodID(clazz, funcName, funcSig);
    }

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    void *artMethod = reinterpret_cast<void *>(methodID);

    jmethodID jmethodID1 = env->GetMethodID(fake_class, funcName, funcSig);

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    void *copyArtMethod = reinterpret_cast<void *>(jmethodID1);

    if (apiLevel == 21) {
        const int32_t entry_offset = 40;
        void *entry_raw_addr = (void *) (reinterpret_cast<const char *>(copyArtMethod) + entry_offset);
        void **entry_addr = reinterpret_cast<void **>(entry_raw_addr);
        void *entry = *entry_addr;

        size_t len = 72;
        memcpy(copyArtMethod, artMethod, len);

        uint8_t *flags_raw_addr = reinterpret_cast<uint8_t*>(copyArtMethod);
        int32_t *flags_addr = reinterpret_cast<int32_t*>(flags_raw_addr + 56);
        *flags_addr = (*flags_addr | kAccPrivate);

        flags_raw_addr = reinterpret_cast<uint8_t*>(artMethod);
        flags_addr = reinterpret_cast<int32_t*>(flags_raw_addr + 56);
        *flags_addr = (*flags_addr | kAccNative);

        entry_raw_addr = (void *) (reinterpret_cast<const char *>(artMethod) + entry_offset);
        entry_addr = reinterpret_cast<void **>(entry_raw_addr);
        *entry_addr = entry;

        int offset = 32;
        void *raw_addr = (void *) (reinterpret_cast<const char *>(artMethod) + offset);
        void **addr = reinterpret_cast<void **>(raw_addr);
        *addr = handleFunc;
    } else if (apiLevel == 22) {
        const int32_t entry_offset = 44;
        void *entry_raw_addr = (void *) (reinterpret_cast<const char *>(copyArtMethod) + entry_offset);
        void **entry_addr = reinterpret_cast<void **>(entry_raw_addr);
        void *entry = *entry_addr;

        size_t len = 48;
        memcpy(copyArtMethod, artMethod, len);

        uint8_t *flags_raw_addr = reinterpret_cast<uint8_t*>(copyArtMethod);
        int32_t *flags_addr = reinterpret_cast<int32_t*>(flags_raw_addr + 20);
        *flags_addr = (*flags_addr | kAccPrivate);

        flags_raw_addr = reinterpret_cast<uint8_t*>(artMethod);
        flags_addr = reinterpret_cast<int32_t*>(flags_raw_addr + 20);
        *flags_addr = (*flags_addr | kAccNative);

        entry_raw_addr = (void *) (reinterpret_cast<const char *>(artMethod) + entry_offset);
        entry_addr = reinterpret_cast<void **>(entry_raw_addr);
        *entry_addr = entry;

        int offset = 40;
        void *raw_addr = (void *) (reinterpret_cast<const char *>(artMethod) + offset);
        void **addr = reinterpret_cast<void **>(raw_addr);
        *addr = handleFunc;
    } else if (apiLevel == 23) {
        const int32_t entry_offset = 36;
        void *entry_raw_addr = (void *) (reinterpret_cast<const char *>(copyArtMethod) + entry_offset);
        void **entry_addr = reinterpret_cast<void **>(entry_raw_addr);
        void *entry = *entry_addr;

        size_t len = 40;
        memcpy(copyArtMethod, artMethod, len);

        uint8_t *flags_raw_addr = reinterpret_cast<uint8_t*>(copyArtMethod);
        int32_t *flags_addr = reinterpret_cast<int32_t*>(flags_raw_addr + 12);
        *flags_addr = (*flags_addr | kAccPrivate);

        flags_raw_addr = reinterpret_cast<uint8_t*>(artMethod);
        flags_addr = reinterpret_cast<int32_t*>(flags_raw_addr + 12);
        *flags_addr = (*flags_addr | kAccNative);

        entry_raw_addr = (void *) (reinterpret_cast<const char *>(artMethod) + entry_offset);
        entry_addr = reinterpret_cast<void **>(entry_raw_addr);
        *entry_addr = entry;

        int offset = 32;
        void *raw_addr = (void *) (reinterpret_cast<const char *>(artMethod) + offset);
        void **addr = reinterpret_cast<void **>(raw_addr);
        *addr = handleFunc;
    } else  {
        // TODO support it later.
    }
}

typedef int (SystemPropertyGetFunction)(const char*, char*);
static int getApiLevel(void) {
    int ret = 21; // default

    void* handle = dlopen("libc.so", RTLD_LAZY);
    if (!handle) {
        return ret;
    }

    SystemPropertyGetFunction* real_system_property_get = reinterpret_cast<SystemPropertyGetFunction*>(dlsym(handle, "__system_property_get"));
    if (!real_system_property_get) {
        return ret;
    }

    char value[8] = {0};
    real_system_property_get("ro.build.version.sdk", value);

    char* end = NULL;
    long v = strtol(value, &end, 10);
    if (end && *end == '\0' && v == (int)v) {
        ret = (int)v;
    }

    return ret;

}

jobject getPackageInfo(JNIEnv *env, jobject thiz, jstring packageName, jint flags) {
    jmethodID getPackageInfo_methodID = env->GetMethodID((jclass)fake_class, "getPackageInfo", "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");
    jobject obj = env->CallObjectMethod(thiz, getPackageInfo_methodID, packageName, flags);
    jclass pkgInfoClass = env->GetObjectClass(obj);
    jfieldID jfieldID1 = env->GetFieldID(pkgInfoClass, "packageName", "Ljava/lang/String;");
    env->SetObjectField(obj, jfieldID1, env->NewStringUTF("it's been hooked!"));
    return obj;
}

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }

    fake_class = env->NewGlobalRef(env->FindClass("com/tian/hook/Fake"));
    hookJavaMethod(env, "android/app/ApplicationPackageManager", "getPackageInfo", "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;",
                   (void*)getPackageInfo, (jclass)fake_class, false, getApiLevel());
    return JNI_VERSION_1_6;
}
