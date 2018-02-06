cmake_minimum_required(VERSION 3.4.1)

set(distribution_DIR ${CMAKE_SOURCE_DIR}/../distribution)

include_directories("${distribution_DIR}/${ANDROID_ABI}/include/" "${distribution_DIR}/libsdl/include/")

add_library( libSDL2 SHARED IMPORTED )
set_target_properties ( libSDL2 PROPERTIES IMPORTED_LOCATION
                        ${distribution_DIR}/libsdl/lib/${ANDROID_ABI}/libSDL2.so )

link_libraries("-Wl,--no-warn-shared-textrel")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

add_library( native-lib SHARED
             src/main/cpp/0_hello_world.c
             src/main/cpp/1_render_surface.c
             src/main/cpp/tutorial02.cpp
             src/main/cpp/tutorial03.cpp
             src/main/cpp/tutorial04.cpp
             src/main/cpp/tutorial05.cpp
             src/main/cpp/tutorial06.cpp
             src/main/cpp/native-lib.cpp )

target_include_directories(native-lib PRIVATE
                           ${distribution_DIR}/libsdl/include/)

target_link_libraries( native-lib
                       libSDL2
                       ${distribution_DIR}/${ANDROID_ABI}/lib/libavformat.a
                       ${distribution_DIR}/${ANDROID_ABI}/lib/libavcodec.a
                       ${distribution_DIR}/${ANDROID_ABI}/lib/libswscale.a
                       ${distribution_DIR}/${ANDROID_ABI}/lib/libswresample.a
                       ${distribution_DIR}/${ANDROID_ABI}/lib/libavutil.a
                       ${distribution_DIR}/${ANDROID_ABI}/lib/libavdevice.a
                       ${distribution_DIR}/${ANDROID_ABI}/lib/libavfilter.a
                       m
                       z
                       GLESv1_CM
                       GLESv2
                       android
                       jnigraphics
                       log )