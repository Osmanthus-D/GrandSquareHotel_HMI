#ifndef EXAMPLEKBDHANDLER_H
#define EXAMPLEKBDHANDLER_H

#include <QSocketNotifier>
#include <QWSServer>
#include <QDebug>
#include <QFile>
//#include <QtSerialPort/QSerialPort>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define USE_STACKOVERFLOW

class ExampleKbdHandler : public QObject
{
    Q_OBJECT
public:
//    explicit ExampleKbdHandler(const QString &device = QString("/dev/input/mice"))
    explicit ExampleKbdHandler(const QString &device = QString("/dev/input/event1"))
    {
#ifdef USE_STACKOVERFLOW
//        qDebug() << "Loaded Example keyboard plugin!";
//        setObjectName("Example Keypad Handler");
        kbdFd = ::open(device.toLocal8Bit().constData(), O_RDONLY, 0);
        if (kbdFd >= 0) {
//            qLog(Input) << "Opened" << device << "as keyboard input";
            m_notify = new QSocketNotifier(kbdFd, QSocketNotifier::Read, this);
            connect(m_notify, SIGNAL(activated(int)), this, SLOT(readKbdData()));
        } else {
            qWarning("Cannot open %s for keyboard input (%s)",
                     device.toLocal8Bit().constData(), strerror(errno));
            return;
        }
        shift = false;
#else
        QSerialPort *dev = new QSerialPort(device);
        bool isOpened = dev->open(QFile::ReadOnly);
        qDebug() << __func__ << "*** is dev opned" << isOpened;
        connect(dev, SIGNAL(readyRead()), this, SLOT(readKbdData()));
#endif
    }
    
    ~ExampleKbdHandler()
    {
        if (kbdFd >= 0)
            ::close(kbdFd);
    }

private:
    QSocketNotifier *m_notify;
    int  kbdFd;
    bool shift;

private slots:
    void readKbdData()
    {
#ifdef USE_STACKOVERFLOW
        unsigned char buf[256] = {0x00};
        int len = ::read(kbdFd, buf, sizeof(buf));

        if (0 < len) {
            qDebug() << "*** mouse event detect";
            QWSServer::setCursorVisible(false);
        } else {
            qDebug() << "*** len:" << len;
        }
#else
        qDebug() << "*** mouse event detect";
#endif
    }
};

#endif // EXAMPLEKBDHANDLER_H
