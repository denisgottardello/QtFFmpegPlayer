#include "qthffmpegplayer.h"

static int ReadPacket(void *opaque, uint8_t *pBuffer, int pBufferSize) {
    return static_cast<QThFFmpegPlayer*>(opaque)->QThFFmpegPlayerReadPacket(pBuffer, pBufferSize);
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
            avformat_network_init();
            AVDictionary *pAVDictionary= nullptr; {
                av_dict_set(&pAVDictionary, "analyzeduration", "90000000000", 0);
                av_dict_set(&pAVDictionary, "probesize", "90000000000", 0);
                #define BUFFERSIZE 4096
                AVIOContext *pAVIOContext;
                uint8_t *pBuffer= static_cast<uint8_t*>(av_malloc(BUFFERSIZE)); {
                    pAVIOContext= avio_alloc_context(pBuffer, BUFFERSIZE, 0, this, ReadPacket, nullptr, nullptr);
                    if (!pAVIOContext) emit UpdateLog("avio_alloc_context Error!!!");
                    else {
                        pAVFormatContextIn= avformat_alloc_context(); {
                            pAVFormatContextIn->pb= pAVIOContext;
                            if (avformat_open_input(&pAVFormatContextIn, nullptr, nullptr, &pAVDictionary)< 0) emit UpdateLog("avformat_open_input Error!!!");
                            else {
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
                    }
                }{
                    //av_free(pBuffer);
                }
            }{
                av_dict_free(&pAVDictionary);
            }
            avformat_network_deinit();
            break;
        }
        case FFMPEG_SOURCE_STREAM: {
            avformat_network_init();
            //AVDictionary *pAVDictionary= nullptr; {
                /*av_dict_set(&pAVDictionary, "stimeout", "5000000", 0); // timeout in microseconds
                switch(RTSPTransport) {
                    case RTSP_TRANSPORT_HTTP: av_dict_set(&pAVDictionary, "rtsp_transport", "http", 0); break;
                    case RTSP_TRANSPORT_TCP: av_dict_set(&pAVDictionary, "rtsp_transport", "tcp", 0); break;
                    case RTSP_TRANSPORT_UDP: av_dict_set(&pAVDictionary, "rtsp_transport", "udp", 0); break;
                    case RTSP_TRANSPORT_UDP_MULTICAST: av_dict_set(&pAVDictionary, "rtsp_transport", "udp_multicast", 0); break;
                }*/
                if (avformat_open_input(&pAVFormatContextIn, Path.toStdString().c_str(), nullptr, nullptr/*&pAVDictionary*/)< 0) emit UpdateLog("avformat_open_input Error!!!");
                else {
                    runCommon(pAVFormatContextIn);
                    avformat_close_input(&pAVFormatContextIn);
                }
            /*}{
                av_dict_free(&pAVDictionary);
            }*/
            avformat_network_deinit();
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
        if (StreamAudioIn || StreamVideoIn) {
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
                    qint64 Begin= av_gettime();
                    qint64 Counter= 0;
                    bool EndOfFile= false;
                    while (DoStart) {
                        bool PleaseWait= true;
                        double Pts= 0;
                        int FrameIndex= -1;
                        for (int count= 0; count< QVFrames.length(); count++) {
                            if (Pts== 0 || Pts> QVFrames.at(count).pts) {
                                Pts= QVFrames.at(count).pts;
                                FrameIndex= count;
                            }
                        }
                        if (FrameIndex> -1) {
                            bool CanGo= false;
                            if (RealTime) {
                                if (QVFrames.at(FrameIndex).pts * 1000000+ Begin< av_gettime()) CanGo= true;
                            } else CanGo= true;
                            if (CanGo) {
                                switch(QVFrames.at(FrameIndex).FrameType) {
                                    case FRAME_TYPE_AUDIO: {
                                        if (pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnAudio((const uchar*)QVFrames.at(FrameIndex).QBABuffer.data(), QVFrames.at(FrameIndex).QBABuffer.length());
                                        emit OnAudio((const uchar*)QVFrames.at(FrameIndex).QBABuffer.data(), QVFrames.at(FrameIndex).QBABuffer.length());
                                        //qDebug() << "audio" << QVFrames.at(FrameIndex).pts;
                                        break;
                                    }
                                    case FRAME_TYPE_VIDEO: {
                                        if (pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnImage(QVFrames[FrameIndex].Image);
                                        emit OnImage(QVFrames.at(FrameIndex).Image);
                                        break;
                                    }
                                }
                                QVFrames.removeAt(FrameIndex);
                                PleaseWait= false;
                            }
                        }
                        if (QVFrames.length()< 128 && !EndOfFile) {
                            int Ret= av_read_frame(pAVFormatContextIn, pAVPacket);
                            if (Ret>= 0) {
                                QDTLastPacket= QDateTime::currentDateTime();
                                PleaseWait= false;
                                if (DecodeFrames) {
                                    if (pAVPacket->stream_index== StreamAudioIn) {
                                        do {
                                            Ret= PacketDecodeAudio(pAVFrame, pAVCodecContextAudio, pAVPacket, pSwrContext, Counter);
                                            if (Ret< 0) break;
                                            pAVPacket->data+= Ret;
                                            pAVPacket->size-= Ret;
                                        } while (pAVPacket->size> 0);
                                    } else if (pAVPacket->stream_index== StreamVideoIn) {
                                        do {
                                            Ret= PacketDecodeVideo(pAVFrame, pAVCodecContextVideo, pAVPacket, pAVStreamVideo);
                                            if (Ret< 0) break;
                                            pAVPacket->data+= Ret;
                                            pAVPacket->size-= Ret;
                                        } while (pAVPacket->size> 0);
                                    }
                                }
                                //av_packet_unref(pAVPacket);
                            } else EndOfFile= true;
                        }
                        if (PleaseWait) av_usleep(100);
                        if (EndOfFile && QVFrames.size()== 0) break;
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

int QThFFmpegPlayer::PacketDecodeAudio(AVFrame *pAVFrame, AVCodecContext *pAVCodecContextAudio, AVPacket *pAVPacket, SwrContext *pSwrContext, qint64 &Counter) {
    if (avcodec_send_packet(pAVCodecContextAudio, pAVPacket)== 0) {
        if (avcodec_receive_frame(pAVCodecContextAudio, pAVFrame)== 0) {
            /* compute destination number of samples */
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
                            Frame frame;
                            frame.FrameType= FRAME_TYPE_AUDIO;
                            for (int count= 0; count< Ret; count++) frame.QBABuffer.append(BufferOut[count]);
                            frame.pts= ((double)Counter) / 2 / pAVCodecContextAudio->sample_rate / pAVCodecContextAudio->ch_layout.nb_channels;
                            Counter+= Ret;
                            QVFrames.append(frame);
                        }
                    }
                }
                av_freep(&BufferOut);
            }
        }
    }
    return pAVPacket->size;
}

int QThFFmpegPlayer::PacketDecodeVideo(AVFrame *pAVFrame, AVCodecContext *pAVCodecContextVideo, AVPacket *pAVPacket, AVStream *pAVStreamVideo) {
    if (avcodec_send_packet(pAVCodecContextVideo, pAVPacket)== 0) {
        if (avcodec_receive_frame(pAVCodecContextVideo, pAVFrame)== 0) {
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
                                    Frame frame;
                                    frame.pts= av_q2d(pAVStreamVideo->time_base) * pAVFrame->pts;
                                    frame.FrameType= FRAME_TYPE_VIDEO;
                                    frame.Image= Image;
                                    //qDebug() << "video" << frame.pts;
                                    QVFrames.append(frame);
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
    }
    return pAVPacket->size;
}

void QThFFmpegPlayer::SpeedSet(int Speed) {
    this->Speed= Speed;
}

void QThFFmpegPlayer::Stop() {
    DoStart= false;
}

int QThFFmpegPlayer::QThFFmpegPlayerReadPacket(uint8_t *pBuffer, int pBufferSize) {
    int BytesIn= 0;
    emit OnPacketRead(pBuffer, pBufferSize, &BytesIn);
    return BytesIn;
}
