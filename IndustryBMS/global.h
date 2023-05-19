#ifndef GLOBAL_H
#define GLOBAL_H

#include <qglobal.h>

#ifdef WEIQIAN
#define COM1                                                ("/dev/ttyAMA0")  // db9/rs232
#define COM2                                                ("/dev/ttyAMA2")  // terminal/rs232
#define COM3                                                ("/dev/ttyAMA3")  // terminal/rs485
#endif

#define ETH0                                                ("eth0")
#define ETH1                                                ("eth1")

#define PREV                                                0
#define NEXT                                                1

enum ExtMsgType {
    EmsMsg = QtFatalMsg + 1,
    PcsMsg,
    PerMsg
};

inline QDebug qDebugEms() { return QDebug((QtMsgType)EmsMsg); }

inline QDebug qDebugPcs() { return QDebug((QtMsgType)PcsMsg); }

inline QDebug qDebugPer() { return QDebug((QtMsgType)PerMsg); }

/**
 * @brief   convert port number to device file node
 * @param   port number
 * @return  device file node
 */
inline QString port2Dev(int port)
{
    QString device;

    switch(port) {
    case 1:
        device = COM1;
        break;
    case 2:
        device = COM2;
        break;
    case 3:
        device = COM3;
        break;
    default:
        break;
    }

    return device;
}

#endif // GLOBAL_H
