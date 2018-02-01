// tutorial04.c
// A pedagogical video player that will stream through every video frame as fast as it can,
// and play audio (out of sync).
//
// Code based on FFplay, Copyright (c) 2003 Fabrice Bellard,
// and a tutorial by Martin Bohme (boehme@inb.uni-luebeckREMOVETHIS.de)

extern "C" {

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avstring.h>

#include <SDL.h>
#include <SDL_thread.h>

#include <stdio.h>
#include <assert.h>
#include <math.h>

#include <jni.h>
#include <android/log.h>

}

#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_AUDIO_FRAME_SIZE 192000

#define MAX_AUDIOQ_SIZE (5 * 16 * 1024)
#define MAX_VIDEOQ_SIZE (5 * 256 * 1024)

#define FF_REFRESH_EVENT (SDL_USEREVENT)
#define FF_QUIT_EVENT (SDL_USEREVENT + 1)

#define VIDEO_PICTURE_QUEUE_SIZE 1

#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "TUT04", __VA_ARGS__))

namespace Tutorial04 {

    typedef struct PacketQueue {
        AVPacketList *first_pkt, *last_pkt;
        int nb_packets;
        int size;
        SDL_mutex *mutex;
        SDL_cond *cond;
    } PacketQueue;

    typedef struct VideoPicture {
        Uint8 *yPlane, *uPlane, *vPlane;
        size_t yPlaneSz, uvPlaneSz;
        int uvPitch;
        int width, height; /* source height & width */
        int allocated;
    } VideoPicture;

    typedef struct VideoState {

        AVFormatContext *pFormatCtx;
        int videoStream, audioStream;
        AVStream *audio_st;
        AVCodecContext *audio_ctx;
        PacketQueue audioq;
        uint8_t audio_buf[(MAX_AUDIO_FRAME_SIZE * 3) / 2];
        unsigned int audio_buf_size;
        unsigned int audio_buf_index;
        AVFrame audio_frame;
        AVPacket audio_pkt;
        uint8_t *audio_pkt_data;
        int audio_pkt_size;
        AVStream *video_st;
        AVCodecContext *video_ctx;
        PacketQueue videoq;
        struct SwsContext *sws_ctx;

        VideoPicture pictq[VIDEO_PICTURE_QUEUE_SIZE];
        int pictq_size, pictq_rindex, pictq_windex;
        SDL_mutex *pictq_mutex;
        SDL_cond *pictq_cond;

        SDL_Thread *parse_tid;
        SDL_Thread *video_tid;

        char filename[1024];
        int quit;
    } VideoState;

    SDL_Texture *texture;
    SDL_Window *screen;
    SDL_Renderer *renderer;
    SDL_mutex *screen_mutex;

    /* Since we only have one decoding thread, the Big Struct
   can be global in case we need it. */
    VideoState *global_video_state;

    void packet_queue_init(PacketQueue *q) {
        memset(q, 0, sizeof(PacketQueue));
        q->mutex = SDL_CreateMutex();
        q->cond = SDL_CreateCond();
    }

    int packet_queue_put(PacketQueue *q, AVPacket *pkt) {

        AVPacketList *pkt1;
        if (av_dup_packet(pkt) < 0) {
            return -1;
        }
        pkt1 = (AVPacketList *) av_malloc(sizeof(AVPacketList));
        if (!pkt1)
            return -1;
        pkt1->pkt = *pkt;
        pkt1->next = NULL;

        SDL_LockMutex(q->mutex);

        if (!q->last_pkt)
            q->first_pkt = pkt1;
        else
            q->last_pkt->next = pkt1;
        q->last_pkt = pkt1;
        q->nb_packets++;
        q->size += pkt1->pkt.size;
        SDL_CondSignal(q->cond);

        SDL_UnlockMutex(q->mutex);
        return 0;
    }

