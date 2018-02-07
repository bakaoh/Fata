#include <jni.h>
#include "fata.h"

extern "C" {

JNIEXPORT jint JNICALL
Java_com_bakaoh_fata_NativeHelper_helloWorld(
        JNIEnv *env,
        jobject /* this */,
        jstring inputFile,
        jstring outputFolder) {
    const char *input = env->GetStringUTFChars(inputFile, 0);
    const char *output = env->GetStringUTFChars(outputFolder, 0);
    int rs = hello_world(input, output);
    env->ReleaseStringUTFChars(inputFile, input);
    env->ReleaseStringUTFChars(outputFolder, output);
    return rs;
}

JNIEXPORT jstring JNICALL
Java_com_bakaoh_fata_NativeHelper_makingScreencaps(
        JNIEnv *env,
        jobject /* this */,
        jstring inputFile,
        jstring outputFolder) {
    const char *input = env->GetStringUTFChars(inputFile, 0);
    const char *output = env->GetStringUTFChars(outputFolder, 0);
    const char *rs = making_screencaps(input, output);
    env->ReleaseStringUTFChars(inputFile, input);
    env->ReleaseStringUTFChars(outputFolder, output);
    return env->NewStringUTF(rs);
}

JNIEXPORT void JNICALL
Java_com_bakaoh_fata_NativeHelper_renderSurface(
        JNIEnv *env,
        jobject /* this */,
        jstring inputFile,
        jobject javaSurface) {
    const char *input = env->GetStringUTFChars(inputFile, 0);
    const ANativeWindow* window = ANativeWindow_fromSurface(env, javaSurface);
    render_surface(input, window);
    env->ReleaseStringUTFChars(inputFile, input);
}

}

