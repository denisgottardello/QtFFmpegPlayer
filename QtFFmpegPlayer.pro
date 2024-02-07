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
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

unix:!mac {
  LIBS += -Wl,-rpath=\\\$$ORIGIN/libs
}

ANDROID_ABIS = arm64-v8a

unix {
    unix:!mac {
        INCLUDEPATH += ../../ffmpeg-6.1.1/
        !android {
            LIBS += ../../ffmpeg-6.1.1/libs/libavdevice.so.60
            LIBS += ../../ffmpeg-6.1.1/libs/libavfilter.so.9
            LIBS += ../../ffmpeg-6.1.1/libs/libavformat.so.60
            LIBS += ../../ffmpeg-6.1.1/libs/libavcodec.so.60
            LIBS += ../../ffmpeg-6.1.1/libs/libpostproc.so.57
            LIBS += ../../ffmpeg-6.1.1/libs/libswresample.so.4
            LIBS += ../../ffmpeg-6.1.1/libs/libswscale.so.7
            LIBS += ../../ffmpeg-6.1.1/libs/libavutil.so.58
        }
        android {
            LIBS += ../../ffmpeg-6.1.1/libs/arm64-v8a/libavdevice.so
            LIBS += ../../ffmpeg-6.1.1/libs/arm64-v8a/libavfilter.so
            LIBS += ../../ffmpeg-6.1.1/libs/arm64-v8a/libavformat.so
            LIBS += ../../ffmpeg-6.1.1/libs/arm64-v8a/libavcodec.so
            LIBS += ../../ffmpeg-6.1.1/libs/arm64-v8a/libswresample.so
            LIBS += ../../ffmpeg-6.1.1/libs/arm64-v8a/libswscale.so
            LIBS += ../../ffmpeg-6.1.1/libs/arm64-v8a/libavutil.so
            ANDROID_EXTRA_LIBS = \
            /home/denis/Cpp/ffmpeg-6.1.1/libs/arm64-v8a/libavdevice.so \
            /home/denis/Cpp/ffmpeg-6.1.1/libs/arm64-v8a/libavfilter.so \
            /home/denis/Cpp/ffmpeg-6.1.1/libs/arm64-v8a/libavformat.so \
            /home/denis/Cpp/ffmpeg-6.1.1/libs/arm64-v8a/libavcodec.so \
            /home/denis/Cpp/ffmpeg-6.1.1/libs/arm64-v8a/libswresample.so \
            /home/denis/Cpp/ffmpeg-6.1.1/libs/arm64-v8a/libswscale.so \
            /home/denis/Cpp/ffmpeg-6.1.1/libs/arm64-v8a/libavutil.so \
        }
    }
    unix:mac {
        INCLUDEPATH += ../../ffmpeg-6.0/
        LIBS += ../../ffmpeg-6.0/libavdevice/libavdevice.60.dylib
        LIBS += ../../ffmpeg-6.0/libavfilter/libavfilter.9.dylib
        LIBS += ../../ffmpeg-6.0/libavformat/libavformat.60.dylib
        LIBS += ../../ffmpeg-6.0/libavcodec/libavcodec.60.dylib
        LIBS += ../../ffmpeg-6.0/libpostproc/libpostproc.57.dylib
        LIBS += ../../ffmpeg-6.0/libswresample/libswresample.4.dylib
        LIBS += ../../ffmpeg-6.0/libswscale/libswscale.7.dylib
        LIBS += ../../ffmpeg-6.0/libavutil/libavutil.58.dylib
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
        qfmainwindow.cpp \
    qiffmpegplayerinterface.cpp \
    qthffmpegplayer.cpp

HEADERS  += qfmainwindow.h \
    qiffmpegplayerinterface.h \
    qthffmpegplayer.h

FORMS    += qfmainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
