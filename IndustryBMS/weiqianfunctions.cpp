#include "weiqianfunctions.h"

QLibrary *WeiqianFunctions::m_pWeiqianLib = new QLibrary;

WeiqianFunctions::WeiqianFunctions()
{
    load();
}

void WeiqianFunctions::InitCan(int baudrate)
{
    load();//
    typedef void (*Fun) (int);
     Fun fun = (Fun)m_pWeiqianLib->resolve("InitCan");
     if(fun) {
         fun(baudrate);
     }
}

int WeiqianFunctions::OpenCan(int baudrate)
{
#if defined(WEIQIAN)
    typedef int (*Fun) ();
    InitCan(baudrate);
     Fun fun = (Fun)m_pWeiqianLib->resolve("OpenCan");
     if(fun) {
         return fun();
     }
     return -1;
#elif defined(YCTEK)
    load();
    typedef void (*Fun) (int);
    Fun fun = (Fun)m_pWeiqianLib->resolve("OpenCan");
    if(fun) {
        fun(baudrate);
    }
    qDebug() << "can opened.";
    return 0;
#else
    return -1;
#endif

}

void WeiqianFunctions::CloseCan()
{
    load();
    typedef void (*Fun) ();
     Fun fun = (Fun)m_pWeiqianLib->resolve("CloseCan");
     if(fun) {
         fun();
     }
}

int WeiqianFunctions::CanWrite(int canId, const char *sendData,int dateLen)
{

#if defined(WEIQIAN)
    typedef int (*Fun) (int, const char *,int);
    Fun fun = (Fun)m_pWeiqianLib->resolve("CanWrite");
    if(fun) {
//        qDebug() << "found";
        return fun(canId, sendData, dateLen);
    }
    return -1;
#elif defined(YCTEK)
    unsigned char data[8];
    memset(data, 0x00, 8);
    memcpy(data, sendData, sizeof(data));
    load();
    typedef void (*Fun) (int, unsigned char *, unsigned char);
    Fun fun = (Fun)m_pWeiqianLib->resolve("WriteCan");
    if (fun) {
        fun(canId, data, dateLen);
        return 0;
    } else {
        qDebug() << "fun(WriteCan) is NULL";
    }
    return -1;
#else
    return -1;
#endif
}

int WeiqianFunctions::CanRead(unsigned int * can_id, int *len, char *data)
{
    load();

#if defined(WEIQIAN)
    typedef int (*Fun) (int *, char *);
    Fun fun = (Fun)m_pWeiqianLib->resolve("CanRead");
    if(fun) {
        *can_id = fun(len, data);
        return *can_id;
    }
    return -1;
#elif defined(YCTEK)
    typedef int (*Fun) (unsigned int * , unsigned char * , unsigned char *);
    Fun fun = (Fun)m_pWeiqianLib->resolve("ReadCan");
    if(fun) {
        return fun(can_id, (unsigned char *)datalenght, (unsigned char *)data);
    }
    return -1;
#else
    return -1;
#endif
}

int WeiqianFunctions::OpenSerialPort(char *path, int baud, int databits, char *parity, int stopbits)
{
    load();
    typedef int (*Fun) (char *, int, int, char *, int);
     Fun fun = (Fun)m_pWeiqianLib->resolve("OpenSerialPort");
     if(fun) {
         return fun(path, baud, databits, parity, stopbits);
     }
     return -1;
}

void WeiqianFunctions::CloseSerialPort(int fd)
{
    load();
    typedef void (*Fun) (int);
     Fun fun = (Fun)m_pWeiqianLib->resolve("CloseSerialPort");
     if(fun) {
         fun(fd);
     }
}

int WeiqianFunctions::ReadSerialPort(int fd, char *buff, int count)
{
    load();
    typedef int (*Fun) (int, char *, int);
     Fun fun = (Fun)m_pWeiqianLib->resolve("ReadSerialPort");
     if(fun) {
         return fun(fd, buff, count);
     }
     return -1;
}

int WeiqianFunctions::WriteSerialPort(int fd, char *buff, int count)
{
    load();
    typedef int (*Fun) (int, char *, int);
     Fun fun = (Fun)m_pWeiqianLib->resolve("WriteSerialPort");
     if(fun) {
         return fun(fd, buff, count);
     }
     return -1;
}

void WeiqianFunctions::Beep()
{
    load();
    typedef void (*Fun) ();
     Fun fun = (Fun)m_pWeiqianLib->resolve("Beep");
     if(fun) {
         fun();
     }
}

