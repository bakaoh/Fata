//
// Created by taitt on 26/01/2018.
//
#ifdef __cplusplus
extern "C" {
#endif

#ifndef FATA_FATA_H
#define FATA_FATA_H

#endif //FATA_FATA_H

#include <jni.h>
#include <android/log.h>
#include <android/native_window_jni.h>

#define logging(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "fata", __VA_ARGS__))

int hello_world(const char *, const char *);

const char *making_screencaps(const char *, const char *);

void render_surface(const char *, const ANativeWindow *);

#ifdef __cplusplus
}
#endif
