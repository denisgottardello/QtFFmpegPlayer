#ifndef QTHFFMPEGPLAYER_H
#define QTHFFMPEGPLAYER_H

extern "C" {
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavutil/imgutils.h"
    #include "libavutil/opt.h"
    #include "libavutil/time.h"
    #include "libswscale/swscale.h"
    #include "libswresample/swresample.h"
}
#include "qcffmpegplayecommons.h"
#include "QDateTime"
#include "QImage"
#include "QThread"
#ifndef QIFFMPEGPLAYERINTERFACE_H
    class QThFFmpegPlayer;
    #include "qiffmpegplayerinterface.h"
#endif

class QThFFmpegPlayer : public QThread
{
    Q_OBJECT

public:
    enum FFMPEGSourceTypes {
        FFMPEG_SOURCE_CALLBACK,
        FFMPEG_SOURCE_STREAM,
    };
    QThFFmpegPlayer(QString Path, bool RealTime= true, FFMPEGSourceTypes FFMPEGSourceType= FFMPEG_SOURCE_STREAM);
    ~QThFFmpegPlayer();
    bool DoStart= true;
    double Speed= 1;
    QIFFmpegPlayerInterface *pQIFFmpegPlayerInterface= nullptr;
    QDateTime QDTLastPacket;
    QElapsedTimer ElapsedTimer;
    int QThFFmpegPlayerReadPacket(uint8_t *pBuffer, int pBufferSize);
    void Stop();

signals:
    void OnAudio(const uchar* data, int Length);
    void OnAudioType(int SampleRate, int ChannelCount);
    void OnConnectionState(ConnectionStates ConnectionState);
    void OnEnd();
    void OnImage(QImage Image);
    void OnPacketRead(uint8_t *pBuffer, int pBufferSize, int *BytesIn);
    void UpdateLog(QString Log);

private:
    bool RealTime;
    FFMPEGSourceTypes FFMPEGSourceType;
    int Socket;
    int64_t DecodingTimeStampStart= -1;
    int64_t TimeStart= -1;
    QString Path;
    void run();
    int CodecContextOpen(int *pStreamIn, AVCodecContext **pAVCodecContext, AVFormatContext *pAVFormatContextIn, AVMediaType type, AVStream **pAVStream);
    QString AVMediaTypeToString(AVMediaType MediaType);
    void AVFrame2QImage(AVFrame *pAVFrame, QImage &Image, int Width, int Height);
    void runCommon(AVFormatContext *pAVFormatContextIn);

};

#endif // QTHFFMPEGPLAYER_H
