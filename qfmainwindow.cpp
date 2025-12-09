#include "qfmainwindow.h"
#include "ui_qfmainwindow.h"

QFMainWindow::QFMainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::QFMainWindow) {
    ui->setupUi(this);
    ui->QLImage->setBackgroundRole(QPalette::Dark);
    ui->QSAImage->setBackgroundRole(QPalette::Dark);
    if (InterfacesList(QVInterfaces)) {
        for (int count= 0; count< QVInterfaces.size(); count++) ui->QCBCameras->addItem(QString::number(QVInterfaces.at(count).index)+ ", "+ QVInterfaces.at(count).Name);
    }
    #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QAudioFormat AudioFormat;
        AudioFormat.setByteOrder(QAudioFormat::LittleEndian);
        AudioFormat.setChannelCount(2);
        AudioFormat.setCodec("audio/pcm");
        AudioFormat.setSampleRate(44100);
        AudioFormat.setSampleSize(16);
        AudioFormat.setSampleType(QAudioFormat::SignedInt);
    #else
        QAudioFormat AudioFormat;
        AudioFormat.setChannelCount(2);
        AudioFormat.setSampleFormat(QAudioFormat::Int16);
        AudioFormat.setSampleRate(44100);
    #endif
    for (int count= 0; count< QCoreApplication::arguments().count(); count++) {
        QString Parameter= QString(QCoreApplication::arguments().at(count));
        if (Parameter.length()> QString("--FilePath=").length()) {
            if (Parameter.left(QString("--FilePath=").length()).compare("--FilePath=")== 0) {
                ui->QLEFilePath->setText(Parameter.right(Parameter.length()- QString("--FilePath=").length()));
                break;
            }
        }
    }
    #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QAudioDeviceInfo AudioDeviceInfo= QAudioDeviceInfo::defaultOutputDevice();
        pQAudioOutput= new QAudioOutput(AudioDeviceInfo, AudioFormat, nullptr);
        pQAudioOutput->setVolume(ui->QDSBVolume->value());
        pQIODevice= pQAudioOutput->start();
    #else
        QAudioDevice pQAudioDevice= QMediaDevices::defaultAudioOutput();
        if (!pQAudioDevice.isFormatSupported(AudioFormat)) {
            AudioFormat= pQAudioDevice.preferredFormat();
        }
        pQAudioSink= new QAudioSink(pQAudioDevice, AudioFormat);
        pQIODevice= pQAudioSink->start();
        pQAudioSink->setVolume(ui->QDSBVolume->value());
    #endif
    connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));
}

QFMainWindow::~QFMainWindow() {
    #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        if (pQAudioOutput) delete pQAudioOutput;
    #else
        if (pQAudioSink) delete pQAudioSink;
    #endif
    if (pQThFFmpegPlayer) delete pQThFFmpegPlayer;
    delete ui;
}

void QFMainWindow::on_QDSBSpeed_valueChanged(double arg1) {
    if (pQThFFmpegPlayer) pQThFFmpegPlayer->Speed= arg1;
}

void QFMainWindow::on_QDSBVolume_valueChanged(double arg1) {
    #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        pQAudioOutput->setVolume(arg1);
    #else
        pQAudioSink->setVolume(arg1);
    #endif
    if (pQThFFmpegPlayer) pQThFFmpegPlayer->VolumeSet(arg1);
}

