#include "qthffmpegplayer.h"

static int ReadPacket(void *opaque, uint8_t *pBuffer, int pBufferSize) {
    return static_cast<QThFFmpegPlayer*>(opaque)->QThFFmpegPlayerReadPacket(pBuffer, pBufferSize);
}

static int TimeoutCallback(void *pQThFFmpegPlayer) {
    if (!((QThFFmpegPlayer*)(pQThFFmpegPlayer))->DoStart) return 1;
    return ((QThFFmpegPlayer*)(pQThFFmpegPlayer))->ElapsedTimer.elapsed()> 10000;
}

QThFFmpegPlayer::QThFFmpegPlayer(QString Path, bool RealTime, FFMPEGSourceTypes FFMPEGSourceType, bool AudioSupport, QString FileName, QString Resolution, QString FormatName, QString RTSPTransport) {
    this->Path= Path;
    this->RealTime= RealTime;
    this->FFMPEGSourceType= FFMPEGSourceType;
    this->AudioSupport= AudioSupport;
    this->FileName= FileName;
    this->Resolution= Resolution;
    this->FormatName= FormatName;
    this->RTSPTransport= RTSPTransport;
    QDTLastPacket= QDateTime::currentDateTime();
}

QThFFmpegPlayer::~QThFFmpegPlayer() {
    DoStart= false;
    while (this->isRunning()) msleep(10);
    emit UpdateLog("QThFFmpegPlayer::~QThFFmpegPlayer()");
}

