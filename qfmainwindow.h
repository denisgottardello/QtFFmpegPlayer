#ifndef QFMAINWINDOW_H
#define QFMAINWINDOW_H

#include "QDebug"
#include "QFile"
#include "QFileDialog"
#include <QMainWindow>
#include "QTimer"
#ifndef QTHFFMPEGPLAYER_H
    class QFMainWindow;
    #include "qthffmpegplayer.h"
#endif
#ifdef Q_OS_UNIX
    #include "unistd.h"
#endif

namespace Ui {
    class QFMainWindow;
}

class QFMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit QFMainWindow(QWidget *parent = nullptr);
    ~QFMainWindow();

private slots:
    int64_t OnCallbackSeek(int64_t offset, int whence);
    void on_QCBRecord_toggled(bool checked);
    void on_QDSBSpeed_valueChanged(double arg1);
    void on_QDSBVolume_valueChanged(double arg1);
    void on_QHSPosition_sliderMoved(int position);
    void on_QHSPosition_sliderReleased();
    void on_QPBPause_toggled(bool checked);
    void on_QPBPlay_clicked();
    void on_QPBQuit_clicked();
    void on_QPBStop_clicked();
    void on_QTBFilePath_clicked();
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
    void OnTimer();
    void UpdateLog(QString Log);

private:
    Ui::QFMainWindow *ui;
    int PachetCount, KeyFrameCount, FrameCount;
    QByteArray QBAAudioBufferOut;
    QDateTime QDTFileNew;
    QFile QFFileIn;
    QIODevice *pQIODevice= nullptr;
    QThFFmpegPlayer *pQThFFmpegPlayer= nullptr;
    QTimer Timer;
    QVector<Interface> QVInterfaces;
    #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QAudioOutput *pQAudioOutput= nullptr;
    #else
        QAudioSink *pQAudioSink= nullptr;
    #endif

};

#endif // QFMAINWINDOW_H