    static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block) {
        AVPacketList *pkt1;
        int ret;

        SDL_LockMutex(q->mutex);

        for (;;) {

            if (global_video_state->quit) {
                ret = -1;
                break;
            }

            pkt1 = q->first_pkt;
            if (pkt1) {
                q->first_pkt = pkt1->next;
                if (!q->first_pkt)
                    q->last_pkt = NULL;
                q->nb_packets--;
                q->size -= pkt1->pkt.size;
                *pkt = pkt1->pkt;
                av_free(pkt1);
                ret = 1;
                break;
            } else if (!block) {
                ret = 0;
                break;
            } else {
                SDL_CondWait(q->cond, q->mutex);
            }
        }
        SDL_UnlockMutex(q->mutex);
        return ret;
    }

    int audio_decode_frame(VideoState *is, uint8_t *audio_buf, int buf_size) {

        int len1, data_size = 0;
        AVPacket *pkt = &is->audio_pkt;

        for (;;) {
            while (is->audio_pkt_size > 0) {
                int got_frame = 0;
                len1 = avcodec_decode_audio4(is->audio_ctx, &is->audio_frame, &got_frame, pkt);
                if (len1 < 0) {
                    /* if error, skip frame */
                    is->audio_pkt_size = 0;
                    break;
                }
                data_size = 0;
                if (got_frame) {
                    data_size = av_samples_get_buffer_size(NULL,
                                                           is->audio_ctx->channels,
                                                           is->audio_frame.nb_samples,
                                                           is->audio_ctx->sample_fmt,
                                                           1);
                    assert(data_size <= buf_size);
                    memcpy(audio_buf, is->audio_frame.data[0], data_size);
                }
                is->audio_pkt_data += len1;
                is->audio_pkt_size -= len1;
                if (data_size <= 0) {
                    /* No data yet, get more frames */
                    continue;
                }
                /* We have data, return it and come back for more later */
                return data_size;
            }
            if (pkt->data)
                av_free_packet(pkt);

            if (is->quit) {
                return -1;
            }
            /* next packet */
            if (packet_queue_get(&is->audioq, pkt, 1) < 0) {
                return -1;
            }
            is->audio_pkt_data = pkt->data;
            is->audio_pkt_size = pkt->size;
        }
    }

    void audio_callback(void *userdata, Uint8 *stream, int len) {

        VideoState *is = (VideoState *) userdata;
        int len1, audio_size;

        while (len > 0) {
            if (is->audio_buf_index >= is->audio_buf_size) {
                /* We have already sent all our data; get more */
                audio_size = audio_decode_frame(is, is->audio_buf, sizeof(is->audio_buf));
                if (audio_size < 0) {
                    /* If error, output silence */
                    is->audio_buf_size = 1024;
                    memset(is->audio_buf, 0, is->audio_buf_size);
                } else {
                    is->audio_buf_size = audio_size;
                }
                is->audio_buf_index = 0;
            }
            len1 = is->audio_buf_size - is->audio_buf_index;
            if (len1 > len)
                len1 = len;
            memcpy(stream, (uint8_t *) is->audio_buf + is->audio_buf_index, len1);
            len -= len1;
            stream += len1;
            is->audio_buf_index += len1;
        }
    }

    static Uint32 sdl_refresh_timer_cb(Uint32 interval, void *opaque) {
        SDL_Event event;
        event.type = FF_REFRESH_EVENT;
        event.user.data1 = opaque;
        SDL_PushEvent(&event);
        return 0; /* 0 means stop timer */
    }

    /* schedule a video refresh in 'delay' ms */
    static void schedule_refresh(VideoState *is, int delay) {
        SDL_AddTimer(delay, sdl_refresh_timer_cb, is);
    }

    void video_display(VideoState *is) {

        SDL_Rect rect;
        VideoPicture *vp;
        float aspect_ratio;
        int w, h, x, y;

        vp = &is->pictq[is->pictq_rindex];
        if (is->video_ctx->sample_aspect_ratio.num == 0) {
            aspect_ratio = 0;
        } else {
            aspect_ratio = av_q2d(is->video_ctx->sample_aspect_ratio) *
                           is->video_ctx->width / is->video_ctx->height;
        }
        if (aspect_ratio <= 0.0) {
            aspect_ratio = (float) is->video_ctx->width /
                           (float) is->video_ctx->height;
        }
        h = 1080;
        w = ((int) rint(h * aspect_ratio)) & -3;
        if (w > 1920) {
            w = 1920;
            h = ((int) rint(w / aspect_ratio)) & -3;
        }
        x = (1920 - w) / 2;
        y = (1080 - h) / 2;

        SDL_UpdateYUVTexture(
                texture,
                NULL,
                vp->yPlane,
                is->video_ctx->width,
                vp->uPlane,
                vp->uvPitch,
                vp->vPlane,
                vp->uvPitch
        );

        SDL_RenderClear(renderer);
        rect.x = x;
        rect.y = y;
        rect.w = w;
        rect.h = h;
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_RenderPresent(renderer);
    }

    void video_refresh_timer(void *userdata) {

        VideoState *is = (VideoState *) userdata;

        if (is->video_st) {
            if (is->pictq_size == 0) {
                schedule_refresh(is, 1);
            } else {
                /* Now, normally here goes a ton of code
               about timing, etc. we're just going to
               guess at a delay for now. You can
               increase and decrease this value and hard code
               the timing - but I don't suggest that ;)
               We'll learn how to do it for real later.
                */
                schedule_refresh(is, 40);

                /* show the picture! */
                video_display(is);

                /* update queue for next picture! */
                if (++is->pictq_rindex == VIDEO_PICTURE_QUEUE_SIZE) {
                    is->pictq_rindex = 0;
                }
                SDL_LockMutex(is->pictq_mutex);
                is->pictq_size--;
                SDL_CondSignal(is->pictq_cond);
                SDL_UnlockMutex(is->pictq_mutex);
            }
        } else {
            schedule_refresh(is, 100);
        }
    }

    void alloc_picture(void *userdata) {

        VideoState *is = (VideoState *) userdata;
        VideoPicture *vp;

        vp = &is->pictq[is->pictq_windex];
        if (vp->allocated) {
            // we already have one make another, bigger/smaller
            free(vp->yPlane);
            free(vp->uPlane);
            free(vp->vPlane);
        }
        // Allocate a place to put our YUV image on that screen
        vp->width = is->video_ctx->width;
        vp->height = is->video_ctx->height;
        vp->allocated = 1;

        // set up YV12 pixel array (12 bits per pixel)
        vp->yPlaneSz = is->video_ctx->width * is->video_ctx->height;
        vp->uvPlaneSz = is->video_ctx->width * is->video_ctx->height / 4;
        vp->yPlane = (Uint8 *) malloc(vp->yPlaneSz);
        vp->uPlane = (Uint8 *) malloc(vp->uvPlaneSz);
        vp->vPlane = (Uint8 *) malloc(vp->uvPlaneSz);
        if (!vp->yPlane || !vp->uPlane || !vp->vPlane) {
            LOGE("Could not allocate pixel buffers - exiting\n");
            exit(1);
        }

        vp->uvPitch = is->video_ctx->width / 2;
    }

    int queue_picture(VideoState *is, AVFrame *pFrame) {

        VideoPicture *vp;
        AVPicture pict;

        /* wait until we have space for a new pic */
        SDL_LockMutex(is->pictq_mutex);
        while (is->pictq_size >= VIDEO_PICTURE_QUEUE_SIZE &&
               !is->quit) {
            SDL_CondWait(is->pictq_cond, is->pictq_mutex);
        }
        SDL_UnlockMutex(is->pictq_mutex);

        if (is->quit)
            return -1;

        // windex is set to 0 initially
        vp = &is->pictq[is->pictq_windex];

        /* allocate or resize the buffer! */
        if (vp->width != is->video_ctx->width || vp->height != is->video_ctx->height) {
            alloc_picture(is);
            if (is->quit) {
                return -1;
            }
        }

        /* point pict at the queue */
        pict.data[0] = vp->yPlane;
        pict.data[1] = vp->uPlane;
        pict.data[2] = vp->vPlane;
        pict.linesize[0] = is->video_ctx->width;
        pict.linesize[1] = vp->uvPitch;
        pict.linesize[2] = vp->uvPitch;

        // Convert the image into YUV format that SDL uses
        sws_scale(is->sws_ctx, (uint8_t const *const *) pFrame->data,
                  pFrame->linesize, 0, is->video_ctx->height,
                  pict.data, pict.linesize);

        /* now we inform our display thread that we have a pic ready */
        if (++is->pictq_windex == VIDEO_PICTURE_QUEUE_SIZE) {
            is->pictq_windex = 0;
        }
        SDL_LockMutex(is->pictq_mutex);
        is->pictq_size++;
        SDL_UnlockMutex(is->pictq_mutex);

        return 0;
    }

    int video_thread(void *arg) {
        VideoState *is = (VideoState *) arg;
        AVPacket pkt1, *packet = &pkt1;
        int frameFinished;
        AVFrame *pFrame;

        pFrame = av_frame_alloc();

        for (;;) {
            if (packet_queue_get(&is->videoq, packet, 1) < 0) {
                // means we quit getting packets
                break;
            }
            // Decode video frame
            avcodec_decode_video2(is->video_ctx, pFrame, &frameFinished, packet);
            // Did we get a video frame?
            if (frameFinished) {
                if (queue_picture(is, pFrame) < 0) {
                    break;
                }
            }
            av_free_packet(packet);
        }
        av_frame_free(&pFrame);
        return 0;
    }

    int stream_component_open(VideoState *is, int stream_index) {

        AVFormatContext *pFormatCtx = is->pFormatCtx;
        AVCodecContext *codecCtx = NULL;
        AVCodec *codec = NULL;
        SDL_AudioSpec wanted_spec, spec;

        if (stream_index < 0 || stream_index >= pFormatCtx->nb_streams) {
            return -1;
        }

        codec = avcodec_find_decoder(pFormatCtx->streams[stream_index]->codec->codec_id);
        if (!codec) {
            LOGE("Unsupported codec!\n");
            return -1;
        }

        codecCtx = avcodec_alloc_context3(codec);
        if (avcodec_copy_context(codecCtx, pFormatCtx->streams[stream_index]->codec) != 0) {
            LOGE("Couldn't copy codec context");
            return -1; // Error copying codec context
        }

        if (codecCtx->codec_type == AVMEDIA_TYPE_AUDIO) {
            // Set audio settings from codec info
            wanted_spec.freq = codecCtx->sample_rate;
            wanted_spec.format = AUDIO_U8;
            wanted_spec.channels = codecCtx->channels;
            wanted_spec.silence = 0;
            wanted_spec.samples = 4096;
            wanted_spec.callback = audio_callback;
            wanted_spec.userdata = is;

            if (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
                LOGE("SDL_OpenAudio: %s\n", SDL_GetError());
                return -1;
            }
        }
        if (avcodec_open2(codecCtx, codec, NULL) < 0) {
            LOGE("Unsupported codec!\n");
            return -1;
        }

        switch (codecCtx->codec_type) {
            case AVMEDIA_TYPE_AUDIO:
                is->audioStream = stream_index;
                is->audio_st = pFormatCtx->streams[stream_index];
                is->audio_ctx = codecCtx;
                is->audio_buf_size = 0;
                is->audio_buf_index = 0;
                memset(&is->audio_pkt, 0, sizeof(is->audio_pkt));
                packet_queue_init(&is->audioq);
                SDL_PauseAudio(0);
                break;
            case AVMEDIA_TYPE_VIDEO:
                is->videoStream = stream_index;
                is->video_st = pFormatCtx->streams[stream_index];
                is->video_ctx = codecCtx;
                packet_queue_init(&is->videoq);
                is->video_tid = SDL_CreateThread(video_thread, "video_thread", is);
                is->sws_ctx = sws_getContext(is->video_ctx->width, is->video_ctx->height,
                                             is->video_ctx->pix_fmt, is->video_ctx->width,
                                             is->video_ctx->height, AV_PIX_FMT_YUV420P,
                                             SWS_BILINEAR, NULL, NULL, NULL
                );
                break;
            default:
                break;
        }
        return 0;
    }

    int decode_thread(void *arg) {

        VideoState *is = (VideoState *) arg;
        AVFormatContext *pFormatCtx;
        AVPacket pkt1, *packet = &pkt1;

        int video_index = -1;
        int audio_index = -1;
        int i;

        is->videoStream = -1;
        is->audioStream = -1;

        global_video_state = is;

        // Open video file
        if (avformat_open_input(&pFormatCtx, is->filename, NULL, NULL) != 0)
            return -1; // Couldn't open file

        is->pFormatCtx = pFormatCtx;

        // Retrieve stream information
        if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
            return -1; // Couldn't find stream information

        // Dump information about file onto standard error
        av_dump_format(pFormatCtx, 0, is->filename, 0);

        // Find the first video stream

        for (i = 0; i < pFormatCtx->nb_streams; i++) {
            if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO &&
                video_index < 0) {
                video_index = i;
            }
            if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO &&
                audio_index < 0) {
                audio_index = i;
            }
        }
        if (audio_index >= 0) {
            stream_component_open(is, audio_index);
        }
        if (video_index >= 0) {
            stream_component_open(is, video_index);
        }

        if (is->videoStream < 0 || is->audioStream < 0) {
            LOGE("%s: could not open codecs\n", is->filename);
            goto fail;
        }

        // main decode loop

        for (;;) {
            if (is->quit) {
                break;
            }
            // seek stuff goes here
            if (is->audioq.size > MAX_AUDIOQ_SIZE ||
                is->videoq.size > MAX_VIDEOQ_SIZE) {
                SDL_Delay(10);
                continue;
            }
            if (av_read_frame(is->pFormatCtx, packet) < 0) {
                if (is->pFormatCtx->pb->error == 0) {
                    SDL_Delay(100); /* no error; wait for user input */
                    continue;
                } else {
                    break;
                }
            }
            // Is this a packet from the video stream?
            if (packet->stream_index == is->videoStream) {
                packet_queue_put(&is->videoq, packet);
            } else if (packet->stream_index == is->audioStream) {
                packet_queue_put(&is->audioq, packet);
            } else {
                av_free_packet(packet);
            }
        }
        /* all done - wait for it */
        while (!is->quit) {
            SDL_Delay(100);
        }

        fail:
        if (1) {
            SDL_Event event;
            event.type = FF_QUIT_EVENT;
            event.user.data1 = is;
            SDL_PushEvent(&event);
        }
        return 0;
    }

    int run(int argc, char *argv[]) {

        SDL_Event event;
        VideoState *is;

        is = (VideoState *) av_mallocz(sizeof(VideoState));

        if (argc < 2) {
            LOGE("Usage: test <file>\n");
            exit(1);
        }
        // Register all formats and codecs
        av_register_all();

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
            LOGE("Could not initialize SDL - %s\n", SDL_GetError());
            exit(1);
        }

        // Make a screen to put our video
        screen = SDL_CreateWindow(
                "FFmpeg Tutorial",
                SDL_WINDOWPOS_UNDEFINED,
                SDL_WINDOWPOS_UNDEFINED,
                1920, 1080,
                SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN
        );
        if (!screen) {
            LOGE("SDL: could not create window - exiting\n");
            exit(1);
        }

        renderer = SDL_CreateRenderer(screen, -1,
                                      SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) {
            LOGE("SDL: could not create renderer - exiting\n");
            exit(1);
        }

        texture = SDL_CreateTexture(
                renderer,
                SDL_PIXELFORMAT_YV12,
                SDL_TEXTUREACCESS_STREAMING,
                1920, 1080
        );
        if (!texture) {
            fprintf(stderr, "SDL: could not create texture - exiting\n");
            exit(1);
        }

        screen_mutex = SDL_CreateMutex();

        av_strlcpy(is->filename, argv[1], sizeof(is->filename));

        is->pictq_mutex = SDL_CreateMutex();
        is->pictq_cond = SDL_CreateCond();

        schedule_refresh(is, 40);

        is->parse_tid = SDL_CreateThread(decode_thread, "decode_thread", is);
        if (!is->parse_tid) {
            av_free(is);
            return -1;
        }
        for (;;) {

            SDL_WaitEvent(&event);
            switch (event.type) {
                case FF_QUIT_EVENT:
                case SDL_QUIT:
                    SDL_DestroyTexture(texture);
                    SDL_DestroyRenderer(renderer);
                    SDL_DestroyWindow(screen);
                    is->quit = 1;
                    SDL_Quit();
                    return 0;
                case FF_REFRESH_EVENT:
                    video_refresh_timer(event.user.data1);
                    break;
                default:
                    break;
            }
        }
    }
}

extern C_LINKAGE DECLSPEC int tutorial04(int argc, char *argv[]) {
    return Tutorial04::run(argc, argv);
}