#include <jni.h>
#include "fata.h"

extern "C"
JNIEXPORT jint

JNICALL
Java_com_bakaoh_fata_MainActivity_chapter0(
        JNIEnv *env,
        jobject /* this */,
        jstring filename) {
    const char *file = env->GetStringUTFChars(filename, 0);
    return hello_world(file);
}
