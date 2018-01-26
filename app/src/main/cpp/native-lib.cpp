#include <jni.h>
#include "fata.h"

extern "C" {

JNIEXPORT jint JNICALL
Java_com_bakaoh_fata_MainActivity_chapter0(
        JNIEnv *env,
        jobject /* this */,
        jstring inputFile,
        jstring outputFolder) {
    const char *input = env->GetStringUTFChars(inputFile, 0);
    const char *output = env->GetStringUTFChars(outputFolder, 0);
    return hello_world(input, output);
}

}

