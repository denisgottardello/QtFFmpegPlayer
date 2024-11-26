#include "qthffmpegplayer.h"

static int ReadPacket(void *opaque, uint8_t *pBuffer, int pBufferSize) {
    return static_cast<QThFFmpegPlayer*>(opaque)->QThFFmpegPlayerReadPacket(pBuffer, pBufferSize);
}

static int TimeoutCallback(void *pQThFFmpegPlayer) {
    if (!((QThFFmpegPlayer*)(pQThFFmpegPlayer))->DoStart) return 1;
    return ((QThFFmpegPlayer*)(pQThFFmpegPlayer))->ElapsedTimer.elapsed()> 10000;
}

QThFFmpegPlayer::QThFFmpegPlayer(QString Path, bool RealTime, FFMPEGSourceTypes FFMPEGSourceType, RTSPTransports RTSPTransport, bool DecodeFrames) {
    this->Path= Path;
    this->RealTime= RealTime;
    this->FFMPEGSourceType= FFMPEGSourceType;
    this->RTSPTransport= RTSPTransport;
    this->DecodeFrames= DecodeFrames;
    QDTLastPacket= QDateTime::currentDateTime();
}

QThFFmpegPlayer::~QThFFmpegPlayer() {
    DoStart= false;
    while (this->isRunning()) msleep(10);
    emit UpdateLog("QThFFmpegPlayer::~QThFFmpegPlayer()");
}

