#ifndef QIFFMPEGPLAYERINTERFACE_H
#define QIFFMPEGPLAYERINTERFACE_H

#include "QImage"

class QIFFmpegPlayerInterface
{
public:
    QIFFmpegPlayerInterface();
    virtual ~QIFFmpegPlayerInterface();
    virtual void FFmpegPlayerOnAudio(const uchar *data, int Length)= 0;
    virtual void FFmpegPlayerOnAudioType(int SampleRate, int ChannelCount)= 0;
    virtual void FFmpegPlayerOnImage(QImage &Image)= 0;

};

#endif // QIFFMPEGPLAYERINTERFACE_H
