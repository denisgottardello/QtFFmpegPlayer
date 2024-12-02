#ifndef QCFFMPEGPLAYECOMMONS_H
#define QCFFMPEGPLAYECOMMONS_H

enum ConnectionStates {
    CONNECTION_STATE_CONNECTED,
    CONNECTION_STATE_CONNECTED_DATA_RECEIVED,
    CONNECTION_STATE_CONNECTING,
    CONNECTION_STATE_IDLE,
};

enum RTSPTransports {
    RTSP_TRANSPORT_AUTO= 0,
    RTSP_TRANSPORT_HTTP= 1,
    RTSP_TRANSPORT_TCP= 2,
    RTSP_TRANSPORT_UDP= 3,
    RTSP_TRANSPORT_UDP_MULTICAST= 4,
};

#endif // QCFFMPEGPLAYECOMMONS_H