void QThFFmpegPlayer::run() {
    Frames= LastFrame= 0;
    AVFormatContext *pAVFormatContextIn= nullptr;
    switch (FFMPEGSourceType) {
        case FFMPEG_SOURCE_CALLBACK: {
            if (pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnConnectionState(CONNECTION_STATE_CONNECTING);
            emit OnConnectionState(CONNECTION_STATE_CONNECTING);
            #define BUFFERSIZE 4096
            AVIOContext *pAVIOContext;
            uint8_t *pBuffer= static_cast<uint8_t*>(av_malloc(BUFFERSIZE));
            if (pBuffer) {
                pAVIOContext= avio_alloc_context(pBuffer, BUFFERSIZE, 0, this, ReadPacket, nullptr, nullptr);
                if (!pAVIOContext) emit UpdateLog("avio_alloc_context Error!!!");
                else {
                    pAVFormatContextIn= avformat_alloc_context(); {
                        pAVFormatContextIn->pb= pAVIOContext;
                        if (avformat_open_input(&pAVFormatContextIn, nullptr, nullptr, nullptr)< 0) emit UpdateLog("avformat_open_input Error!!!");
                        else {
                            if (pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnConnectionState(CONNECTION_STATE_CONNECTED);
                            emit OnConnectionState(CONNECTION_STATE_CONNECTED);
                            runCommon(pAVFormatContextIn);
                            avformat_close_input(&pAVFormatContextIn);
                        }
                    }{
                        avformat_free_context(pAVFormatContextIn);
                    }
                    if (pAVIOContext) {
                        av_freep(&pAVIOContext->buffer);
                        av_freep(&pAVIOContext);
                    }
                    UpdateLog("ddd");
                }
            }
            if (pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnConnectionState(CONNECTION_STATE_IDLE);
            emit OnConnectionState(CONNECTION_STATE_IDLE);
            break;
        }
        case FFMPEG_SOURCE_STREAM: {
            if (pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnConnectionState(CONNECTION_STATE_CONNECTING);
            emit OnConnectionState(CONNECTION_STATE_CONNECTING);
            avformat_network_init();
            pAVFormatContextIn= avformat_alloc_context(); {
                pAVFormatContextIn->interrupt_callback.callback= TimeoutCallback;
                pAVFormatContextIn->interrupt_callback.opaque= this;
                ElapsedTimer.restart();
                if (avformat_open_input(&pAVFormatContextIn, Path.toStdString().c_str(), nullptr, nullptr)< 0) emit UpdateLog("avformat_open_input Error!!!");
                else {
                    if (pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnConnectionState(CONNECTION_STATE_CONNECTED);
                    emit OnConnectionState(CONNECTION_STATE_CONNECTED);
                    runCommon(pAVFormatContextIn);
                    avformat_close_input(&pAVFormatContextIn);
                }
            }{
                avformat_free_context(pAVFormatContextIn);
            }
            avformat_network_deinit();
            if (pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnConnectionState(CONNECTION_STATE_IDLE);
            emit OnConnectionState(CONNECTION_STATE_IDLE);
            break;
        }
    }
    if (pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnEnd();
    emit OnEnd();
}

void QThFFmpegPlayer::runCommon(AVFormatContext *pAVFormatContextIn) {
    if (avformat_find_stream_info(pAVFormatContextIn, nullptr)< 0) emit UpdateLog("avformat_find_stream_info Error!!!");
    else {
        av_dump_format(pAVFormatContextIn, 0, Path.toStdString().c_str(), 0);
        int StreamAudioIn= -1;
        int StreamVideoIn= -1;
        AVCodecContext *pAVCodecContextAudio= nullptr;
        AVCodecContext *pAVCodecContextVideo= nullptr;
        AVStream *pAVStreamAudio= nullptr;
        AVStream *pAVStreamVideo= nullptr;
        CodecContextOpen(&StreamAudioIn, &pAVCodecContextAudio, pAVFormatContextIn, AVMEDIA_TYPE_AUDIO, &pAVStreamAudio);
        CodecContextOpen(&StreamVideoIn, &pAVCodecContextVideo, pAVFormatContextIn, AVMEDIA_TYPE_VIDEO, &pAVStreamVideo);
        if (pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnConnectionState(CONNECTION_STATE_CONNECTED_DATA_RECEIVED);
        emit OnConnectionState(CONNECTION_STATE_CONNECTED_DATA_RECEIVED);
        if (DoStart && (StreamAudioIn || StreamVideoIn)) {
            AVFrame *pAVFrame= av_frame_alloc();
            if (!pAVFrame) emit UpdateLog("av_frame_alloc Error!!!");
            else {
                AVPacket *pAVPacket= av_packet_alloc();
                if (!pAVPacket) emit UpdateLog("av_packet_alloc Error!!!");
                else {
                    pAVPacket->data= nullptr;
                    pAVPacket->size= 0;
                    SwrContext *pSwrContext= nullptr;
                    if (pAVCodecContextAudio) {
                        pSwrContext= swr_alloc();
                        if (!pSwrContext) emit UpdateLog("swr_alloc Error!!!");
                        else {
                            av_opt_set_chlayout(pSwrContext, "in_chlayout", &pAVCodecContextAudio->ch_layout, 0);
                            av_opt_set_int(pSwrContext, "in_sample_rate", pAVCodecContextAudio->sample_rate, 0);
                            av_opt_set_sample_fmt(pSwrContext, "in_sample_fmt", pAVCodecContextAudio->sample_fmt, 0);
                            av_opt_set_chlayout(pSwrContext, "out_chlayout", &pAVCodecContextAudio->ch_layout, 0);
                            av_opt_set_int(pSwrContext, "out_sample_rate", pAVCodecContextAudio->sample_rate, 0);
                            av_opt_set_sample_fmt(pSwrContext, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
                            if (swr_init(pSwrContext)< 0) emit UpdateLog("swr_init Error!!!");
                            else {
                                if (pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnAudioType(pAVCodecContextAudio->sample_rate, pAVCodecContextAudio->ch_layout.nb_channels);
                                emit OnAudioType(pAVCodecContextAudio->sample_rate, pAVCodecContextAudio->ch_layout.nb_channels);
                            }
                        }
                    }
                    while (DoStart) {
                        ElapsedTimer.restart();
                        int Ret= av_read_frame(pAVFormatContextIn, pAVPacket);
                        if (Ret>= 0) {
                            QDTLastPacket= QDateTime::currentDateTime();
                            if (DecodeFrames) {
                                if (pAVPacket->stream_index== StreamAudioIn) {
                                    Ret= avcodec_send_packet(pAVCodecContextAudio, pAVPacket);
                                    if (Ret< 0) emit UpdateLog("StreamAudioIn Error submitting a packet for decoding.");
                                    while (Ret>= 0) {
                                        Ret= avcodec_receive_frame(pAVCodecContextAudio, pAVFrame);
                                        if (Ret>= 0) {
                                            int64_t SamplesOut= av_rescale_rnd(swr_get_delay(pSwrContext, pAVCodecContextAudio->sample_rate)+ pAVFrame->nb_samples, 44100, pAVCodecContextAudio->sample_rate, AV_ROUND_UP);
                                            uint8_t *BufferOut;
                                            int Ret= av_samples_alloc(static_cast<uint8_t**>(&BufferOut), nullptr, pAVCodecContextAudio->ch_layout.nb_channels, static_cast<int>(SamplesOut), AV_SAMPLE_FMT_S16, 0);
                                            if (Ret< 0) emit UpdateLog("av_samples_alloc Error!!!");
                                            else {
                                                /* convert to destination format */
                                                Ret= swr_convert(pSwrContext, static_cast<uint8_t**>(&BufferOut), SamplesOut, const_cast<const uint8_t**>(pAVFrame->data), pAVFrame->nb_samples);
                                                if (Ret< 0) emit UpdateLog("swr_convert Error!!!");
                                                else {
                                                    Ret= av_samples_get_buffer_size(nullptr, pAVCodecContextAudio->ch_layout.nb_channels, Ret, AV_SAMPLE_FMT_S16, 0);
                                                    if (Ret< 0) emit UpdateLog("av_samples_get_buffer_size Error!!!");
                                                    else {
                                                        if (pAVFrame->pts!= AV_NOPTS_VALUE) {
                                                            if (Speed== 1) {
                                                                if (pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnAudio(BufferOut, Ret);
                                                                emit OnAudio(BufferOut, Ret);
                                                            }
                                                        }
                                                    }
                                                }
                                                av_freep(&BufferOut);
                                            }
                                        }
                                    }
                                } else if (pAVPacket->stream_index== StreamVideoIn) {
                                    Ret= avcodec_send_packet(pAVCodecContextVideo, pAVPacket);
                                    if (Ret< 0) emit UpdateLog("Error submitting a packet for decoding.");
                                    while (Ret>= 0) {
                                        Ret= avcodec_receive_frame(pAVCodecContextVideo, pAVFrame);
                                        if (Ret>= 0) {
                                            AVFrame *pAVFrameRGB= av_frame_alloc(); {
                                                uint8_t *Buffer= static_cast<uint8_t*>(malloc(static_cast<size_t>(av_image_get_buffer_size(AV_PIX_FMT_RGB24, pAVCodecContextVideo->width, pAVCodecContextVideo->height, 1)))); {
                                                    av_image_fill_arrays(pAVFrameRGB->data, pAVFrameRGB->linesize, Buffer, AV_PIX_FMT_RGB24, pAVCodecContextVideo->width, pAVCodecContextVideo->height, 1);
                                                    SwsContext* encoderSwsContext= sws_getContext(pAVCodecContextVideo->width, pAVCodecContextVideo->height, pAVCodecContextVideo->pix_fmt, pAVCodecContextVideo->width, pAVCodecContextVideo->height, AV_PIX_FMT_RGB24, SWS_BICUBIC, nullptr, nullptr, nullptr);{
                                                        sws_scale(encoderSwsContext, pAVFrame->data, pAVFrame->linesize, 0, pAVCodecContextVideo->height, pAVFrameRGB->data, pAVFrameRGB->linesize);
                                                        QImage Image= QImage(pAVCodecContextVideo->width, pAVCodecContextVideo->height, QImage::Format_RGB32);
                                                        AVFrame2QImage(pAVFrameRGB, Image, pAVCodecContextVideo->width, pAVCodecContextVideo->height);
                                                        if (!Image.isNull()) {
                                                            if (DoStart) {
                                                                if (pAVFrame->pts!= AV_NOPTS_VALUE) {
                                                                    if (pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnImage(Image);
                                                                    emit OnImage(Image);
                                                                }
                                                            }
                                                        }
                                                    }{
                                                        sws_freeContext(encoderSwsContext);
                                                    }
                                                }{
                                                    free(Buffer);
                                                }
                                            }{
                                                av_frame_free(&pAVFrameRGB);
                                            }
                                        }
                                        av_frame_unref(pAVFrame);
                                    }
                                }
                                if (pAVPacket->dts== AV_NOPTS_VALUE) {
                                    QString Result= QString("Decoding Time Stamp error, stream index: %1, id: %2, type: %3")
                                    .arg(pAVPacket->stream_index)
                                        .arg(pAVFormatContextIn->streams[pAVPacket->stream_index]->id)
                                        .arg(AVMediaTypeToString(pAVFormatContextIn->streams[pAVPacket->stream_index]->codecpar->codec_type));
                                    emit UpdateLog(Result);
                                } else {
                                    AVRational time_base= pAVFormatContextIn->streams[pAVPacket->stream_index]->time_base;
                                    AVRational time_base_q= {1, static_cast<int>(static_cast<double>(AV_TIME_BASE) / Speed)};
                                    int64_t DecodingTimeStampTemp= av_rescale_q(pAVPacket->dts, time_base, time_base_q);
                                    if (DecodingTimeStampStart< 0) {
                                        DecodingTimeStampStart= DecodingTimeStampTemp;
                                        TimeStart= av_gettime();
                                    } else {
                                        int64_t nowTime= av_gettime()- TimeStart;
                                        if ((DecodingTimeStampTemp- DecodingTimeStampStart)> nowTime) {
                                            av_usleep(DecodingTimeStampTemp- DecodingTimeStampStart- nowTime);
                                        }
                                    }
                                }
                            }
                            av_packet_unref(pAVPacket);
                        } else DoStart= false;
                    }
                    if (pSwrContext) swr_free(&pSwrContext);
                    av_packet_free(&pAVPacket);
                }
                av_frame_free(&pAVFrame);
            }
        } else emit UpdateLog("CodecContextOpen Error!!!");
        avcodec_free_context(&pAVCodecContextAudio);
        avcodec_free_context(&pAVCodecContextVideo);
    }
}

QString QThFFmpegPlayer::AVMediaTypeToString(AVMediaType MediaType) {
    switch(MediaType) {
        case AVMEDIA_TYPE_VIDEO: return "AVMEDIA_TYPE_VIDEO";
        case AVMEDIA_TYPE_AUDIO: return "AVMEDIA_TYPE_AUDIO";
        case AVMEDIA_TYPE_SUBTITLE: return "AVMEDIA_TYPE_SUBTITLE";
        case AVMEDIA_TYPE_DATA: return "AVMEDIA_TYPE_DATA";
        case AVMEDIA_TYPE_ATTACHMENT: return "AVMEDIA_TYPE_ATTACHMENT";
        case AVMEDIA_TYPE_NB: return "AVMEDIA_TYPE_NB";
        case AVMEDIA_TYPE_UNKNOWN:
        default:
            return "AVMEDIA_TYPE_UNKNOWN";
    }
}

void QThFFmpegPlayer::AVFrame2QImage(AVFrame *pAVFrame, QImage &Image, int Width, int Height) {
    for (int y= 0; y< Height; y++) {
        QRgb *scanLine= reinterpret_cast<QRgb*>(Image.scanLine(y));
        for (int x= 0; x< Width; x++) {
            scanLine[x]= qRgb(pAVFrame->data[0][3 * x], pAVFrame->data[0][3 * x+ 1], pAVFrame->data[0][3 * x+ 2]);
        }
        pAVFrame->data[0]+= pAVFrame->linesize[0];
    }
}

int QThFFmpegPlayer::CodecContextOpen(int *pStreamIn, AVCodecContext **pAVCodecContext, AVFormatContext *pAVFormatContextIn, AVMediaType type, AVStream **pAVStream) {
    const AVCodec *pAVCodec= nullptr;
    int stream_index;
    AVDictionary *opts= nullptr;
    int Ret= av_find_best_stream(pAVFormatContextIn, type, -1, -1, nullptr, 0);
    if (Ret< 0) return Ret;
    else {
        stream_index= Ret;
        *pAVStream= pAVFormatContextIn->streams[stream_index];
        /* find decoder for the stream */
        pAVCodec= avcodec_find_decoder((*pAVStream)->codecpar->codec_id);
        if (!pAVCodec) {
            fprintf(stderr, "Failed to find %s codec\n", av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }
        /* Allocate a codec context for the decoder */
        *pAVCodecContext= avcodec_alloc_context3(pAVCodec);
        if (!*pAVCodecContext) {
            fprintf(stderr, "Failed to allocate the %s codec context\n", av_get_media_type_string(type));
            return AVERROR(ENOMEM);
        }
        /* Copy codec parameters from input stream to output codec context */
        if ((Ret= avcodec_parameters_to_context(*pAVCodecContext, (*pAVStream)->codecpar)) < 0) {
            fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n", av_get_media_type_string(type));
            return Ret;
        }
        /* Init the decoders, with or without reference counting */
        av_dict_set(&opts, "refcounted_frames", "0", 0);
        if ((Ret= avcodec_open2(*pAVCodecContext, pAVCodec, &opts)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n", av_get_media_type_string(type));
            return Ret;
        }
        (*pStreamIn)= stream_index;
    }
    return 0;
}

void QThFFmpegPlayer::Stop() {
    DoStart= false;
}

int QThFFmpegPlayer::QThFFmpegPlayerReadPacket(uint8_t *pBuffer, int pBufferSize) {
    int BytesIn= 0;
    emit OnPacketRead(pBuffer, pBufferSize, &BytesIn);
    if (!BytesIn) return AVERROR_EOF;
    else return BytesIn;
}
