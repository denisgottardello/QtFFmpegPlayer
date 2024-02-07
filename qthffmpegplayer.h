#ifndef QTHPLAYER_H
#define QTHPLAYER_H

#include "QThread"
extern "C" {
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavutil/imgutils.h"
    #include "libavutil/opt.h"
    #include "libavutil/time.h"
    #include "libswscale/swscale.h"
    #include "libswresample/swresample.h"
}
#include "QDateTime"
#include "QImage"
#include "qiffmpegplayerinterface.h"

enum FrameTypes {
    FRAME_TYPE_AUDIO,
    FRAME_TYPE_VIDEO,
};

enum RTSPTransports {
    RTSP_TRANSPORT_AUTO= 0,
    RTSP_TRANSPORT_HTTP= 1,
    RTSP_TRANSPORT_TCP= 2,
    RTSP_TRANSPORT_UDP= 3,
    RTSP_TRANSPORT_UDP_MULTICAST= 4,
};

struct Frame {
    FrameTypes FrameType;
    double pts;
    QImage Image;
    QByteArray QBABuffer;
    Frame() : FrameType(FRAME_TYPE_VIDEO), pts(0) {}
};

class QThFFmpegPlayer : public QThread
{
    Q_OBJECT

public:
    enum FFMPEGSourceTypes {
        FFMPEG_SOURCE_CALLBACK,
        FFMPEG_SOURCE_STREAM,
    };
    QThFFmpegPlayer(QString Path, bool RealTime, FFMPEGSourceTypes FFMPEGSourceType, RTSPTransports RTSPTransport= RTSP_TRANSPORT_AUTO, bool DecodeFrames= true);
    ~QThFFmpegPlayer();
    QIFFmpegPlayerInterface *pQIFFmpegPlayerInterface= nullptr;
    QDateTime QDTLastPacket;
    int QThFFmpegPlayerReadPacket(uint8_t *pBuffer, int pBufferSize);
    void SpeedSet(int Speed);
    void Stop();

signals:
    void OnAudio(const uchar* data, int Length);
    void OnAudioType(int SampleRate, int ChannelCount);
    void OnEnd();
    void OnImage(QImage Image);
    void OnPacketRead(uint8_t *pBuffer, int pBufferSize, int *BytesIn);
    void UpdateLog(QString Log);

private:
    bool DoStart= true, RealTime, DecodeFrames;
    FFMPEGSourceTypes FFMPEGSourceType;
    int RTSPTransport, Socket, Speed= 1;
    qint64 LastFrame, Frames;
    QString Path;
    QVector<Frame> QVFrames;
    void run();
    int CodecContextOpen(int *pStreamIn, AVCodecContext **pAVCodecContext, AVFormatContext *pAVFormatContextIn, AVMediaType type, AVStream **pAVStream);
    int PacketDecodeAudio(AVFrame *pAVFrame, AVCodecContext *pAVCodecContextAudio, AVPacket *pAVPacket, SwrContext *pSwrContext, qint64 &Counter);
    int PacketDecodeVideo(AVFrame *pAVFrame, AVCodecContext *pAVCodecContextVideo, AVPacket *pAVPacket, AVStream *pAVStreamVideo);
    void AVFrame2QImage(AVFrame *pAVFrame, QImage &Image, int Width, int Height);
    void runCommon(AVFormatContext *pAVFormatContextIn);

};

#endif // QTHPLAYER_H
