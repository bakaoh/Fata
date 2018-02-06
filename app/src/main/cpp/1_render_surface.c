#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <stdio.h>

#include <jni.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include "fata.h"

#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "EXT01", __VA_ARGS__))

void render_surface(const char *input, const ANativeWindow *window) {
    AVFormatContext *pFormatCtx = NULL;
    int videoStream;
    unsigned i;
    AVCodecContext *pCodecCtxOrig = NULL;
    AVCodecContext *pCodecCtx = NULL;
    AVCodec *pCodec = NULL;
    AVFrame *pFrame = NULL;
    AVPacket packet;
    int frameFinished;
    struct SwsContext *sws_ctx = NULL;
    uint8_t *yPlane, *uvPlane;
    size_t yPlaneSz, uvPlaneSz;
    int uvPitch;

    // Register all formats and codecs
    av_register_all();

    // Open video file
    if (avformat_open_input(&pFormatCtx, input, NULL, NULL) != 0)
        return; // Couldn't open file

    // Retrieve stream information
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
        return; // Couldn't find stream information

    // Find the first video stream
    videoStream = -1;
    for (i = 0; i < pFormatCtx->nb_streams; i++)
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    if (videoStream == -1)
        return; // Didn't find a video stream

    // Get a pointer to the codec context for the video stream
    pCodecCtxOrig = pFormatCtx->streams[videoStream]->codec;
    // Find the decoder for the video stream
    pCodec = avcodec_find_decoder(pCodecCtxOrig->codec_id);
    if (pCodec == NULL) {
        LOGE("Unsupported codec!\n");
        return; // Codec not found
    }

    // Copy context
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {
        LOGE("Couldn't copy codec context");
        return; // Error copying codec context
    }

    // Open codec
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
        return; // Could not open codec

    // Allocate video frame
    pFrame = av_frame_alloc();

    // initialize SWS context for software scaling
    sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                             pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
                             AV_PIX_FMT_NV21,
                             SWS_BILINEAR,
                             NULL,
                             NULL,
                             NULL);

    // set up YV12 pixel array (12 bits per pixel)
    yPlaneSz = pCodecCtx->width * pCodecCtx->height;
    uvPlaneSz = pCodecCtx->width * pCodecCtx->height / 2;
    yPlane = (uint8_t *) malloc(yPlaneSz);
    uvPlane = (uint8_t *) malloc(uvPlaneSz);
    if (!yPlane || !uvPlane) {
        LOGE("Could not allocate pixel buffers - exiting\n");
        exit(1);
    }

    uvPitch = pCodecCtx->width;
    ANativeWindow_setBuffersGeometry(window, pCodecCtx->width, pCodecCtx->height, 17);

    while (av_read_frame(pFormatCtx, &packet) >= 0) {
        // Is this a packet from the video stream?
        if (packet.stream_index == videoStream) {
            // Decode video frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

            // Did we get a video frame?
            if (frameFinished) {
                AVPicture pict;
                pict.data[0] = yPlane;
                pict.data[1] = uvPlane;
                pict.linesize[0] = pCodecCtx->width;
                pict.linesize[1] = uvPitch;

                // Convert the image into YUV format that SDL uses
                sws_scale(sws_ctx, (uint8_t const *const *) pFrame->data,
                          pFrame->linesize, 0, pCodecCtx->height, pict.data,
                          pict.linesize);

                ANativeWindow_Buffer buffer;
                if (ANativeWindow_lock(window, &buffer, NULL) == 0) {
                    if (buffer.width == buffer.stride) {
                        memcpy(buffer.bits, pict.data[0], yPlaneSz);
                        memcpy(buffer.bits + yPlaneSz, pict.data[1], uvPlaneSz);
                    } else {
                        int height = pCodecCtx->height * 3 / 2;
                        int width = pCodecCtx->width;
                        int i = 0;
                        for (; i < height; ++i)
                            memcpy(buffer.bits + buffer.stride * i, pict.data + width * i, width);
                    }
                    ANativeWindow_unlockAndPost(window);
                }
            }
        }

        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }
    ANativeWindow_release(window);

    // Free the YUV frame
    av_frame_free(&pFrame);
    free(yPlane);
    free(uvPlane);

    // Close the codec
    avcodec_close(pCodecCtx);
    avcodec_close(pCodecCtxOrig);

    // Close the video file
    avformat_close_input(&pFormatCtx);

    return;
}