void QFMainWindow::on_QPBPlay_clicked() {
    ui->QPBPlay->setEnabled(false);
    QBAAudioBufferOut.clear();
    FrameCount= 0;
    KeyFrameCount= 0;
    ui->QLFrames->clear();;
    if (ui->QRBCallbackStream->isChecked()) {
        QFFileIn.setFileName(ui->QLEFilePath->text());
        if (QFFileIn.open(QIODevice::ReadOnly)) {
            PachetCount= 0;
            pQThFFmpegPlayer= new QThFFmpegPlayer("", ui->QCBRealTime->isChecked(), QThFFmpegPlayer::FFMPEG_SOURCE_CALLBACK, false, ui->QCBRecord->isChecked() ? QDateTime::currentDateTime().toString("yyyyMMdd hh:mm:ss")+ ".mp4" : "", "", ui->QLEFormatName->text());
            connect(pQThFFmpegPlayer, SIGNAL(OnAudio(const uchar*,int)), this, SLOT(OnAudio(const uchar*,int)), Qt::BlockingQueuedConnection);
            connect(pQThFFmpegPlayer, SIGNAL(OnAudioType(int,int)), this, SLOT(OnAudioType(int,int)), Qt::BlockingQueuedConnection);
            connect(pQThFFmpegPlayer, SIGNAL(OnConnectionState(ConnectionStates)), this, SLOT(OnConnectionState(ConnectionStates)), Qt::BlockingQueuedConnection);
            connect(pQThFFmpegPlayer, SIGNAL(OnEnd()), this, SLOT(OnEnd()));
            connect(pQThFFmpegPlayer, SIGNAL(OnImage(QImage)), this, SLOT(OnImage(QImage)), Qt::BlockingQueuedConnection);
            connect(pQThFFmpegPlayer, SIGNAL(OnKeyFrame()), this, SLOT(OnKeyFrame()));
            connect(pQThFFmpegPlayer, SIGNAL(OnPacketRead(uint8_t*,int,int*)), this, SLOT(OnPacketRead(uint8_t*,int,int*)), Qt::BlockingQueuedConnection);
            connect(pQThFFmpegPlayer, SIGNAL(UpdateLog(QString)), this, SLOT(UpdateLog(QString)));
            pQThFFmpegPlayer->Speed= ui->QDSBSpeed->value();
            pQThFFmpegPlayer->start();
            QDTFileNew= QDateTime::currentDateTime();
            Timer.start(1000);
        }
    } else if (ui->QRBFFmpegStream->isChecked()) {
        if (ui->QRBCameras->isChecked()) {
            if (ui->QCBCameras->currentIndex()> -1) pQThFFmpegPlayer= new QThFFmpegPlayer(QVInterfaces.at(ui->QCBCameras->currentIndex()).Path, ui->QCBRealTime->isChecked(), QThFFmpegPlayer::FFMPEG_SOURCE_DEVICE, false, ui->QCBRecord->isChecked() ? QDateTime::currentDateTime().toString("yyyyMMdd hh:mm:ss")+ ".mp4" : "", ui->QCBResolution->currentText(), ui->QLEFormatName->text());
        } else pQThFFmpegPlayer= new QThFFmpegPlayer(ui->QLEFilePath->text(), ui->QCBRealTime->isChecked(), QThFFmpegPlayer::FFMPEG_SOURCE_STREAM, false, ui->QCBRecord->isChecked() ? QDateTime::currentDateTime().toString("yyyyMMdd hh:mm:ss")+ ".mp4" : "", "", ui->QLEFormatName->text(), ui->QLERTSPTransport->text());
        if (pQThFFmpegPlayer) {
            connect(pQThFFmpegPlayer, SIGNAL(OnAudio(const uchar*,int)), this, SLOT(OnAudio(const uchar*,int)), Qt::BlockingQueuedConnection);
            connect(pQThFFmpegPlayer, SIGNAL(OnAudioType(int,int)), this, SLOT(OnAudioType(int,int)), Qt::BlockingQueuedConnection);
            connect(pQThFFmpegPlayer, SIGNAL(OnConnectionState(ConnectionStates)), this, SLOT(OnConnectionState(ConnectionStates)), Qt::BlockingQueuedConnection);
            connect(pQThFFmpegPlayer, SIGNAL(OnEnd()), this, SLOT(OnEnd()));
            connect(pQThFFmpegPlayer, SIGNAL(OnImage(QImage)), this, SLOT(OnImage(QImage)), Qt::BlockingQueuedConnection);
            connect(pQThFFmpegPlayer, SIGNAL(OnKeyFrame()), this, SLOT(OnKeyFrame()));
            connect(pQThFFmpegPlayer, SIGNAL(UpdateLog(QString)), this, SLOT(UpdateLog(QString)));
            pQThFFmpegPlayer->Speed= ui->QDSBSpeed->value();
            pQThFFmpegPlayer->start();
            QDTFileNew= QDateTime::currentDateTime();
            Timer.start(1000);
        }
    }
    ui->QPBStop->setEnabled(true);
}

void QFMainWindow::on_QPBQuit_clicked() {
    if (ui->QPBStop->isEnabled()) on_QPBStop_clicked();
    this->close();
}

