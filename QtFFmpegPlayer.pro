#-------------------------------------------------
#
# Project created by QtCreator 2011-08-25T10:35:41
#
#-------------------------------------------------

QT       += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x061000    # disables all the APIs deprecated before Qt 6.0.0

unix:!mac {
  LIBS += -Wl,-rpath=\\\$$ORIGIN/libs
}

ANDROID_ABIS = arm64-v8a

unix {
    unix:!mac {
        INCLUDEPATH += ../../ffmpeg-7.1/
        !android {
            LIBS += ../../ffmpeg-7.1/libs/libavdevice.so.61
            LIBS += ../../ffmpeg-7.1/libs/libavfilter.so.10
            LIBS += ../../ffmpeg-7.1/libs/libavformat.so.61
            LIBS += ../../ffmpeg-7.1/libs/libavcodec.so.61
            LIBS += ../../ffmpeg-7.1/libs/libpostproc.so.58
            LIBS += ../../ffmpeg-7.1/libs/libswresample.so.5
            LIBS += ../../ffmpeg-7.1/libs/libswscale.so.8
            LIBS += ../../ffmpeg-7.1/libs/libavutil.so.59
        }
        android {
            LIBS += ../../ffmpeg-7.1/libs/arm64-v8a/libavdevice.so
            LIBS += ../../ffmpeg-7.1/libs/arm64-v8a/libavfilter.so
            LIBS += ../../ffmpeg-7.1/libs/arm64-v8a/libavformat.so
            LIBS += ../../ffmpeg-7.1/libs/arm64-v8a/libavcodec.so
            LIBS += ../../ffmpeg-7.1/libs/arm64-v8a/libswresample.so
            LIBS += ../../ffmpeg-7.1/libs/arm64-v8a/libswscale.so
            LIBS += ../../ffmpeg-7.1/libs/arm64-v8a/libavutil.so
            ANDROID_EXTRA_LIBS = \
            /home/denis/Cpp/ffmpeg-7.1/libs/arm64-v8a/libavdevice.so \
            /home/denis/Cpp/ffmpeg-7.1/libs/arm64-v8a/libavfilter.so \
            /home/denis/Cpp/ffmpeg-7.1/libs/arm64-v8a/libavformat.so \
            /home/denis/Cpp/ffmpeg-7.1/libs/arm64-v8a/libavcodec.so \
            /home/denis/Cpp/ffmpeg-7.1/libs/arm64-v8a/libswresample.so \
            /home/denis/Cpp/ffmpeg-7.1/libs/arm64-v8a/libswscale.so \
            /home/denis/Cpp/ffmpeg-7.1/libs/arm64-v8a/libavutil.so \
        }
    }
    unix:mac {
        INCLUDEPATH += ../../ffmpeg-7.1/
        LIBS += ../../ffmpeg-7.1/libavdevice/libavdevice.61.dylib
        LIBS += ../../ffmpeg-7.1/libavfilter/libavfilter.10.dylib
        LIBS += ../../ffmpeg-7.1/libavformat/libavformat.61.dylib
        LIBS += ../../ffmpeg-7.1/libavcodec/libavcodec.61.dylib
        LIBS += ../../ffmpeg-7.1/libpostproc/libpostproc.58.dylib
        LIBS += ../../ffmpeg-7.1/libswresample/libswresample.5.dylib
        LIBS += ../../ffmpeg-7.1/libswscale/libswscale.8.dylib
        LIBS += ../../ffmpeg-7.1/libavutil/libavutil.59.dylib
    }
}
windows {
    INCLUDEPATH += ../../ffmpeg-6.0/include
    LIBS += ../../ffmpeg-6.0/lib/avcodec.lib
    LIBS += ../../ffmpeg-6.0/lib/avdevice.lib
    LIBS += ../../ffmpeg-6.0/lib/swresample.lib
    LIBS += ../../ffmpeg-6.0/lib/avfilter.lib
    LIBS += ../../ffmpeg-6.0/lib/avformat.lib
    LIBS += ../../ffmpeg-6.0/lib/avutil.lib
    LIBS += ../../ffmpeg-6.0/lib/postproc.lib
    LIBS += ../../ffmpeg-6.0/lib/swscale.lib
}

SOURCES += main.cpp\
    qcffmpegplayecommons.cpp \
        qfmainwindow.cpp \
    qiffmpegplayerinterface.cpp \
    qthffmpegplayer.cpp

HEADERS  += qfmainwindow.h \
    qcffmpegplayecommons.h \
    qiffmpegplayerinterface.h \
    qthffmpegplayer.h

FORMS    += qfmainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
