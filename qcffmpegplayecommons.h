#ifndef QCFFMPEGPLAYECOMMONS_H
#define QCFFMPEGPLAYECOMMONS_H

#include "QFileInfo"
#include "QString"

enum ConnectionStates {
    CONNECTION_STATE_CONNECTED,
    CONNECTION_STATE_CONNECTED_DATA_RECEIVED,
    CONNECTION_STATE_CONNECTING,
    CONNECTION_STATE_IDLE,
};

struct Interface {
    int index;
    QString Name, Version, Path;
};

bool InterfacesList(QVector<Interface> &QVInterfaces);

#endif // QCFFMPEGPLAYECOMMONS_H
