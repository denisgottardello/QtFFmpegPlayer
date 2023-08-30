#include "qfmainwindow.h"
#include "ui_qfmainwindow.h"

QFMainWindow::QFMainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::QFMainWindow) {
    ui->setupUi(this);
    ui->QLImage->setBackgroundRole(QPalette::Dark);
    ui->QSAImage->setBackgroundRole(QPalette::Dark);
    QAudioFormat AudioFormat;
    AudioFormat.setChannelCount(2);
    AudioFormat.setCodec("audio/pcm");
    AudioFormat.setSampleRate(44100);
    AudioFormat.setSampleSize(16);
    AudioFormat.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo AudioDeviceInfo(QAudioDeviceInfo::defaultOutputDevice());
    if (!AudioDeviceInfo.isFormatSupported(AudioFormat)) {
        qDebug() << "raw audio format not supported by backend, cannot play audio.";
        AudioFormat= AudioDeviceInfo.nearestFormat(AudioFormat);
    }
    pQAudioOutput= new QAudioOutput(AudioFormat, this);
    pQIODevice= pQAudioOutput->start();
}

QFMainWindow::~QFMainWindow() {
    if (pQThFFmpegPlayer) delete pQThFFmpegPlayer;
    if (pQAudioOutput) delete pQAudioOutput;
    delete ui;
}

void QFMainWindow::on_QPBPlay_clicked() {
    ui->QPBPlay->setEnabled(false);
    Frames= 0;
    if (ui->QRBCallbackStream->isChecked()) {
        QFFileIn.setFileName(ui->QLEPath->text());
        if (QFFileIn.open(QIODevice::ReadOnly)) {
            PachetCount= 0;
            pQThFFmpegPlayer= new QThFFmpegPlayer(ui->QLEPath->text(), "", 0, "", "", "", ui->QCBRealTime->isChecked(), QThFFmpegPlayer::FFMPEG_SOURCE_CALLBACK);
            connect(pQThFFmpegPlayer, SIGNAL(OnAudio(const uchar*,int)), this, SLOT(OnAudio(const uchar*,int)), Qt::BlockingQueuedConnection);
            connect(pQThFFmpegPlayer, SIGNAL(OnAudioType(int,int)), this, SLOT(OnAudioType(int,int)), Qt::BlockingQueuedConnection);
            connect(pQThFFmpegPlayer, SIGNAL(OnEnd()), this, SLOT(OnEnd()));
            connect(pQThFFmpegPlayer, SIGNAL(OnImage(QImage)), this, SLOT(OnImage(QImage)), Qt::BlockingQueuedConnection);
            connect(pQThFFmpegPlayer, SIGNAL(OnPacketRead(uint8_t*,int,int*)), this, SLOT(OnPacketRead(uint8_t*,int,int*)), Qt::BlockingQueuedConnection);
            connect(pQThFFmpegPlayer, SIGNAL(UpdateLog(QString)), this, SLOT(UpdateLog(QString)));
            pQThFFmpegPlayer->start();
        }
    } else if (ui->QRBFFmpegStream->isChecked()) {
        pQThFFmpegPlayer= new QThFFmpegPlayer(ui->QLEPath->text(), "", 0, "", "", "", ui->QCBRealTime->isChecked(), QThFFmpegPlayer::FFMPEG_SOURCE_STREAM, RTSP_TRANSPORT_UDP);
        connect(pQThFFmpegPlayer, SIGNAL(OnAudio(const uchar*,int)), this, SLOT(OnAudio(const uchar*,int)), Qt::BlockingQueuedConnection);
        connect(pQThFFmpegPlayer, SIGNAL(OnAudioType(int,int)), this, SLOT(OnAudioType(int,int)), Qt::BlockingQueuedConnection);
        connect(pQThFFmpegPlayer, SIGNAL(OnEnd()), this, SLOT(OnEnd()));
        connect(pQThFFmpegPlayer, SIGNAL(OnImage(QImage)), this, SLOT(OnImage(QImage)), Qt::BlockingQueuedConnection);
        connect(pQThFFmpegPlayer, SIGNAL(UpdateLog(QString)), this, SLOT(UpdateLog(QString)));
        pQThFFmpegPlayer->start();
    } else {
        pQThFFmpegPlayer= new QThFFmpegPlayer(ui->QLEPath->text(), "domusboss", 3001, "Admin", "sissi", "/Live?id_camera=8&Type=mjpg", ui->QCBRealTime->isChecked(), QThFFmpegPlayer::FFMPEG_SOURCE_HTTP);
        connect(pQThFFmpegPlayer, SIGNAL(OnAudio(const uchar*,int)), this, SLOT(OnAudio(const uchar*,int)), Qt::BlockingQueuedConnection);
        connect(pQThFFmpegPlayer, SIGNAL(OnAudioType(int,int)), this, SLOT(OnAudioType(int,int)), Qt::BlockingQueuedConnection);
        connect(pQThFFmpegPlayer, SIGNAL(OnEnd()), this, SLOT(OnEnd()));
        connect(pQThFFmpegPlayer, SIGNAL(OnImage(QImage)), this, SLOT(OnImage(QImage)), Qt::BlockingQueuedConnection);
        connect(pQThFFmpegPlayer, SIGNAL(UpdateLog(QString)), this, SLOT(UpdateLog(QString)));
        pQThFFmpegPlayer->start();
    }
    ui->QPBStop->setEnabled(true);
}

