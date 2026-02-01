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
    QThFFmpegPlayer(QString Path, bool RealTime= true, FFMPEGSourceTypes FFMPEGSourceType= FFMPEG_SOURCE_STREAM, bool AudioSupport= false, QString FileName= "", QString Resolution= "320x240", QString FormatName= "mp4",  QString RTSPTransport= "tcp");
    ~QThFFmpegPlayer();
    bool DoStart= true, Pause= false;
    double Position= -1;
    double Speed= 1;
    double Volume= 1;
    QIFFmpegPlayerInterface *pQIFFmpegPlayerInterface= nullptr;
    QDateTime QDTLastPacket;
    QElapsedTimer ElapsedTimer;
    int QThFFmpegPlayerCallbackRead(uint8_t *pBuffer, int pBufferSize);
    int64_t QThFFmpegPlayerCallbackSeek(int64_t offset, int whence);
    void FileRenew(QString FileName);
    void PositionSet(double Value);
    void Stop();
    void VolumeSet(double Value);

signals:
    int64_t OnCallbackSeek(int64_t offset, int whence);
    void OnAudio(const uchar* data, int Length);
    void OnAudioType(int SampleRate, int ChannelCount);
    void OnCallbackRead(uint8_t *pBuffer, int pBufferSize, int *BytesIn);
    void OnConnectionState(ConnectionStates ConnectionState);
    void OnDuration(double Value);
    void OnEnd();
    void OnImage(QImage Image);
    void OnKeyFrame();
    void OnPosition(double Value);
    void OnSeekable(bool Seekable);
    void UpdateLog(QString Log);

private:
    bool RealTime, AudioSupport, FileOutRenew= false;
    double SpeedCurrent= 1;
    FFMPEGSourceTypes FFMPEGSourceType;
    int Socket;
    QString Path, FileName, Resolution, FormatName, RTSPTransport;
    void run();
    int CodecContextOpen(int *pStreamIn, AVCodecContext **pAVCodecContext, AVFormatContext *pAVFormatContextIn, AVMediaType type, AVStream **pAVStream);
    QString AVMediaTypeToString(AVMediaType MediaType);
    void AVFrame2QImage(AVFrame *pAVFrame, QImage &Image, int Width, int Height);
    void Delay(AVStream *pAVStream, AVFrame *pAVFrame, bool *pSeekExecuted, bool *pIsPaused, int64_t *pTimeStart, int64_t *pPauseStart);
    void FileClose(AVFormatContext **pAVFormatContextOut);
    void FileOpen(AVFormatContext *pAVFormatContextIn, AVFormatContext **pAVFormatContextOut);
    void runCommon(AVFormatContext *pAVFormatContextIn);

};

#endif // QTHFFMPEGPLAYER_H