void WeiqianFunctions::SetBackLight(int flag)
{
    load();
    typedef void (*Fun) (int);
     Fun fun = (Fun)m_pWeiqianLib->resolve("SetBackLight");
     if(fun) {
         fun(flag);
     }
}

void WeiqianFunctions::SetBackLightLevel(int level)
{
#ifdef YCTEK
    typedef void (*Fun) (int);
    Fun fun = (Fun)m_pWeiqianLib->resolve("SetDefaultBackLightLevel");
    if(fun) {
        fun(level);
    }
#else
    Q_UNUSED(level)

    return;
#endif
}

int WeiqianFunctions::GetBacklightLevel()
{
#ifdef YCTEK
    typedef int (*Fun) ();
    Fun fun = (Fun)m_pWeiqianLib->resolve("GetDefaultBackLightLevel");
    if(fun) {
        return fun();
    }
#else
    return -1;
#endif
}

void WeiqianFunctions::SetBacklightBrightness(int brightness)
{
#ifdef WEIQIAN
    QFile *fp = new QFile("/etc/init.d/backlight_sh");
    QString command = QString("echo 100,%1 > /sys/devices/platform/pwm/pwm.0").arg(brightness);

    if (NULL == fp || 0 > brightness || 100 < brightness) {
        return;
    }

    if (0 == brightness) {
        system(qPrintable(command));
        return;
    }

    if (fp->open(QFile::WriteOnly | QFile::Text)) {
        if (0 < fp->write(qPrintable("#!/bin/sh\n" + command))) {
            fp->flush();
            system(qPrintable(command));
        } else {
            qDebug() << __func__ << fp->fileName() << "file write error.";
        }

        fp->close();
    } else {
        qDebug() << __func__ << fp->fileName() << "file open failed.";
    }

    delete fp;
    return;
#else
    return;
#endif
}

