#ifndef QTHFFMPEGPLAYER_H
#define QTHFFMPEGPLAYER_H

extern "C" {
    #include "libavcodec/avcodec.h"
    #include "libavdevice/avdevice.h"
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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    #include "QAudioFormat"
    #include "QAudioOutput"
#else
    #include "QAudioDevice"
    #include "QAudioSink"
    #include "QMediaDevices"
#endif
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
        FFMPEG_SOURCE_DEVICE,
        FFMPEG_SOURCE_STREAM,
    };
    QThFFmpegPlayer(QString Path, bool RealTime= true, FFMPEGSourceTypes FFMPEGSourceType= FFMPEG_SOURCE_STREAM, bool AudioSupport= false, QString FileName= "", QString Resolution= "320x240");
    ~QThFFmpegPlayer();
    bool DoStart= true;
    double Speed= 1;
    double Volume= 1;
    QIFFmpegPlayerInterface *pQIFFmpegPlayerInterface= nullptr;
    QDateTime QDTLastPacket;
    QElapsedTimer ElapsedTimer;
    int QThFFmpegPlayerReadPacket(uint8_t *pBuffer, int pBufferSize);
    void FileRenew(QString FileName);
    void Stop();
    void VolumeSet(double Value);

signals:
    void OnAudio(const uchar* data, int Length);
    void OnAudioType(int SampleRate, int ChannelCount);
    void OnConnectionState(ConnectionStates ConnectionState);
    void OnEnd();
    void OnImage(QImage Image);
    void OnKeyFrame();
    void OnPacketRead(uint8_t *pBuffer, int pBufferSize, int *BytesIn);
    void UpdateLog(QString Log);

private:
    bool RealTime, AudioSupport, FileOutRenew= false;
    FFMPEGSourceTypes FFMPEGSourceType;
    int Socket;
    int64_t DecodingTimeStampStart= -1;
    int64_t TimeStart= -1;
    QString Path, FileName, Resolution;
    void run();
    int CodecContextOpen(int *pStreamIn, AVCodecContext **pAVCodecContext, AVFormatContext *pAVFormatContextIn, AVMediaType type, AVStream **pAVStream);
    QString AVMediaTypeToString(AVMediaType MediaType);
    void AVFrame2QImage(AVFrame *pAVFrame, QImage &Image, int Width, int Height);
    void FileClose(AVFormatContext **pAVFormatContextOut);
    void FileOpen(AVFormatContext *pAVFormatContextIn, AVFormatContext **pAVFormatContextOut);
    void runCommon(AVFormatContext *pAVFormatContextIn);

};

#endif // QTHFFMPEGPLAYER_H
