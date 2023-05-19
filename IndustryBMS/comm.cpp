#include "comm.h"
#include "mainwindow.h"
#include "math.h"
#include <QString>
#include <QDebug>

COMM::COMM()
{
}

int COMM::Decode(can_frame &frame, MainWindow * const mw)
{
    QMutexLocker locker(&mw->HeartMutex);

#ifdef CAN_BUS_MONITOR
    static quint16 crc[MAX_MODULE_NUM][2] = {{0}};
#endif

    Module *m = mw->module;
    uint8_t *data = frame.data;

    uint can_id = frame.can_id & CAN_SFF_MASK;
    uint msg_id = (can_id & ~MODULE_ID_MASK) >> 4;
    uint m_id = can_id & MODULE_ID_MASK;
    uint p_id = 0;
    uint page = 0;

    if(can_id < CAN::BOOTLOAD_PING)
    {
        if(m_id > 0 && m_id < (CAN::MODULE_0 + mw->MODULE_NUM)) {
            switch (msg_id) {
            case CAN::BCU_CHARGE_DISCHARGE_CONTROL:

                if(m[m_id-1].chargeCurrentMax ^ data[4])
                    qCritical()<< QString("Module: %1,ChargeCurrent: %2 -> %3,State: %4,Current: %5,VoltageMax: %6,VoltageMin: %7,TempMax: %8,TempMin: %9")
                                  .arg(m_id).arg(m[m_id-1].chargeCurrentMax).arg(data[4]).arg(m[m_id-1].chargeState).arg(m[m_id-1].moduleCurrent)
                                    .arg(m[m_id-1].maxV).arg(m[m_id-1].minV).arg(m[m_id-1].maxT).arg(m[m_id-1].minT);

                if(m[m_id-1].dischargeCurrentMax ^ data[5])
                    qCritical()<< QString("Module: %1,DischargeCurrent: %2 -> %3,State: %4,Current: %5,VoltageMax: %6,VoltageMin: %7,TempMax: %8,TempMin: %9")
                                          .arg(m_id).arg(m[m_id-1].dischargeCurrentMax).arg(data[5]).arg(m[m_id-1].chargeState).arg(m[m_id-1].moduleCurrent)
                                            .arg(m[m_id-1].maxV).arg(m[m_id-1].minV).arg(m[m_id-1].maxT).arg(m[m_id-1].minT);

                m[m_id-1].chargeEnable = data[2];
                m[m_id-1].dischargeEnable = data[3];
                m[m_id-1].chargeCurrentMax = data[4];
                m[m_id-1].dischargeCurrentMax = data[5];
                m[m_id-1].chargePowerMax = data[6];
                m[m_id-1].dischargePowerMax = data[7];
                break;

            case CAN::BCU_ACK:
                if(data[0] == 0x05)return m_id;

            case CAN::BCU_HEARTBEAT:
                m[m_id-1].heartbeat = true;
                m[m_id-1].BCU_swVer = (uint16_t)(data[0] << 8 | data[1]);
                m[m_id-1].BCU_hwVer = (uint16_t)(data[2] << 8 | data[3]);
                m[m_id-1].BMU_swVer = (uint16_t)(data[4] << 8 | data[5]);
                m[m_id-1].BMU_hwVer = (uint16_t)(data[6] << 8 | data[7]);
                break;

            case CAN::BCU_INFO:
                switch (data[0])
                {
                case 0x01:
                    if(m[m_id-1].chargeState ^ data[1])
                        qCritical()<< QString("Module: %1,State: %2 -> %3").arg(m_id).arg(m[m_id-1].chargeState).arg(data[1]);

                    m[m_id-1].chargeState = data[1];
                    m[m_id-1].mainRelay = data[2];
                    m[m_id-1].totalNegativeRelay = data[3];
                    m[m_id-1].preChargeRelay = data[4];
                    m[m_id-1].breaker = getBit(data[6], 0);
                    m[m_id-1].fuseWire = getBit(data[6], 1);
                    m[m_id-1].buzzer = getBit(data[6], 2);
                    m[m_id-1].forceMode = getBit(data[7], 0);
                    break;
                case 0x02:
                    m[m_id-1].moduleVoltage = (data[1] << 8 | data[2]);
                    m[m_id-1].moduleCurrent = (data[3] << 8 | data[4]);
                    m[m_id-1].insulationRes = (data[5] << 8 | data[6]);
                    m[m_id-1].selfChecking = 0x01 == data[7];
                    break;
                case 0x03:
                    m[m_id-1].moduleSoc = (data[1] << 8 | data[2]);
                    m[m_id-1].realCapacity = (data[3] << 8 | data[4]) ;
                    m[m_id-1].realSOC = (data[5] << 8 | data[6]);
                    break;
                case 0x04:
                    m[m_id-1].fanState = (data[3] << 16 | data[2] << 8 | data[1]) ;
                    break;
                case 0x05:
                    m[m_id-1].afterPrechargeVoltage = (data[1] << 8 | data[2]);
                    m[m_id-1].moduleSoh = (data[3] << 8 | data[4]);
                    m[m_id-1].tempRiseRate = (data[5] << 8 | data[6]);
                    break;
                case 0x06:
                    m[m_id-1].maxV = (data[1] << 8 | data[2]);
                    m[m_id-1].minV = (data[3] << 8 | data[4]);
                    m[m_id-1].maxVPackID = data[5];
                    m[m_id-1].maxVID = data[6];
                    m[m_id-1].minVPackID = data[7];
                    break;
                case 0x07:
                    m[m_id-1].minVID = data[1];
                    m[m_id-1].minT = (data[2] << 8 | data[3]);
                    m[m_id-1].maxT = (data[4] << 8 | data[5]);
                    m[m_id-1].maxTPackID = data[6];
                    m[m_id-1].maxTID = data[7];
                    break;
                case 0x08:
                    m[m_id-1].minTPackID = data[1];
                    m[m_id-1].minTID = data[2];
                    m[m_id-1].moduleSoe = data[3];
                    m[m_id-1].chargingCapacityOnce = (data[4] << 8 | data[5]);
                    m[m_id-1].dischargingCapacityOnce = (data[6] << 8 | data[7]);
                    break;
                }
                break;

            case CAN::BCU_CELL_VOLTAGE:

                p_id = data[0]-1;
                page = data[1]-1;

                if(p_id < MAX_PACK_NUM && page < ceil((double)BAT_NUM/3)){

                    int id = 0;
                    for (int i = 0; i < 3; ++i) {

                        id = p_id*BAT_NUM + (page*3 + i);
                        if(id < MAX_PACK_NUM*BAT_NUM && (page*3 + i) < BAT_NUM) {
                            m[m_id-1].voltage[id] = (short)(data[i*2 + 2] << 8 | data[i*2 + 3]);
                        }
                    }
                }
                break;

            case CAN::BCU_CELL_TEMP:

                p_id = data[0]-1;
                page = data[1]-1;

                if(p_id < MAX_PACK_NUM && page < ceil((double)TEMP_NUM/3)){

                    int id = 0;
                    for (int i = 0; i < 3; ++i) {

                        id = p_id*TEMP_NUM + (page*3 + i);

                        if(id < MAX_PACK_NUM*TEMP_NUM && (page*3 + i) < TEMP_NUM) {
                            m[m_id-1].temp[id] = (short)(data[i*2 + 2] << 8 | data[i*2 + 3]);
                        }
                    }
                }
                break;

            case CAN::BCU_CELL_BALANCE:

                p_id = (data[0] & PACK_ID_MASK) - 1;

                if (p_id < MAX_PACK_NUM) {
                    m[m_id-1].pack[p_id].balanceState = data[1];
                    m[m_id-1].pack[p_id].balanceBat = data[2];
                    m[m_id-1].pack[p_id].balanceMode = data[3];
                }
                break;

            case CAN::BCU_FAULT:
            {

#ifdef CAN_BUS_MONITOR
                if (0 < m_id && MAX_MODULE_NUM + 1 > m_id && 0 < data[0] && 3 > data[0]) {
                    quint16 _crc = qChecksum((const char *)data, 8);
                    if (_crc != crc[m_id - 1][data[0] - 1]) {
                        crc[m_id - 1][data[0] - 1] = _crc;
                        mw->logCanFrame(frame, "CAN_FRAME :");
                    }
                } else {
                    mw->logCanFrame(frame, "Unresolved CAN_FRAME :");
                }
#endif

//                QMutexLocker locker(&mw->MapMutex);
                for (int i = 1; i < 8; ++i) {
                    for (int j = 0; j < CHAR_BIT; ++j) {
                        int key = ((data[0] - 1) * 8 + i) * CHAR_BIT + j;
                        char bit = data[i] & (0x01 << j);
                        mw->handleBitInWarningSet(m_id, key, 0 != bit);
//                        if (data[i] & (0x01 << j)) {
//                            if (mw->wnMarks.contains(key)) {
//                                if (!mw->wnMarks.values(key).contains(m_id)) {
//                                    mw->wnMarks.insertMulti(key, m_id);
//                                }
//                            } else {
//                                mw->wnMarks.insert(key, m_id);
//                            }
//                        } else {
//                            QMap<int, int>::iterator i = mw->wnMarks.lowerBound(key);
//                            while (i != mw->wnMarks.upperBound(key)) {
//                                if (m_id == (uint)i.value()) {
//                                    mw->wnMarks.erase(i);
//                                    break;
//                                }
//                                ++i;
//                            }
//                        }
                    }
                }
                /*uint code = (m_id << 16) | (data[0] << 8) | data[1];
                uint value = (data[2] << 8 | data[3]);

                if(mw->fault.find(code) == mw->fault.end()) {
                    qCritical()<< QString("Module: %1,FaultCode: %2,FaultValue: %3,State: %4,Current: %5,VoltageMax: %6,VoltageMin: %7,TempMax: %8,TempMin: %9")
                                          .arg(m_id).arg(code).arg(value).arg(m[m_id-1].chargeState).arg(m[m_id-1].moduleCurrent)
                                            .arg(m[m_id-1].maxV).arg(m[m_id-1].minV).arg(m[m_id-1].maxT).arg(m[m_id-1].minT);
                }

                mw->fault.insert(code,value);*/
            }
                break;

            case CAN::BCU_CHARGING_CAPACITY:
                m[m_id - 1].chargingCapacity = (data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3]);
                m[m_id - 1].dischargingCapacity = (data[4] << 24 | data[5] << 16 | data[6] << 8 | data[7]);
                break;
            case CAN::BCU_CALIBRATION_PARA:
                if (!(MAX_PARAM_NUM_PER_MODULE > data[1])) {
                    break;
                }

                mw->calibParam[m_id - 1][data[1]] = (quint32)(data[2] << 24 | data[3] << 16 | data[4] << 8 | data[5]);
                break;
            case CAN::BCU_OPERATION_PARA:
                if (!(MAX_PARAM_NUM_PER_MODULE > data[1] && MAX_PARAM_NUM_PER_MODULE > data[4])) {
                    break;
                }

                mw->operatingParam[m_id - 1][data[1]] = (quint16)(data[2] << 8 | data[3]);
                mw->operatingParam[m_id - 1][data[4]] = (quint16)(data[5] << 8 | data[6]);
                break;
            case CAN::BCU_FAULT_PARA:
                if (!(MAX_PARAM_NUM_PER_MODULE > data[1] && MAX_PARAM_NUM_PER_MODULE > data[4])) {
                    break;
                }

                mw->warningParam[m_id - 1][data[1]] = (quint16)(data[2] << 8 | data[3]);
                mw->warningParam[m_id - 1][data[4]] = (quint16)(data[5] << 8 | data[6]);
//                qDebug()<< QString("CAN_id: 0x%1 msg_id: 0x%2 m_id : %3! ").
//                             arg(frame.can_id,8,16,QChar('0')).toUpper().arg(msg_id,2,16,QChar('0')).arg(m_id);
                break;

            default:
                break;
            }
        }

    }
    else if(can_id == CAN::BOOTLOAD_ACK || can_id == CAN::BOOTLOAD_ACK + 0x100)
    {
        mw->mcu_ack = true;
    }
    else
    {
//        qWarning()<< QObject::tr("CAN ID error!id: 0x%1! ").arg(frame.can_id,8,16,QChar('0')).toUpper();
    }

    return 0;
}

