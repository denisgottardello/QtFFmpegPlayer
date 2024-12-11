#include "qfmainwindow.h"
#include "ui_qfmainwindow.h"

#ifdef Q_OS_WINDOWS
    #include <windows.h>
    void usleep(__int64 usec) {
        HANDLE timer;
        LARGE_INTEGER ft;
        ft.QuadPart= -(10*usec);
        timer = CreateWaitableTimer(NULL, TRUE, NULL);
        SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
        WaitForSingleObject(timer, INFINITE);
        CloseHandle(timer);
    }
#endif

QFMainWindow::QFMainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::QFMainWindow) {
    ui->setupUi(this);
    ui->QLImage->setBackgroundRole(QPalette::Dark);
    ui->QSAImage->setBackgroundRole(QPalette::Dark);
    QAudioFormat AudioFormat;
    AudioFormat.setByteOrder(QAudioFormat::LittleEndian);
    AudioFormat.setChannelCount(2);
    AudioFormat.setCodec("audio/pcm");
    AudioFormat.setSampleRate(44100);
    AudioFormat.setSampleSize(16);
    AudioFormat.setSampleType(QAudioFormat::SignedInt);
    for (int count= 0; count< QCoreApplication::arguments().count(); count++) {
        QString Parameter= QString(QCoreApplication::arguments().at(count));
        if (Parameter.length()> QString("--FilePath=").length()) {
            if (Parameter.leftRef(QString("--FilePath=").length()).toString().compare("--FilePath=")== 0) {
                ui->QLEFilePath->setText(Parameter.right(Parameter.length()- QString("--FilePath=").length()));
                break;
            }
        }
    }
    QAudioDeviceInfo AudioDeviceInfo= QAudioDeviceInfo::defaultOutputDevice();
    pQAudioOutput= new QAudioOutput(AudioDeviceInfo, AudioFormat, nullptr);
    pQAudioOutput->setVolume(ui->QDSBVolume->value());
    pQIODevice= pQAudioOutput->start();
}

QFMainWindow::~QFMainWindow() {
    if (pQThFFmpegPlayer) delete pQThFFmpegPlayer;
    delete ui;
}

void QFMainWindow::on_QDSBSpeed_valueChanged(double arg1) {
    if (pQThFFmpegPlayer) pQThFFmpegPlayer->Speed= arg1;
}

void QFMainWindow::on_QDSBVolume_valueChanged(double arg1) {
    if (pQThFFmpegPlayer) pQThFFmpegPlayer->VolumeSet(arg1);
}

void QFMainWindow::on_QPBPlay_clicked() {
    ui->QPBPlay->setEnabled(false);
    Frames= 0;
    ui->QLFrames->clear();;
    if (ui->QRBCallbackStream->isChecked()) {
        QFFileIn.setFileName(ui->QLEFilePath->text());
        if (QFFileIn.open(QIODevice::ReadOnly)) {
            PachetCount= 0;
            pQThFFmpegPlayer= new QThFFmpegPlayer("", ui->QCBRealTime->isChecked(), QThFFmpegPlayer::FFMPEG_SOURCE_CALLBACK);
            //connect(pQThFFmpegPlayer, SIGNAL(OnAudio(const uchar*,int)), this, SLOT(OnAudio(const uchar*,int)), Qt::BlockingQueuedConnection);
            connect(pQThFFmpegPlayer, SIGNAL(OnAudioType(int,int)), this, SLOT(OnAudioType(int,int)), Qt::BlockingQueuedConnection);
            connect(pQThFFmpegPlayer, SIGNAL(OnConnectionState(ConnectionStates)), this, SLOT(OnConnectionState(ConnectionStates)), Qt::BlockingQueuedConnection);
            connect(pQThFFmpegPlayer, SIGNAL(OnEnd()), this, SLOT(OnEnd()));
            connect(pQThFFmpegPlayer, SIGNAL(OnImage(QImage)), this, SLOT(OnImage(QImage)), Qt::BlockingQueuedConnection);
            connect(pQThFFmpegPlayer, SIGNAL(OnPacketRead(uint8_t*,int,int*)), this, SLOT(OnPacketRead(uint8_t*,int,int*)), Qt::BlockingQueuedConnection);
            connect(pQThFFmpegPlayer, SIGNAL(UpdateLog(QString)), this, SLOT(UpdateLog(QString)));
            pQThFFmpegPlayer->Speed= ui->QDSBSpeed->value();
            pQAudioOutput->moveToThread(pQThFFmpegPlayer);
            pQThFFmpegPlayer->start();
        }
    } else if (ui->QRBFFmpegStream->isChecked()) {
        pQThFFmpegPlayer= new QThFFmpegPlayer(ui->QLEFilePath->text(), ui->QCBRealTime->isChecked(), QThFFmpegPlayer::FFMPEG_SOURCE_STREAM);
        //connect(pQThFFmpegPlayer, SIGNAL(OnAudio(const uchar*,int)), this, SLOT(OnAudio(const uchar*,int)), Qt::BlockingQueuedConnection);
        connect(pQThFFmpegPlayer, SIGNAL(OnAudioType(int,int)), this, SLOT(OnAudioType(int,int)), Qt::BlockingQueuedConnection);
        connect(pQThFFmpegPlayer, SIGNAL(OnConnectionState(ConnectionStates)), this, SLOT(OnConnectionState(ConnectionStates)), Qt::BlockingQueuedConnection);
        connect(pQThFFmpegPlayer, SIGNAL(OnEnd()), this, SLOT(OnEnd()));
        connect(pQThFFmpegPlayer, SIGNAL(OnImage(QImage)), this, SLOT(OnImage(QImage)), Qt::BlockingQueuedConnection);
        connect(pQThFFmpegPlayer, SIGNAL(UpdateLog(QString)), this, SLOT(UpdateLog(QString)));
        pQThFFmpegPlayer->Speed= ui->QDSBSpeed->value();
        pQAudioOutput->moveToThread(pQThFFmpegPlayer);
        pQThFFmpegPlayer->start();
    }
    ui->QPBStop->setEnabled(true);
}