void QThFFmpegPlayer::run() {
    AVFormatContext *pAVFormatContextIn= nullptr;
    switch (FFMPEGSourceType) {
        case FFMPEG_SOURCE_CALLBACK: {
            if (DoStart && pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnConnectionState(CONNECTION_STATE_CONNECTING);
            if (DoStart) emit OnConnectionState(CONNECTION_STATE_CONNECTING);
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
                            if (DoStart && pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnConnectionState(CONNECTION_STATE_CONNECTED);
                            if (DoStart) emit OnConnectionState(CONNECTION_STATE_CONNECTED);
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
            }
            if (DoStart && pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnConnectionState(CONNECTION_STATE_IDLE);
            if (DoStart) emit OnConnectionState(CONNECTION_STATE_IDLE);
            break;
        }
        case FFMPEG_SOURCE_DEVICE: {
            if (DoStart && pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnConnectionState(CONNECTION_STATE_CONNECTING);
            if (DoStart) emit OnConnectionState(CONNECTION_STATE_CONNECTING);
            avdevice_register_all();
            #ifdef Q_OS_LINUX
                const AVInputFormat *pAVInputFormat= av_find_input_format("video4linux2");
            #endif
            #ifdef Q_OS_WIN
                const AVInputFormat *pAVInputFormat= av_find_input_format("dshow");
            #endif
            #ifdef Q_OS_MAC
                const AVInputFormat *pAVInputFormat= av_find_input_format("avfoundation");
            #endif
            if (pAVInputFormat) {
                AVDictionary *pAVDictionary= nullptr;
                av_dict_set(&pAVDictionary, "video_size", Resolution.toStdString().c_str(), 0);
                av_dict_set(&pAVDictionary, "framerate", "30", 0);
                #ifdef Q_OS_UNIX
                    int Result= avformat_open_input(&pAVFormatContextIn, Path.toStdString().c_str(), pAVInputFormat, &pAVDictionary);
                #endif
                #ifdef Q_OS_WIN
                    QString Temp= "video="+ Path;
                    int Result= avformat_open_input(&pAVFormatContextIn, Temp.toStdString().c_str(), pAVInputFormat, &pAVDictionary);
                #endif
                if (Result!= 0) {
                    char Buffer[1024];
                    av_make_error_string(Buffer, sizeof(Buffer), Result);
                    if (DoStart) emit UpdateLog("avformat_open_input Error!!! "+ QString(Buffer));
                } else {
                    if (DoStart) emit OnConnectionState(CONNECTION_STATE_CONNECTED);
                    runCommon(pAVFormatContextIn);
                    avformat_close_input(&pAVFormatContextIn);
                }
                av_dict_free(&pAVDictionary);
            }
            if (DoStart) emit OnConnectionState(CONNECTION_STATE_IDLE);
            break;
        }
        case FFMPEG_SOURCE_STREAM: {
            if (DoStart && pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnConnectionState(CONNECTION_STATE_CONNECTING);
            if (DoStart) emit OnConnectionState(CONNECTION_STATE_CONNECTING);
            avformat_network_init();
            pAVFormatContextIn= avformat_alloc_context(); {
                pAVFormatContextIn->interrupt_callback.callback= TimeoutCallback;
                pAVFormatContextIn->interrupt_callback.opaque= this;
                ElapsedTimer.restart();
                AVDictionary *pAVDictionary= nullptr;
                av_dict_set(&pAVDictionary, "rtsp_transport", RTSPTransport.toStdString().c_str(), 0);
                if (avformat_open_input(&pAVFormatContextIn, Path.toStdString().c_str(), nullptr, &pAVDictionary)< 0) emit UpdateLog("avformat_open_input Error!!!");
                else {
                    if (DoStart && pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnConnectionState(CONNECTION_STATE_CONNECTED);
                    if (DoStart) emit OnConnectionState(CONNECTION_STATE_CONNECTED);
                    runCommon(pAVFormatContextIn);
                    avformat_close_input(&pAVFormatContextIn);
                }
            }{
                avformat_free_context(pAVFormatContextIn);
            }
            avformat_network_deinit();
            if (DoStart && pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnConnectionState(CONNECTION_STATE_IDLE);
            if (DoStart) emit OnConnectionState(CONNECTION_STATE_IDLE);
            break;
        }
    }
    if (DoStart && pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnEnd();
    if (DoStart) emit OnEnd();
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
            AVFormatContext *pAVFormatContextOut= nullptr;
            if (FileName.length()) {
                FileOpen(pAVFormatContextIn, &pAVFormatContextOut);
            }
            AVFrame *pAVFrame= av_frame_alloc();
            if (!pAVFrame) emit UpdateLog("av_frame_alloc Error!!!");
            else {
                #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                    QAudioOutput *pQAudioOutput= nullptr;
                #else
                    QAudioSink *pQAudioSink= nullptr;
                #endif
                QByteArray QBAAudioBufferOut;
                QIODevice *pQIODevice= nullptr;
                AVPacket *pAVPacket= av_packet_alloc();
                if (!pAVPacket) emit UpdateLog("av_packet_alloc Error!!!");
                else {
                    pAVPacket->data= nullptr;
                    pAVPacket->size= 0;
                    if (pAVCodecContextAudio) {
                        if (AudioSupport) {
                            #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                                QAudioFormat AudioFormat;
                                AudioFormat.setByteOrder(QAudioFormat::LittleEndian);
                                AudioFormat.setChannelCount(pAVCodecContextAudio->ch_layout.nb_channels);
                                AudioFormat.setCodec("audio/pcm");
                                AudioFormat.setSampleRate(pAVCodecContextAudio->sample_rate);
                                AudioFormat.setSampleSize(16);
                                AudioFormat.setSampleType(QAudioFormat::SignedInt);
                                QAudioDeviceInfo AudioDeviceInfo= QAudioDeviceInfo::defaultOutputDevice();
                                pQAudioOutput= new QAudioOutput(AudioDeviceInfo, AudioFormat, nullptr);
                                pQAudioOutput->moveToThread(this);
                                pQAudioOutput->setVolume(Volume);
                                pQIODevice= pQAudioOutput->start();
                            #else
                                QAudioFormat AudioFormat;
                                AudioFormat.setChannelCount(2);
                                AudioFormat.setSampleFormat(QAudioFormat::Int16);
                                AudioFormat.setSampleRate(44100);
                                QAudioDevice pQAudioDevice= QMediaDevices::defaultAudioOutput();
                                if (!pQAudioDevice.isFormatSupported(AudioFormat)) {
                                    AudioFormat= pQAudioDevice.preferredFormat();
                                }
                                pQAudioSink= new QAudioSink(pQAudioDevice, AudioFormat);
                                pQIODevice= pQAudioSink->start();
                                pQAudioSink->setVolume(Volume);
                            #endif
                        }
                        if (DoStart && pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnAudioType(pAVCodecContextAudio->sample_rate, pAVCodecContextAudio->ch_layout.nb_channels);
                        if (DoStart) emit OnAudioType(pAVCodecContextAudio->sample_rate, pAVCodecContextAudio->ch_layout.nb_channels);
                        printf("sample_rate in: %d, sample_fmt in: %d, channels: %d, pAVFrame->nb_samples: %d\n", pAVCodecContextAudio->sample_rate, pAVCodecContextAudio->sample_fmt, pAVCodecContextAudio->ch_layout.nb_channels, pAVFrame->nb_samples);
                        fflush(stdout);
                    }
                    int64_t PreviousDts[1024];
                    int64_t PreviousPts[1024];
                    for (int count= 0; count< 1024; count++) {
                        PreviousDts[count]= -1;
                        PreviousPts[count]= -1;
                    }
                    bool IsPaused= false;
                    int64_t PauseStart= 0;
                    while (DoStart) {
                        while (DoStart && Pause) {
                            if (!IsPaused) {
                                IsPaused= true;
                                PauseStart= av_gettime();
                            }
                            av_usleep(1 * 1000 * 100);
                        }
                        ElapsedTimer.restart();
                        int Ret= av_read_frame(pAVFormatContextIn, pAVPacket);
                        if (Ret>= 0) {
                            QDTLastPacket= QDateTime::currentDateTime();
                            if (StreamVideoIn== -1 || ((pAVPacket->flags & AV_PKT_FLAG_KEY) && pAVPacket->stream_index== StreamVideoIn)) emit OnKeyFrame();
                            if (FileOutRenew && ! pAVFormatContextOut) {
                                if (FileName.length()) FileOpen(pAVFormatContextIn, &pAVFormatContextOut);
                            }
                            if (pAVFormatContextOut) {
                                AVPacket AVPacketTemp;
                                av_packet_ref(&AVPacketTemp, pAVPacket); {
                                    if (FileOutRenew) {
                                        if (StreamVideoIn== -1 || ((AVPacketTemp.flags & AV_PKT_FLAG_KEY) && AVPacketTemp.stream_index== StreamVideoIn)) {
                                            for (int count= 0; count< 1024; count++) {
                                                PreviousDts[count]= -1;
                                                PreviousPts[count]= -1;
                                            }
                                            FileClose(&pAVFormatContextOut);
                                            if (FileName.length()) FileOpen(pAVFormatContextIn, &pAVFormatContextOut);
                                            FileOutRenew= false;
                                        }
                                    }
                                    if (pAVFormatContextOut) {
                                        if (PreviousDts[AVPacketTemp.stream_index]== -1 && AVPacketTemp.dts!= AV_NOPTS_VALUE) {
                                            PreviousDts[AVPacketTemp.stream_index]= AVPacketTemp.dts;
                                            PreviousPts[AVPacketTemp.stream_index]= AVPacketTemp.pts;
                                        }
                                        if (AVPacketTemp.dts!= AV_NOPTS_VALUE) AVPacketTemp.dts-= PreviousDts[AVPacketTemp.stream_index];
                                        if (AVPacketTemp.pts!= AV_NOPTS_VALUE) AVPacketTemp.pts-= PreviousPts[AVPacketTemp.stream_index];
                                        av_packet_rescale_ts(&AVPacketTemp, pAVFormatContextIn->streams[AVPacketTemp.stream_index]->time_base, pAVFormatContextOut->streams[AVPacketTemp.stream_index]->time_base);
                                        av_interleaved_write_frame(pAVFormatContextOut, &AVPacketTemp);
                                    }
                                }{
                                    av_packet_unref(&AVPacketTemp);
                                }
                            }
                            if (RealTime) {
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
                                        if (IsPaused ) {
                                            IsPaused= false;
                                            TimeStart+= av_gettime()- PauseStart;
                                        }
                                        int64_t nowTime= av_gettime()- TimeStart;
                                        if ((DecodingTimeStampTemp- DecodingTimeStampStart)> nowTime) {
                                            av_usleep(DecodingTimeStampTemp- DecodingTimeStampStart- nowTime);
                                        }
                                    }
                                }
                            }
                            if (pAVPacket->stream_index== StreamAudioIn) {
                                Ret= avcodec_send_packet(pAVCodecContextAudio, pAVPacket);
                                if (Ret< 0) emit UpdateLog("StreamAudioIn Error submitting a packet for decoding.");
                                while (DoStart && Ret>= 0) {
                                    Ret= avcodec_receive_frame(pAVCodecContextAudio, pAVFrame);
                                    if (Ret>= 0) {
                                        //
                                        // begin
                                        SwrContext *pSwrContext= nullptr;
                                        Ret= swr_alloc_set_opts2(&pSwrContext,
                                                                  &pAVCodecContextAudio->ch_layout,     // out_ch_layout
                                                                  AV_SAMPLE_FMT_S16,                    // out_sample_fmt
                                                                  pAVCodecContextAudio->sample_rate,    // out_sample_rate
                                                                  &pAVCodecContextAudio->ch_layout,     // in_ch_layout
                                                                  pAVCodecContextAudio->sample_fmt,     // in_sample_fmt
                                                                  pAVCodecContextAudio->sample_rate,    // in_sample_rate
                                                                  0,
                                                                  nullptr);
                                        if (Ret== 0) {
                                            if (swr_init(pSwrContext)== 0) {
                                                int OutCount= (int64_t)pAVFrame->nb_samples * pAVCodecContextAudio->sample_rate / pAVFrame->sample_rate + 256;
                                                int OutSize= av_samples_get_buffer_size(nullptr,        // calculated linesize, may be NULL
                                                                                         pAVFrame->ch_layout.nb_channels,   // the number of channels
                                                                                         OutCount,      // 	the number of samples in a single channel
                                                                                         AV_SAMPLE_FMT_S16, // the sample format
                                                                                         0);    // align
                                                if (OutSize> 0) {
                                                    uint8_t *BufferOut= new uint8_t[OutSize]; {
                                                        Ret= swr_convert(pSwrContext,  // allocated Swr context, with parameters set
                                                                          &BufferOut,   // output buffers, only the first one need be set in case of packed audio
                                                                          OutCount,     // mount of space available for output in samples per channel
                                                                          const_cast<const uint8_t**>(pAVFrame->data),    // 	input buffers, only the first one need to be set in case of packed audio
                                                                          pAVFrame->nb_samples);    // number of input samples available in one channel
                                                        if (Ret> 0) {
                                                            int RetOut= Ret * pAVCodecContextAudio->ch_layout.nb_channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
                                                            if (DoStart && pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnAudio(BufferOut, RetOut);
                                                            if (DoStart) emit OnAudio(BufferOut, RetOut);
                                                            if (AudioSupport) {
                                                                QBAAudioBufferOut.append(reinterpret_cast<const char*>(BufferOut), RetOut);
                                                                #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                                                                    pQAudioOutput->setVolume(Volume);
                                                                #else
                                                                    pQAudioSink->setVolume(Volume);
                                                                #endif
                                                            }
                                                        }
                                                    }{
                                                        delete []BufferOut;
                                                    }
                                                }
                                            }
                                            if (pSwrContext) swr_free(&pSwrContext);
                                        }
                                        // end
                                        //
                                        #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                                            if (pQIODevice && pQAudioOutput && AudioSupport) {
                                                int BytesFree= pQAudioOutput->bytesFree();
                                                if (BytesFree<= QBAAudioBufferOut.length()) {
                                                    int BytesOut= pQIODevice->write(reinterpret_cast<const char*>(QBAAudioBufferOut.data()), BytesFree);
                                                    QBAAudioBufferOut.remove(0, BytesOut);
                                                } else {
                                                    int BytesOut= pQIODevice->write(reinterpret_cast<const char*>(QBAAudioBufferOut.data()), QBAAudioBufferOut.length());
                                                    QBAAudioBufferOut.remove(0, BytesOut);
                                                }
                                            }
                                        #else
                                            if (pQIODevice && pQIODevice->isWritable()) {
                                                qint64 BytesOut= pQIODevice->write(QBAAudioBufferOut);
                                                if (BytesOut> 0) QBAAudioBufferOut.remove(0, BytesOut);
                                            }
                                        #endif
                                    }
                                }
                            } else if (pAVPacket->stream_index== StreamVideoIn) {
                                Ret= avcodec_send_packet(pAVCodecContextVideo, pAVPacket);
                                if (Ret< 0) emit UpdateLog("Error submitting a packet for decoding.");
                                while (DoStart && Ret>= 0) {
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
                                                            // Some cameras such as Sricam black one, does not sends the pts value in AVFrame.
                                                            //if (pAVFrame->pts!= AV_NOPTS_VALUE) {
                                                                if (pQIFFmpegPlayerInterface) pQIFFmpegPlayerInterface->FFmpegPlayerOnImage(Image);
                                                                emit OnImage(Image);
                                                            //}
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
                            av_packet_unref(pAVPacket);
                        } else {
                            char Result[AV_ERROR_MAX_STRING_SIZE];
                            av_make_error_string(Result, AV_ERROR_MAX_STRING_SIZE, Ret);
                            emit UpdateLog(QString(Result));
                            break;
                        }
                    }
                    av_packet_free(&pAVPacket);
                }
                #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                    if (pQAudioOutput) delete pQAudioOutput;
                #else
                    if (pQAudioSink) delete pQAudioSink;
                #endif
                av_frame_free(&pAVFrame);
                if (pAVFormatContextOut) FileClose(&pAVFormatContextOut);
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

void QThFFmpegPlayer::FileClose(AVFormatContext **pAVFormatContextOut) {
    if (*pAVFormatContextOut) {
        av_write_trailer(*pAVFormatContextOut);
        avio_close((*pAVFormatContextOut)->pb);
        avformat_free_context(*pAVFormatContextOut);
        *pAVFormatContextOut= nullptr;
    }
}

void QThFFmpegPlayer::FileOpen(AVFormatContext *pAVFormatContextIn, AVFormatContext **pAVFormatContextOut) {
    avformat_alloc_output_context2(pAVFormatContextOut, nullptr, FormatName.toStdString().c_str(), FileName.toStdString().c_str());
    for (uint32_t count= 0; count< pAVFormatContextIn->nb_streams; count++) {
        AVStream *pAVStreamIn= pAVFormatContextIn->streams[count];
        AVStream *pAVStreamOut= avformat_new_stream(*pAVFormatContextOut, nullptr);
        avcodec_parameters_copy(pAVStreamOut->codecpar, pAVStreamIn->codecpar);
        pAVStreamOut->codecpar->codec_tag= 0;
    }
    avio_open(&(*pAVFormatContextOut)->pb, FileName.toStdString().c_str(), AVIO_FLAG_WRITE);
    if (avformat_write_header(*pAVFormatContextOut, nullptr)< 0) {
        avio_close((*pAVFormatContextOut)->pb);
        avformat_free_context(*pAVFormatContextOut);
        *pAVFormatContextOut= nullptr;
    }
}

void QThFFmpegPlayer::FileRenew(QString FileName) {
    this->FileName= FileName;
    FileOutRenew= true;
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

void QThFFmpegPlayer::VolumeSet(double Value) {
    Volume= Value;
}