/*void COMM::modifyPara(int type,int id,int16_t data,int page,MainWindow * const mw)
{
    float value;
    QString flag;

    switch (type) {
    case CAN::BCU_OPERATION_PARA:
        flag = "m";
        switch(id)
        {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            value = (float)data/1000;
            break;
        case 7:
        case 8:
            value = (float)data/10;
            break;
        case 9:
        case 10:
            value = (float)data;
            break;
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
            value = (float)data/10;
            break;
        case 17:
            value = (float)data;
            break;
        case 18:
            value = (float)data/10;
            break;
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
        case 27:
        case 28:
            value = (float)data;
            break;
        case 29:
            value = (float)data/10;
            break;
        case 30:
            value = (float)data/100;
            break;
        case 31:
            value = (float)data;
            break;
        }
        break;

    case CAN::BCU_FAULT_PARA:
        flag = "f";
        switch(id)
        {
        case 1:
        case 2:
            value = (float)data/1000;
            break;
        case 3:
        case 4:
            value = (float)data/10;
            break;
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
            value = (float)data;
            break;
        case 12:
        case 13:
            value = (float)data/10;
            break;
        }
        break;

    case CAN::BCU_CALIBRATION_PARA:
        flag = "c";
        switch(id)
        {
        case 1:
        case 2:
        case 3:
        case 4:
        case 17:
        case 18:
            value = (float)data/100;
            break;
        }
        break;

    default:
        return;
    }

    QString *p = mw->getPtr(flag,id,page-1);
    if(p == NULL){
        qWarning()<<"Error parameter id,flag:%1"<<flag<<"id:"<<id;
        return;
    }

    *p = QString::number(value);

}*/

