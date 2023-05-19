#ifndef WEIQIANFUNCTIONS_H
#define WEIQIANFUNCTIONS_H

#include <QLibrary>
#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QStringList>
#include <QHostAddress>
#include <QNetworkInterface>
#include <qmath.h>

class WeiqianFunctions {

public:
    WeiqianFunctions();

    static int OpenCan(int baudrate);
    static void CloseCan();
    static int CanWrite(int canId, const char *sendData,int dateLen);
    static int CanRead(unsigned int * can_id, int *len, char *data);

    static int OpenSerialPort(char *path, int baud, int databits, char *parity, int stopbits);
    static void CloseSerialPort(int fd);
    static int ReadSerialPort(int fd, char *buff, int count);
    static int WriteSerialPort(int fd, char *buff, int count);

    static void Beep();
    static void SetBackLight(int flag);
    static void SetBackLightLevel(int level);
    static int GetBacklightLevel();

    static void SetBacklightBrightness(int brightness);         // 0 <= brightnes <= 100
    static int GetBacklightBrightness();

    static bool StarWatchDog();
    static bool StopWatchDog();
    static bool SetWatchDog(int timeout);
    static int GetWatchDog();
    static bool FeedWatchDog();
    static void WatchDogEnable(int flag);

    static bool SetNetWorkCfg(const char * iFace, bool isDhcp,char *ip,char *subnetmask,char *gateway,char *dns);
    static bool GetNetWorkCfg(const char * iFace, bool *isDhcp,char *ip,char *subnetmask,char *gateway,char *dns,char *macAddr);
    static int SetMacAddress(const char *macAddress);

    static bool setIO(unsigned char level, unsigned char ioNum);

private:
    static QLibrary *m_pWeiqianLib;

    static void load();
    static void InitCan(int baudrate);

};

#endif // WEIQIANFUNCTIONS_H
