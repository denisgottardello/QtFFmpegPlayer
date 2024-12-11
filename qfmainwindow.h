#ifndef QFMAINWINDOW_H
#define QFMAINWINDOW_H

#include "QDebug"
#include "QFile"
#include "QFileDialog"
#include <QMainWindow>
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
    void on_QDSBSpeed_valueChanged(double arg1);
    void on_QDSBVolume_valueChanged(double arg1);
    void on_QPBPlay_clicked();
    void on_QPBQuit_clicked();
    void on_QPBStop_clicked();
    void on_QTBFilePath_clicked();
    void OnAudio(const uchar* data, int Length);
    void OnAudioType(int SampleRate, int ChannelCount);
    void OnConnectionState(ConnectionStates ConnectionState);
    void OnEnd();
    void OnImage(QImage Image);
    void OnPacketRead(uint8_t *pBuffer, int pBufferSize, int *BytesIn);
    void UpdateLog(QString Log);

private:
    Ui::QFMainWindow *ui;
    int PachetCount, Frames;
    QAudioOutput *pQAudioOutput= nullptr;
    QFile QFFileIn;
    QIODevice *pQIODevice= nullptr;
    QThFFmpegPlayer *pQThFFmpegPlayer= nullptr;

};

#endif // QFMAINWINDOW_H