void COMM::Encode(can_frame &frame, qreal pValue, int dest, int id, int precision, ParamLoader::ParamType pType)
{
    quint16 ui16 = 0;
    quint32 ui32 = 0;

    switch (pType) {
    case ParamLoader::Operating:
        ui16 = (quint16)qRound(pValue * qPow(10, precision));
        frame.can_dlc = 4;
        frame.can_id = (CAN::BAMS_SEND_OPERATION_PARA << 4) | dest;
        frame.data[0] = 0x80;
        frame.data[1] = id;
        frame.data[2] = ui16 >> 8;
        frame.data[3] = ui16;
        break;
    case ParamLoader::Warning:
        ui16 = (quint16)qRound(pValue * qPow(10, precision));
        frame.can_dlc = 4;
        frame.can_id = (CAN::BAMS_SEND_FAULT_PARA << 4) | dest;
        frame.data[0] = 0x90;
        frame.data[1] = id;
        frame.data[2] = ui16 >> 8;
        frame.data[3] = ui16;
        break;
    case ParamLoader::Calibration:
        ui32 = (quint32)qRound64(pValue * qPow(10, precision));
        frame.can_dlc = 6;
        frame.can_id = (CAN::BAMS_SEND_CALIBRATION_PARA << 4) | dest;
        frame.data[0] = 0x70;
        frame.data[1] = id;
        frame.data[2] = ui32 >> 24;
        frame.data[3] = ui32 >> 16;
        frame.data[4] = ui32 >> 8;
        frame.data[5] = ui32;
        break;
    default:
        break;
    }
}

bool COMM::getBit(unsigned char byte, int bit)
{
    if (0 > bit || CHAR_BIT < bit + 1) {
        return false;
    }

    return 0 != (byte & (0x01 << bit));
}