void QFMainWindow::on_QPBStop_clicked() {
    ui->QPBStop->setEnabled(false);
    Timer.stop();
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
        QBAAudioBufferOut.append(reinterpret_cast<const char*>(data), Length);
        #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            if (pQIODevice) {
                int BytesFree= pQAudioOutput->bytesFree();
                if (BytesFree<= QBAAudioBufferOut.length()) {
                    int BytesOut= pQIODevice->write(reinterpret_cast<const char*>(QBAAudioBufferOut.data()), BytesFree);
                    if (BytesOut> 0) QBAAudioBufferOut.remove(0, BytesOut);
                } else {
                    int BytesOut= pQIODevice->write(reinterpret_cast<const char*>(QBAAudioBufferOut.data()), QBAAudioBufferOut.length());
                    if (BytesOut> 0) QBAAudioBufferOut.remove(0, BytesOut);
                }
            }
        #else
            if (pQIODevice && pQIODevice->isWritable()) {
                qint64 BytesOut= pQIODevice->write(QBAAudioBufferOut);
                if (BytesOut> 0) QBAAudioBufferOut.remove(0, BytesOut);
            }
        #endif
    }
}

void QFMainWindow::OnAudioType(int SampleRate, int ChannelCount) {
    #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
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
            pQIODevice= pQAudioOutput->start();
            ui->QLAudioType->setText("SampleRate: "+ QString::number(SampleRate)+ ", ChannelCount: "+ QString::number(ChannelCount));
        } else {
            UpdateLog(tr("Unsupported AudioFormat!!!"));
            ui->QLAudioType->setText("SampleRate: "+ QString::number(SampleRate)+ ", ChannelCount: "+ QString::number(ChannelCount)+ tr(", unsupported audio format!!!"));
        }
    #else
        if (pQAudioSink) {
            pQAudioSink->stop();
            delete pQAudioSink;
            pQAudioSink= nullptr;
        }
        QAudioFormat AudioFormat;
        AudioFormat.setChannelCount(ChannelCount);
        AudioFormat.setSampleFormat(QAudioFormat::Int16);
        AudioFormat.setSampleRate(SampleRate);
        QAudioDevice pQAudioDevice= QMediaDevices::defaultAudioOutput();
        if (!pQAudioDevice.isFormatSupported(AudioFormat)) {
            AudioFormat= pQAudioDevice.preferredFormat();
        }
        pQAudioSink= new QAudioSink(pQAudioDevice, AudioFormat);
        pQIODevice= pQAudioSink->start();
        pQAudioSink->setVolume(ui->QDSBVolume->value());
    #endif
}

void QFMainWindow::on_QCBRecord_toggled(bool checked) {
    if (pQThFFmpegPlayer) {
        if (checked) {
            pQThFFmpegPlayer->FileRenew(QDateTime::currentDateTime().toString("yyyyMMdd hh:mm:ss")+ ".mp4");
            QDTFileNew= QDateTime::currentDateTime();
        } else pQThFFmpegPlayer->FileRenew("");
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
    FrameCount++;
    if (ui->QPBStop->isEnabled()) {
        ui->QLImage->clear();
        Image= Image.scaled(QSize(Image.width() * ui->QDSBZoom->value(), Image.height() * ui->QDSBZoom->value()), Qt::KeepAspectRatio);
        ui->QLImage->setPixmap(QPixmap::fromImage((Image)));
    }
    ui->QLFrames->setText("Frames: "+ QString::number(FrameCount)+ ", key frames: "+ QString::number(KeyFrameCount));
}

void QFMainWindow::OnKeyFrame() {
    KeyFrameCount++;
}

void QFMainWindow::OnPacketRead(uint8_t *pBuffer, int pBufferSize, int *BytesIn) {
    QByteArray QBAByteIn= QFFileIn.read(pBufferSize);
    memcpy(pBuffer, QBAByteIn.data(), static_cast<ulong>(QBAByteIn.size()));
    *BytesIn= QBAByteIn.size();
    PachetCount++;
    //qDebug() << "PachetCount:" << PachetCount << "bytes:" << *BytesIn << "pBufferSize:" << pBufferSize;
}

void QFMainWindow::OnTimer() {
    if (ui->QCBRecord->isChecked() && pQThFFmpegPlayer) {
        if (QDTFileNew.isNull() || QDTFileNew.msecsTo(QDateTime::currentDateTime())> ui->QSBRecordLength->value() * 1000) {
            pQThFFmpegPlayer->FileRenew(QDateTime::currentDateTime().toString("yyyyMMdd hh:mm:ss")+ ".mp4");
            QDTFileNew= QDateTime::currentDateTime();
        }
    }
}

void QFMainWindow::UpdateLog(QString Log) {
    qDebug() << Log;
}
