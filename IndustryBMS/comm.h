#ifndef COMM_H
#define COMM_H

#include <QString>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include "paramloader.h"
#include "global.h"

class MainWindow;

namespace CAN{

enum CAN_ID
{  
    MODULE_0 = 0x01,
    MODULE_1,
    MODULE_2,
    MODULE_3,
    MODULE_4,
    MODULE_5,
    MODULE_6,
    MODULE_7,
    MODULE_8,
    MODULE_9,
    MODULE_10,
    MODULE_11
};

enum CAN_MESSAGE
{
    BCU_CHARGE_DISCHARGE_CONTROL = 0x00,
    BCU_ACK = 0x02,
    BCU_HEARTBEAT = 0x04,
    BCU_INFO = 0x20,
    BCU_CELL_VOLTAGE = 0x22,
    BCU_CELL_TEMP = 0x24,
    BCU_CELL_BALANCE = 0x26,
    BCU_FAULT = 0x28,
    BCU_CHARGING_CAPACITY = 0x2A,
    BCU_CALIBRATION_PARA = 0x52,
    BCU_OPERATION_PARA = 0x54,
    BCU_FAULT_PARA = 0x56,

    BAMS_BALANCE_INSTRUCTION_ = 0x10,
    BAMS_RELAY_INSTRUCTION = 0x18,
    BAMS_SLEEP_INSTRUCTION = 0x1A,
    BAMS_SEND_CALIBRATION_PARA = 0x32,
    BAMS_SEND_OPERATION_PARA = 0x34,
    BAMS_SEND_FAULT_PARA = 0x36,
    BAMS_PARALLEL_OPERATION = 0x3A,
    BAMS_READ_PARA = 0x42,

    BAMS_HEARTBEAT = 0x1C1
};

enum CAN_BOOTLOAD
{
    BOOTLOAD_PING = 0x5A0,
    BOOTLOAD_DOWNLOAD = 0x5A1,
    BOOTLOAD_RUN = 0x5A2,
    BOOTLOAD_SEND_DATA = 0x5A4,
    BOOTLOAD_RESET = 0x5A5,
    BOOTLOAD_ACK = 0x5A6

};

}

namespace TCPIP{

typedef struct network_t
{
    bool isDhcp;
    char ip[20];
    char mask[20];
    char gateway[20];
    char dns[20];
    char macAddr[20];
}network;

}

namespace MODBUS{

typedef struct PCS_t
{
    bool _isCharging;
    bool isCharging;
    quint16 pileSoc[2];
    quint16 antialiasingSoc[2];
    quint16 data[16];
    QList<unsigned> checkList;
    
    quint16 getPileSoc() {
        return 0xFFFF == antialiasingSoc[NEXT] ? 0 : antialiasingSoc[NEXT];
    }
}PCS;

const uint16_t MB_BITS_NB_MAX = 2000;
const uint16_t MB_REGISTERS_NB_MAX = 120;

}

class COMM
{
public:
    COMM();
    static int Decode(can_frame &frame, MainWindow * const mw);
    static void Encode(can_frame &frame, qreal pValue, int dest, int id, int precision, ParamLoader::ParamType pType);
    static bool getBit(unsigned char byte, int bit);

private:
    /*static void modifyPara(int type,int id,int16_t data,int page,MainWindow * const mw);*/
};

#endif // COMM_H
