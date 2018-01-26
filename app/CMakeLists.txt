cmake_minimum_required(VERSION 3.4.1)

include_directories("src/main/jniLibs/${ANDROID_ABI}/include/")

add_library( native-lib SHARED
             src/main/cpp/0_hello_world.c
             src/main/cpp/native-lib.cpp )

link_libraries("-Wl,--no-warn-shared-textrel")

target_link_libraries( native-lib
                       ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/lib/libavformat.a
                       ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/lib/libavcodec.a
                       ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/lib/libswscale.a
                       ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/lib/libswresample.a
                       ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/lib/libavutil.a
                       ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/lib/libavdevice.a
                       ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/lib/libavfilter.a
                       m
                       z
                       log )