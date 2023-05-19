#ifndef HMI_H
#define HMI_H

#include <QtConcurrentRun>
#include <QFutureWatcher>
#include <QMutex>
#include <QBitArray>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <modbus.h>
#include "bms.h"
#include "global.h"
#include "config.h"
#include "weiqianfunctions.h"

typedef struct _temp_humidity_meter_t_ {
    quint16 humidity;
    qint16 temperature;
} temp_humidity_meter_t;

typedef struct _dc_insulation_monitoring_device_t_ {
    quint16 bus_voltage;
    quint16 total_bus_resistance;
    quint16 positive_bus_resistance_to_ground;
    quint16 negative_bus_resistance_to_ground;
} dc_insulation_monitoring_device_t;

typedef struct _air_conditioner_t_ {
    quint16 working_status;
    quint16 reserve1;
    quint16 inside_fan_status;
    quint16 reserve2;
    quint16 outside_fan_status;
    quint16 reserve3;
    quint16 compressor_status;
    quint16 reserve4;
    qint16 cabinet_air_temp;
    quint16 reserve5;
    qint16 water_pump_temp;
    quint16 reserve6;
    qint16 out_cabinet_temp;
    quint16 reserve7;
    qint16 condenser_temp;
    quint16 reserve8;
    qint16 evaporator_temp;
    quint16 reserve9;
} air_conditioner_t;

typedef struct _electricity_meter_t_
{
    quint16 voltage;
} electricity_meter_t;

typedef struct _peripheral_t_ {
    temp_humidity_meter_t tempHumidityMeter[TEMP_HUMIDITY_METER_NUM];
    dc_insulation_monitoring_device_t dcInsulationMonitoringDevice[DC_INSULATION_MONITORING_DEVICE_NUM];
    air_conditioner_t airConditioner[AIR_CONDITIONER_NUM];
    electricity_meter_t electricityMeter[ELECTRICITY_METER_NUM];
} peripheral_t;

class HMI : public QObject
{
    Q_OBJECT

public:
    bool canOpened;

    bool isDhcp;
    char ip[20];
    char mask[20];
    char gateway[20];
    char dns[20];
    char macAddr[20];
    peripheral_t peripheral;
    modbus_t *modbus4IO;

public:
    HMI();
    ~HMI();
    void Beep();

    void OpenCan(int baudRate);
    int ReadCan(can_frame *frame);
    int WriteCan(can_frame *frame);
    void CloseCan();
    void RestartCan(int baudRate);

    bool SetWDog(int interval);
    bool StartWDog();
    bool FeedWDog();
    bool StopWDog();

    bool getNetWork(const char * iFace, bool *isDhcp,char *ip,char *subnetmask,char *gateway,char *dns,char *macAddr);
    bool setNetWork(const char * iFace, bool isDhcp,char *ip,char *subnetmask,char *gateway,char *dns);

    void getBacklightState(int *level,bool *autoClose,int *timeout);
    int getBacklightLevel();
    void setBacklightLevel(int level);
    void setBacklightAutoClose(bool autoClose,int timeout);
    void powerOffBackLight();

    int setIO(quint8 addr, bool open);
    int setIO(quint8 index, quint8 addr, quint8 nb, bool open);
    int getIO(bool input, quint8 addr, quint8 len, quint8 slave = 1);
    int getDataDI(quint8 *buf, int len);
    int getDataDO(quint8 *buf, int len);

    int getTempHumidity(int slave, int offset = 0);
    int getDcInsulation(int slave, int offset = 0);
    int getAirConditionerInfo(int slave, int offset = 0);
    int getElectricityMeter(int slave, int offset = 0);

signals:
    void writeDigitalOutput(int result);
    void readyReadDigiInput(quint8 addr, quint8 len, QBitArray bitArray);
    void readyReadDigiOutput(quint8 addr, quint8 len, QBitArray bitArray);
    void readyReadPeripheral(int type, int index, int result);

private:

#ifdef ARM_LINUX
    QMutex mutex4CanBus;
#endif

#ifdef WEIQIAN
    quint8 *data4Di;
    quint8 *data4Do;
    QMutex mutex4Modbus;

    int doWriteDigitalOutput(quint8 addr, bool open);
    int doWriteDigitalOutput(quint8 index, quint8 addr, quint8 nb, bool open);
    int doReadDigitalInputOutput(bool input, quint8 addr, quint8 len, quint8 slave);
    int doReadTempHumidity(int slave, int offset);
    int doReadDcInsulation(int slave, int offset);
    int doReadAirConditioner(int slave, int offset);
    int doReadElectricityMeter(int slave, int offset);
#endif
};

#endif // HMI_H



