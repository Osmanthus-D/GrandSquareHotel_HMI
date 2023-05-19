#ifndef MODULE_H
#define MODULE_H

#include <qglobal.h>
#include <stdint.h>
#include "bms.h"

#define Q_MAX 1658

typedef struct Pack_t
{
    quint8 balanceBat;
    quint8 balanceState;
    quint8 balanceMode;
} Pack;

typedef struct Module_t
{
    bool heartbeat;
    bool heartbeat_bak;

    quint16 BCU_swVer;
    quint16 BCU_hwVer;
    quint16 BMU_swVer;
    quint16 BMU_hwVer;

    quint16 minV;
    quint16 maxV;
    qint16 minT;
    qint16 maxT;

    quint8 minVPackID;
    quint8 maxVPackID;
    quint8 minTPackID;
    quint8 maxTPackID;

    quint8 minVID;
    quint8 maxVID;
    quint8 minTID;
    quint8 maxTID;

    quint8 chargeState;
    quint8 chargeState_bak;

    quint16 moduleSoc;
    quint16 moduleSoh;
    quint16 moduleSoe;
    quint8 mainRelay;
    quint8 totalNegativeRelay;
    quint8 preChargeRelay;
    quint16 moduleVoltage;
    qint16 moduleCurrent;
    quint16 insulationRes;
    quint8 forceMode;
    quint32 fanState;
    quint16 afterPrechargeVoltage;
    quint16 tempRiseRate;
    quint16 chargingCapacityOnce;
    quint16 dischargingCapacityOnce;
    quint32 chargingCapacity;
    quint32 dischargingCapacity;
    bool breaker;
    bool fuseWire;
    bool buzzer;
    bool selfChecking;

    quint16 realCapacity;
    quint16 realSOC;
    quint16 residualCapacity;
    quint16 nominalCapacity;

    quint8 chargeEnable;
    quint8 dischargeEnable;
    quint8 chargeCurrentMax;
    quint8 dischargeCurrentMax;
    quint8 chargePowerMax;
    quint8 dischargePowerMax;

    short voltage[MAX_PACK_NUM*BAT_NUM];
    short temp[MAX_PACK_NUM*TEMP_NUM];

    quint32 chargeableQ;
    quint32 remainQ;

    Pack pack[MAX_PACK_NUM];
} Module;

typedef struct FictitiousSoc_t
{
    FictitiousSoc_t() {}

    quint8 drift;
    quint16 soc;
} FictitiousSoc;

typedef struct Quantity_t
{

    float singleChargeQuantity;
    float singleDischargeQuantity;

    float totalCharge;
    float totalDischarge;
}Quantity;

typedef struct Heap_t
{
    quint16 voltage;
    quint16 current;
    quint16 soc;
    quint16 soh;
    quint16 soe;
    quint16 ir;

    quint16 bat_voltage_max;
    quint16 bat_voltage_min;
    qint16 bat_temp_max;
    qint16 bat_temp_min;

    quint32 chargeableQ;
    quint32 remainQ;

    uint64_t chargingCapacity;
    uint64_t dischargingCapacity;
} Heap;
#endif // MODULE_H