void QFMainWindow::on_QPBStop_clicked() {
    ui->QPBStop->setEnabled(false);
    if (pQThFFmpegPlayer) pQThFFmpegPlayer->Stop();
}

void QFMainWindow::on_QPBQuit_clicked() {
    if (ui->QPBStop->isEnabled()) on_QPBStop_clicked();
    this->close();
}

void QFMainWindow::OnAudio(const uchar* data, int Length) {
    pQIODevice->write(reinterpret_cast<const char*>(data), Length);
}

void QFMainWindow::OnAudioType(int SampleRate, int ChannelCount) {
    QAudioFormat AudioFormat= pQAudioOutput->format();
    AudioFormat.setChannelCount(ChannelCount);
    AudioFormat.setSampleRate(SampleRate);
    delete pQAudioOutput;
    pQAudioOutput= new QAudioOutput(AudioFormat, this);
    pQIODevice= pQAudioOutput->start();
}

void QFMainWindow::OnEnd() {
    UpdateLog("OnEnd() begin");
    if (pQThFFmpegPlayer) {
        delete pQThFFmpegPlayer;
        pQThFFmpegPlayer= nullptr;
    }
    ui->QPBPlay->setEnabled(true);
    UpdateLog("OnEnd() end");
}

void QFMainWindow::OnImage(QImage Image) {
    Frames++;
    if (ui->QPBStop->isEnabled()) {
        ui->QLImage->clear();
        ui->QLImage->setPixmap(QPixmap::fromImage((Image)));
        ui->QLImage->adjustSize();
        ui->QSAImage->setMinimumWidth(ui->QLImage->width()+ 8);
        ui->QSAImage->setMinimumHeight(ui->QLImage->height()+ 8);
    }
    ui->QLFrames->setText(QString::number(Frames));
}

void QFMainWindow::OnPacketRead(uint8_t *pBuffer, int pBufferSize, int *BytesIn) {
    QByteArray QBAByteIn= QFFileIn.read(pBufferSize);
    memcpy(pBuffer, QBAByteIn.data(), static_cast<ulong>(QBAByteIn.size()));
    *BytesIn= QBAByteIn.size();
    PachetCount++;
    qDebug() << "PachetCount:" << PachetCount << "bytes:" << *BytesIn << "pBufferSize:" << pBufferSize;
}

void QFMainWindow::UpdateLog(QString Log) {
    qDebug() << Log;
}
