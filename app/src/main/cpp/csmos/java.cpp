//
// Created by 33688 on 2024/12/4.
//
#include <jni.h>
#include "main.h"
#include "java.h"



Java::Java(JavaVM* g_java_vm, JNIEnv* env) {
    m_Env = env;
    javaVM = g_java_vm;
    initMasterServers();
}

/**
 * 设置上下文
 * @param thiz SDLActivity
 * @param env
 */
void Java::setupContext(jobject thiz, JNIEnv* env) {
    spdlog::info("setup context...");
    m_Env = env;
    activity = env->NewGlobalRef(thiz);
    jclass sdlClass = env->GetObjectClass(activity);
    if (!sdlClass)
    {
        LOGI("SDL class not found");
        return;
    }
    m_passwordDialog = m_Env->GetMethodID(sdlClass, "passwordDialog", "(Ljava/lang/String;)V");
    m_Env->DeleteLocalRef(sdlClass);
}

JNIEnv *Java::getEnv()
{
    if (!javaVM) {
        LOGI("No java vm");
        return nullptr;
    }
    JNIEnv *env;
    javaVM->GetEnv((void **)&env, JNI_VERSION_1_4);
    return env;
}

jobject Java::getContext() {
    return activity;
}

void Java::showPasswordDialog(const char* password) {
    JNIEnv *env = getEnv();
    spdlog::info("showPasswordDialog native method called");
    if (env == nullptr) {
        return;
    }
    jstring jPassword = env->NewStringUTF(password);
    if (activity == nullptr) {
        spdlog::info("activity not found");
        return;
    }
    env->CallVoidMethod(activity, m_passwordDialog, jPassword);
    env->DeleteLocalRef(jPassword);
}

std::string Java::getFlavor() {
    if (!m_flavor.empty()) {
        return m_flavor;
    }
    if (m_Env == nullptr) {
        return "";
    }
    // 找类
    jclass nativeUtilClass = m_Env->FindClass("com/valvesoftware/NativeUtils");
    if (nativeUtilClass == nullptr) { // 类没有找到
        return "";
    }
    jmethodID getFlavorMethod = m_Env->GetStaticMethodID(nativeUtilClass, "getFlavor", "()Ljava/lang/String;");
    if (getFlavorMethod == nullptr) { // 方法找不到
        return "";
    }
    // 执行方法
    jstring flavor = (jstring) m_Env->CallStaticObjectMethod(nativeUtilClass, getFlavorMethod);
    if (flavor != nullptr) {
        // 获取 C++ 字符串
        const char *flavorCStr = m_Env->GetStringUTFChars(flavor, nullptr);
        std::string flavorStr(flavorCStr);

        // 释放资源
        m_Env->ReleaseStringUTFChars(flavor, flavorCStr);
        m_flavor = flavorStr;
        return flavorStr;
    }

    m_Env->DeleteLocalRef(nativeUtilClass);
}

std::vector<std::string> Java::initMasterServers() {
    std::vector<std::string> serverList;

    jclass nativeUtilClass = m_Env->FindClass("com/valvesoftware/NativeUtils");
    if (nativeUtilClass == nullptr) { // 类没有找到
        return serverList;
    }

    jmethodID getMasterServersMethod = m_Env->GetStaticMethodID(nativeUtilClass, "getMasterServers", "()Ljava/util/List;");
    if (getMasterServersMethod == nullptr) {
        m_Env->DeleteLocalRef(nativeUtilClass);
        return serverList;
    }

    // 调用 getAppUpdateInfo() 获取 AppUpdateInfo 对象
    jobject serverListObj = m_Env->CallStaticObjectMethod(nativeUtilClass, getMasterServersMethod);
    if (serverListObj == nullptr) {
        m_Env->DeleteLocalRef(nativeUtilClass);
        return serverList;
    }

    // 获取 List 的类和 get() 方法
    jclass listClass = m_Env->GetObjectClass(serverListObj);
    jmethodID getMethod = m_Env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
    jmethodID sizeMethod = m_Env->GetMethodID(listClass, "size", "()I");
    if (getMethod == nullptr || sizeMethod == nullptr) {
        m_Env->DeleteLocalRef(nativeUtilClass);
        return serverList;
    }

    // 获取 List 的大小
    jint listSize = m_Env->CallIntMethod(serverListObj, sizeMethod);

    // 获取 List 中的每个元素 (String)
    for (int i = 0; i < listSize; ++i) {
        jstring serverString = (jstring) m_Env->CallObjectMethod(serverListObj, getMethod, i);
        const char* serverCStr = m_Env->GetStringUTFChars(serverString, nullptr);
        serverList.push_back(std::string(serverCStr));
        m_Env->ReleaseStringUTFChars(serverString, serverCStr);
    }

    m_masterServers = serverList;
    // 清理引用
    m_Env->DeleteLocalRef(serverListObj);
    m_Env->DeleteLocalRef(nativeUtilClass);

    return serverList;
}


