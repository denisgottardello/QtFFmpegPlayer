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
#include "QTcpSocket"

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
};

class QThFFmpegPlayer : public QThread
{
    Q_OBJECT

public:
    enum FFMPEGSourceTypes {
        FFMPEG_SOURCE_CALLBACK,
        FFMPEG_SOURCE_HTTP,
        FFMPEG_SOURCE_STREAM,
    };
    QThFFmpegPlayer(QString Path, QString Server, int Socket, QString UserID, QString Password, QString Function, bool RealTime, FFMPEGSourceTypes FFMPEGSourceType, RTSPTransports RTSPTransport= RTSP_TRANSPORT_AUTO);
    ~QThFFmpegPlayer();
    QIFFmpegPlayerInterface *pQIFFmpegPlayerInterface= nullptr;
    QDateTime QDTLastFrame;
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
    bool DoStart= true, RealTime;
    FFMPEGSourceTypes FFMPEGSourceType;
    int RTSPTransport, Socket, Speed= 1;
    qint64 LastFrame, Frames;
    QString Path, Server, UserID, Password, Function;
    QVector<Frame> QVFrames;
    void run();
    int CodecContextOpen(int *pStreamIn, AVCodecContext **pAVCodecContext, AVFormatContext *pAVFormatContextIn, AVMediaType type, AVStream **pAVStream);
    int PacketDecodeAudio(AVFrame *pAVFrame, AVCodecContext *pAVCodecContextAudio, AVPacket *pAVPacket, SwrContext *pSwrContext, qint64 &Counter);
    int PacketDecodeVideo(AVFrame *pAVFrame, AVCodecContext *pAVCodecContextVideo, AVPacket *pAVPacket, AVStream *pAVStreamVideo);
    void AVFrame2QImage(AVFrame *pAVFrame, QImage &Image, int Width, int Height);
    void runCommon(AVFormatContext *pAVFormatContextIn);

};

#endif // QTHPLAYER_H