void QFMainWindow::on_QPBQuit_clicked() {
    if (ui->QPBStop->isEnabled()) on_QPBStop_clicked();
    this->close();
}

void QFMainWindow::on_QPBStop_clicked() {
    ui->QPBStop->setEnabled(false);
    if (pQThFFmpegPlayer) {
        disconnect(pQThFFmpegPlayer, SIGNAL(OnImage(QImage)), this, SLOT(OnImage(QImage)));
        pQThFFmpegPlayer->Stop();
    }
    ui->QPBPlay->setEnabled(true);
}

void QFMainWindow::on_QTBFilePath_clicked() {
    QFileDialog FileDialog(this);
    FileDialog.setDirectory(QFileInfo(ui->QLEFilePath->text()).dir().absolutePath());
    FileDialog.setViewMode(QFileDialog::Detail);
    if (FileDialog.exec()== QDialog::Accepted) {
        ui->QLEFilePath->setText(FileDialog.selectedFiles().at(0));
    }
}

void QFMainWindow::OnAudio(const uchar* data, int Length) {
    if (ui->QCBRealTime->isChecked() && ui->QCBAudio->isChecked()) {
        printf("pQAudioOutput->bytesFree() in: %d, Length: %d\n", pQAudioOutput->bytesFree(), Length);
        fflush(stdout);
        while (pQAudioOutput->bytesFree()< Length) usleep(100);
        pQIODevice->write(reinterpret_cast<const char*>(data), Length);
    }
}

void QFMainWindow::OnAudioType(int SampleRate, int ChannelCount) {
    QAudioFormat AudioFormat;
    AudioFormat.setByteOrder(QAudioFormat::LittleEndian);
    AudioFormat.setChannelCount(ChannelCount);
    AudioFormat.setCodec("audio/pcm");
    AudioFormat.setSampleRate(SampleRate);
    AudioFormat.setSampleSize(16);
    AudioFormat.setSampleType(QAudioFormat::SignedInt);
    QAudioDeviceInfo AudioDeviceInfo(QAudioDeviceInfo::defaultOutputDevice());
    if (AudioDeviceInfo.isFormatSupported(AudioFormat)) {
        qreal Volume= pQAudioOutput->volume();
        delete pQAudioOutput;
        QAudioDeviceInfo AudioDeviceInfo= QAudioDeviceInfo::defaultOutputDevice();
        pQAudioOutput= new QAudioOutput(AudioDeviceInfo, AudioFormat);
        pQAudioOutput->setVolume(Volume);
        pQAudioOutput->moveToThread(pQThFFmpegPlayer);
        pQIODevice= pQAudioOutput->start();
        ui->QLAudioType->setText("SampleRate: "+ QString::number(SampleRate)+ ", ChannelCount: "+ QString::number(ChannelCount));
    } else {
        UpdateLog(tr("Unsupported AudioFormat!!!"));
        ui->QLAudioType->setText("SampleRate: "+ QString::number(SampleRate)+ ", ChannelCount: "+ QString::number(ChannelCount)+ tr(", unsupported audio format!!!"));
    }
}

void QFMainWindow::OnConnectionState(ConnectionStates ConnectionState) {
    switch(ConnectionState) {
        case CONNECTION_STATE_CONNECTED: ui->QLConnectionState->setText(tr("Connected")); break;
        case CONNECTION_STATE_CONNECTED_DATA_RECEIVED: ui->QLConnectionState->setText(tr("Data received")); break;
        case CONNECTION_STATE_CONNECTING: ui->QLConnectionState->setText(tr("Connecting")); break;
        case CONNECTION_STATE_IDLE: {
            pQThFFmpegPlayer->deleteLater();
            pQThFFmpegPlayer= nullptr;
            ui->QLConnectionState->setText(tr("Idle"));
            ui->QLImage->clear();
            break;
        }
    }
}

void QFMainWindow::OnEnd() {
    ui->QPBStop->click();
}

void QFMainWindow::OnImage(QImage Image) {
    Frames++;
    if (ui->QPBStop->isEnabled()) {
        ui->QLImage->clear();
        Image= Image.scaled(QSize(Image.width() * ui->QDSBZoom->value(), Image.height() * ui->QDSBZoom->value()), Qt::KeepAspectRatio);
        ui->QLImage->setPixmap(QPixmap::fromImage((Image)));
    }
    ui->QLFrames->setText("Frames: "+ QString::number(Frames));
}

void QFMainWindow::OnPacketRead(uint8_t *pBuffer, int pBufferSize, int *BytesIn) {
    QByteArray QBAByteIn= QFFileIn.read(pBufferSize);
    memcpy(pBuffer, QBAByteIn.data(), static_cast<ulong>(QBAByteIn.size()));
    *BytesIn= QBAByteIn.size();
    PachetCount++;
    //qDebug() << "PachetCount:" << PachetCount << "bytes:" << *BytesIn << "pBufferSize:" << pBufferSize;
}

void QFMainWindow::UpdateLog(QString Log) {
    qDebug() << Log;
}
