#ifndef QFMAINWINDOW_H
#define QFMAINWINDOW_H

#include <QMainWindow>
#include "QDebug"
#include "qthffmpegplayer.h"
#include "QFile"
#include "QAudioFormat"
#include "QAudioOutput"

namespace Ui {
    class QFMainWindow;
}

class QFMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit QFMainWindow(QWidget *parent = nullptr);
    ~QFMainWindow();

public slots:
    void OnPacketRead(uint8_t *pBuffer, int pBufferSize, int *BytesIn);

private slots:
    void on_QPBPlay_clicked();
    void on_QPBQuit_clicked();
    void on_QPBStop_clicked();
    void OnAudio(const uchar* data, int Length);
    void OnAudioType(int SampleRate, int ChannelCount);
    void OnEnd();
    void OnImage(QImage Image);
    void UpdateLog(QString Log);

private:
    Ui::QFMainWindow *ui;
    int PachetCount, Frames;
    QAudioOutput *pQAudioOutput= nullptr;
    QIODevice *pQIODevice= nullptr;
    QFile QFFileIn;
    QThFFmpegPlayer *pQThFFmpegPlayer= nullptr;

};

#endif // QFMAINWINDOW_H
