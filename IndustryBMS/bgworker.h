#ifndef BGWORKER_H
#define BGWORKER_H
#include <QThread>
#include <QObject>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QTextCodec>
#include <QDateTime>
#include <unistd.h>
#include <stdint.h>
#include <base.h>
#include "MCU801A.h"
#include "comm.h"

class MainWindow;
class BgWorker : public QObject, Base
{
    Q_OBJECT
public:
    BgWorker(MainWindow *mainwindow, int moduleNum = DEFAULT_MODULE_NUM, int interval = 0);
    QThread thread;
    QDir dir_SD;
    QDir dir;
    QFile file;

signals:
    void processDone_Signal();
    void updatingMCU_Signal(int);
    void response_Signal(int);

public slots:
    void stopThread();
    void readCan();
    void readLog();
    void read_log(QFile& log,QFile& logBak,QByteArray *text);
    void saveData();
    void send2BMS();
    void send2PCS();
    void send2EMS();
    void feedBack2BMS();
    void pcs();
    /*void checkWarning();*/

    void modbusDataMap();
    void modbusTcpServer();
    void copyData(QString);
    void updateMCU(QString, int);
    void doQueryPeripheral();
    void runIEC104Slave();

private:
    MainWindow *mw;
    int period;         // unit: ms
    bool isStop;

    int faultBit;

    bool checkDisk();
    void clearDisk();
    void pcsDataProc(int id);
    void processData();
    void checkFault();
    bool isMaxAllowedCurrentZero(unsigned m, bool isCharging);
    void fdbk2BMSOne(bool *hasChrgZero
                     , bool *hasDischrgZero
                     , bool *hasChrgCurrent
                     , bool *hasDischrgCurrent);
    bool groupHasCurrent(unsigned id, bool isCharging);

    int switchFileByDate(QDate date);
    int getBit(uint16_t data,uint8_t bit);
    void writeCan(uint8_t *data,int len,int dest);
    void initAutoRecovFilter(QList<int> *filter);
    void initCriticalAutoRecovFilter(QList<int> *filter);
    int calcBatteryExtremum(int n);
    void calcPileSoc(MODBUS::PCS & pile, const QList<quint16> & socList);
    void handlePileSoc(MODBUS::PCS & pile);
    QString removeOldestDataDir();
    QString removeOldestDataFile();
    void clearDiskIfNeed();
    static bool checkCount(bool isCount, unsigned & curCount, unsigned maxCount);
    static quint16 crc16(const void *s, int n);
};

#endif // BGWORKER_H