int WeiqianFunctions::GetBacklightBrightness()
{
#ifdef WEIQIAN
    bool ok = false;
    int value = 0;
    QFile *fp = NULL;
    QString brightness;
    QTextStream *textStream;

    fp = new QFile("/sys/devices/platform/pwm/pwm.0");

    if(NULL == fp)
    {
        return -1;
    }

    if(fp->open(QFile::ReadOnly))
    {
        textStream = new QTextStream(fp);
        brightness = textStream->readLine();
        brightness = brightness.section('%', 0, 0);
        brightness = brightness.section(',', 1, 1);
        value = brightness.toInt(&ok);
        fp->close();
        delete fp;

        if(ok)
        {
            return value;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }
#else
    return -1;
#endif
}

bool WeiqianFunctions::StarWatchDog()
{
    load();

#if defined(WEIQIAN)
    typedef void (*Fun)();
     Fun fun = (Fun)m_pWeiqianLib->resolve("StarWatchDog");
     if (fun) {
         fun();
         return true;
     } else {
         return false;
     }
#elif defined(YCTEK)
    typedef int (*Fun)();
    Fun fun = (Fun)m_pWeiqianLib->resolve("StartWDog");
    if (fun) {
        int ret = fun();
        qDebug() << "StartWatchDog" << ret;
        return 0 < ret;
    }
#else
    return false;
#endif
}

bool WeiqianFunctions::StopWatchDog()
{
    load();

#if defined(WEIQIAN)
    typedef void (*Fun)();
     Fun fun = (Fun)m_pWeiqianLib->resolve("StopWatchDog");
     if (fun) {
         fun();
		 return true;
     } else {
         return false;
     }
#elif defined(YCTEK)
    typedef int (*Fun)();
    Fun fun = (Fun)m_pWeiqianLib->resolve("StopWDog");
    if (fun) {
        int ret = fun();
        qDebug() << "StopWatchDog" << ret;
        return 0 < ret;
    }
#else
    return false;
#endif
}

bool WeiqianFunctions::SetWatchDog(int timeout)
{
    load();

#if defined(WEIQIAN)
    typedef void (*Fun) (int);
     Fun fun = (Fun)m_pWeiqianLib->resolve("SetWatchDog");
     if(fun) {
         fun(timeout);
         return true;
     } else {
         return false;
     }
#elif defined(YCTEK)
    typedef int (*Fun)();
    Fun fun = (Fun)m_pWeiqianLib->resolve("SetWDog");
    if (fun) {
        int ret = fun();
        qDebug() << "SetWatchDog" << ret;
        return 0 < ret;
    }
#else
    return false;
#endif
}

int WeiqianFunctions::GetWatchDog()
{
    load();

#if defined(WEIQIAN)
    typedef int (*Fun) ();
     Fun fun = (Fun)m_pWeiqianLib->resolve("GetWatchDog");
     if(fun) {
         return fun();
     }
     return -1;
#endif
}

bool WeiqianFunctions::FeedWatchDog()
{
    load();

#if defined(WEIQIAN)
    typedef void (*Fun) ();
     Fun fun = (Fun)m_pWeiqianLib->resolve("FeedWatchDog");
     if (fun) {
         fun();
         return true;
     } else {
         return false;
     }
     return false;
#elif defined(YCTEK)
    typedef int (*Fun) ();
    Fun fun = (Fun)m_pWeiqianLib->resolve("FeedWDog");
    if(fun)
    {
        int ret = fun();
        qDebug() << "FeedWatchDog" << ret;
        return 0 < ret;
    }
    return false;
#else
    return false;
#endif
}

void WeiqianFunctions::WatchDogEnable(int flag)
{
    load();
    typedef void (*Fun) (int);
     Fun fun = (Fun)m_pWeiqianLib->resolve("WatchDogEnable");
     if(fun) {
         fun(flag);
     }
}

bool WeiqianFunctions::SetNetWorkCfg(const char * iFace, bool isDhcp, char *ip, char *subnetmask, char *gateway, char *dns)
{
    // this version do NOT supoort dhcp, dhcp will be ready at next version
#if defined(WEIQIAN)
    Q_UNUSED(isDhcp)

    bool ret = false;
    QStringList args;
    QProcess process;
    QHostAddress ipTester;
    QString fileName = QString("/etc/init.d/ifconfig_%1").arg(iFace);
    QFile *fp = new QFile(fileName);
    QString command;
    QList<QNetworkInterface> list = QNetworkInterface::allInterfaces();

    // use reference in foreach with non-trivial type
    foreach (const QNetworkInterface & networkInterface, list) {
        if (!networkInterface.name().startsWith("eth")) {
            continue;
        }

        if (networkInterface.name().contains(iFace)) {
            command.prepend(QString("route add default gw %1\n").arg(gateway));
            command.prepend(QString("ifconfig %1 %2 netmask %3\n").arg(networkInterface.name()).arg(ip).arg(subnetmask));
            continue;
        }

        command += QString("ifconfig %1 down\n").arg(networkInterface.name());
        command += QString("ifconfig %1 up\n").arg(networkInterface.name());
    }

    command.prepend("#!/bin/sh\n");
    if (ipTester.setAddress(ip) && ipTester.setAddress(subnetmask) && ipTester.setAddress(gateway)) {
        if (NULL == fp) {
            return ret;
        }

        if (fp->open(QFile::WriteOnly | QFile::Text)) {
            if (0 < fp->write(qPrintable(command))) {
                fp->flush();
                fp->close();
                ret = true;
            } else {
                fp->close();
            }
        }

        delete fp;
        fp = NULL;
        if (ret) {
            ret = false;
            fp = new QFile("/etc/resolv.conf");
            command = QString("nameserver %1\n").arg(dns);
            if (NULL != fp) {
                if (fp->open(QFile::WriteOnly | QFile::Text)) {
                    if (0 < fp->write(command.toLatin1())) {
                        fp->flush();
                        args << fileName;
                        process.start("sh", args);
                        ret = process.waitForFinished();
                        process.close();
                    }

                    fp->close();
                }

                delete fp;
            }
        }
    }

    return ret;
#elif defined(YCTEK)
    typedef int (*Fun) (int, int ,char *, char *, char*, char*);
    Fun fun = (Fun)m_pWeiqianLib->resolve("SetNetWork");
    if(fun) {
        return fun(0, isDhcp, ip, subnetmask, gateway, dns);
    }
    return -1;
#else
    return false;
#endif
}

bool WeiqianFunctions::GetNetWorkCfg(const char * iFace, bool *isDhcp,char *ip,char *subnetmask,char *gateway,char *dns,char *macAddr)
{
#if defined(WEIQIAN)
    Q_UNUSED(macAddr)

    QFile *fp = NULL;
    QString line = "";
    QStringList words;
    QHostAddress ipTester;

    *isDhcp = false;
    fp = new QFile(QString("/etc/init.d/ifconfig_%1").arg(iFace));

    if(NULL == fp)
    {
        return false;
    }

    if(fp->open(QFile::ReadOnly))
    {
        QTextStream in(fp);

        do {
            if(line.isEmpty())
            {
                line = in.readLine(255);
                continue;
            }

            words = line.split(' ');

            if(0 == words.at(0).compare("ifconfig") && 5 <= words.size())       // like "ifconfig eth0 10.2.100.209 netmask 255.255.255.0"
            {
                if(ipTester.setAddress(words.at(2)))
                {
                    qstrncpy(ip, qPrintable(words.at(2)), words.at(2).size() + 1);
                }

                if(ipTester.setAddress(words.at(4)))
                {
                    qstrncpy(subnetmask, qPrintable(words.at(4)), words.at(4).size() + 1);
                }
            }
            else if(0 == words.at(0).compare("route") && 5 <= words.size())     // like "route add default gw 10.2.100.1"
            {
                if(ipTester.setAddress(words.at(4)))
                {
                    qstrncpy(gateway, qPrintable(words.at(4)), words.at(4).size() + 1);
                }
            }

            line = in.readLine(255);
        } while (!line.isNull());

        fp->close();
        delete fp;

        line.clear();
        fp = new QFile("/etc/resolv.conf");

        if(fp->open(QFile::ReadOnly))
        {
            QTextStream in(fp);

            do {
                if(line.isEmpty())
                {
                    continue;
                }

                words = line.split(' ', QString::SkipEmptyParts);

                if(0 == words.at(0).compare("nameserver") && 2 <= words.size())      // like "nameserver  223.5.5.5"
                {
                    if(ipTester.setAddress(words.at(1)))
                    {
                        qstrncpy(dns, qPrintable(words.at(1)), words.at(1).size() + 1);
                    }
                }
            }
            while (!((line = in.readLine(255)).isNull()));

            fp->close();
            delete fp;
        }
        else
        {
            delete fp;

            return false;
        }

        return true;
    }
    else
    {
        delete fp;

        return false;
    }
#elif defined(YCTEK)
    load();
    typedef int (*Fun) (int, int *, char *, char *, char *, char *, char *);
    int dhcp = 0;
    Fun fun = (Fun)m_pWeiqianLib->resolve("GetNetWorkCfg");
    if(fun) {
        qDebug() << "found function GetNetWorkCfg().";
        int ret = 0 == fun(0, &dhcp, ip, subnetmask, gateway, dns, macAddr);
        *isDhcp = dhcp;
        return ret;
    }
    return false;
#elif defined(WZD)
    QFile *fp = NULL;
    QString line = "";
    QStringList words;

    fp = new QFile("/etc/network/interfaces");

    if(fp->open(QFile::ReadOnly))
    {
        do {
            if(line.isEmpty())
            {
                continue;
            }

            words = line.split(' ');

            if(words.contains("address"))
            {
                strncpy(ip, qPrintable(words.at(1)), words.at(1).size() + 1);
            }
            else if(words.contains("netmask"))
            {
                strncpy(subnetmask, qPrintable(words.at(1)), words.at(1).size() + 1);
            }
            else if(words.contains("gateway"))
            {
                strncpy(gateway, qPrintable(words.at(1)), words.at(1).size() + 1);
            }
            else if(words.contains("dns-nameserver"))
            {
                strncpy(dns, qPrintable(words.at(1)), words.at(1).size() + 1);
            }

            line = fp->readLine(255);
        } while (!line.isNull());

        fp->close();
        delete fp;

        return true;
    }
    else
    {
        delete fp;

        return false;
    }
#else
    return false;
#endif
}

int WeiqianFunctions::SetMacAddress(const char *macAddress)
{
    load();
    typedef int (*Fun) (const char *);
     Fun fun = (Fun)m_pWeiqianLib->resolve("SetMacAddress");
     if(fun) {
         return fun(macAddress);
     }
     return -1;
}

bool WeiqianFunctions::setIO(unsigned char level, unsigned char ioNum)
{
    load();

#if defined(WEIQIAN)
    Q_UNUSED(level)
    Q_UNUSED(ioNum)

    return false;
#elif defined(YCTEK)
    typedef bool (*Fun) (unsigned char, unsigned char);
    Fun fun = (Fun)m_pWeiqianLib->resolve("SetIO");
    if(fun) {
        return fun(level, ioNum);
    }
    return false;
#else
    m_pWeiqianLib = new QLibrary();
#endif
    return false;

}

void WeiqianFunctions::load()
{
    if(!m_pWeiqianLib->isLoaded()) {

#if defined(WEIQIAN)
        m_pWeiqianLib = new QLibrary("/lib/libWeiqianHardware.so");
#elif defined(YCTEK)
        m_pWeiqianLib = new QLibrary("/lib/libycapic.so");
#else
        m_pWeiqianLib = new QLibrary();
#endif
        bool ret = m_pWeiqianLib->load();
        qDebug() << "lib load:" << ret;
    }
}

