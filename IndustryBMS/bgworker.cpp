#include <QThread>
#include <QDebug>
#include <QDateTime>
#include <QFileInfo>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <QStringList>
#include "bgworker.h"
#include "mainwindow.h"
#include "comm.h"
#include "dlt645.h"
#include "dlt645_port.h"
#include "libserialport.h"

extern "C"{

int min_3(int a,int b,int c)
    {
        int temp;
        temp = MIN(a,b);
        return MIN(temp,c);
    }

int max_3(int a,int b,int c)
    {
        int temp;
        temp = MAX(a,b);
        return MAX(temp,c);
    }

int cmp(const void *a, const void *b)
    {
        return *(short *)a - *(short *)b;
    }
}

BgWorker::BgWorker(MainWindow *mainwindow, int n, int interval) :
    Base(n)
  , mw(mainwindow)
  , period(interval * 1000)
{
    moveToThread(&thread);
    isStop = false;
}

void BgWorker::readCan()
{
    struct can_frame buf;
    while(!isStop)
    {
        int ret = mw->hmi->ReadCan(&buf);
        if(ret > 0)
        {
           int ret2 = COMM::Decode(buf,mw);

           if(ret2){
               emit response_Signal(ret2);
           }
        }
        else
        {
            usleep(period);
        }
    }
}

void BgWorker::readLog()
{
    read_log(g_log,g_logBak,mw->logText);
    read_log(g_log2,g_log2Bak,mw->log2Text);
}


void BgWorker::read_log(QFile& log,QFile& logBak,QByteArray *text)
{
    int beginPos,endPos,num,index;

    QByteArray ba;
    ba.clear();
    logBak.seek(0);
    ba.append(logBak.readAll());
    log.seek(0);
    ba.append(log.readAll());

    beginPos =0;
    endPos = ba.size()-1;
    num = 0;
    for(int i=0;i < LOG_MAX_PAGE;i++)
    {
        text[i].clear();
    }

    do{
        index = num/LOG_MAX_COUNT;
        if(index == LOG_MAX_PAGE)break;

        beginPos = ba.lastIndexOf("\n",beginPos-1);
        num++;
        if(num > 1)
        {
            text[index].append(ba.mid(beginPos+1,endPos-beginPos));
            endPos = beginPos;
        }
    }while(beginPos > 0);

}

/**
 * @brief switch the path of data file to be written into APP_DATA_DIRPATH/year-month/year-month-day.csv
 * if direcotry 'year-month' is not created, create it.
 * @param date today's date.
 * @return 0 switch successful.
 * @return -1 make path failed.
 * @return -2 file open failed.
 */
int BgWorker::switchFileByDate(QDate date)
{
    int ret = 0;
    QString dirPath = QString("%1/%2").arg(APP_DATA_DIRPATH, date.toString("yyyy-MM"));
    QDir dataDir(dirPath);

    if (dataDir.exists()) {
        if (!file.fileName().contains(date.toString("yyyy-MM-dd"))) {
            if (file.isOpen()) {
                file.close();
            }

            file.setFileName(dirPath + "/" + date.toString("yyyy-MM-dd") + ".csv");
            if (!file.open(QFile::ReadWrite | QFile::Append)) {
                ret = -2;
            }
        } else {
            if (!file.exists()) {
                if (file.isOpen()) {
                    qDebug() << __func__ << "file not exists but opened";
                    file.close();
                }

                if (!file.open(QFile::ReadWrite | QFile::Append)) {
                    ret = -2;
                }
            }
        }
    } else {
        if (dataDir.mkpath(dirPath)) {
            if (file.isOpen()) {
                file.close();
            }

            file.setFileName(dirPath + "/" + date.toString("yyyy-MM-dd") + ".csv");
            if (!file.open(QFile::ReadWrite | QFile::Append)) {
                ret = -2;
            }
        } else {
            ret = -1;
        }
    }

    return ret;
}

void BgWorker::saveData()
{
    QMutexLocker locker(&mw->HeartMutex);

    int ret = 0;
    bool firstFound = true;
    QString header,info;
    QDateTime dateTime = QDateTime::currentDateTime();
    QDate today = dateTime.date();
    QTime now = dateTime.time();
    QString balance;
    QTextCodec *codec = QTextCodec::codecForName("GBK");        // codec for Windows
    QTextStream out(&file);

#if 0
    if(file.size() > DATA_FILE_MAX_SIZE)
    {
        if(dir_SD.exists())
        {
            dir.setPath(dir_SD.absolutePath() + "/" + today.toString("yyyy-MM-dd"));
            if(!dir.exists())dir.mkpath(dir.absolutePath());

            if(!file.copy(dir.absolutePath() + "/" + QTime::currentTime().toString("hh_mm_ss") + QString(".csv")))
            {
                qWarning()<<"Copy data to SDcard failed!";

                if(checkDisk())
                {
                    clearDisk();
                    qWarning()<<"SDcard full,auto clear!";
                }
            }
        }
        else
            qWarning()<<"No SDcard,copy data to SDcard failed!";

        if(!file.isOpen())file.open(QIODevice::ReadWrite | QIODevice::Truncate);

        file.resize(0);
    }
#else
    clearDiskIfNeed();
    ret = switchFileByDate(today);
    if (0 > ret) {
        qDebug() << "switch file by date failed with errno:" << ret;
        return;
    }
#endif

    out.setCodec(codec);
//    qDebug() << __func__ << file.fileName() << "file size:" << file.size() << "open:" << file.isOpen();
    if (file.size() == 0) {
//        header = QString("Date,Time,moduleID,chargeState,moduleSOC,mainRelay,forceMode,moduleVoltage,moduleCurrent,insulationRes,chargeCurrentMax,dischargeCurrentMax,"
//                 "cellVoltageMax,ID,cellVoltageMin,ID,cellTempMax,ID,cellTempMin,ID,balanceCellID(state)");
        header = QString::fromUtf8("日期,时间,组号,电池状态,组SOC(%),主继电器状态,强制模式,组电压(V),组电流(A),绝缘电阻(Ω),最大充电电流(A),最大放电电流(A),"
                                   "最高单体电压(V),从控编号,单体编号,最低单体电压(V),从控编号,单体编号,最高单体温度(℃),从控编号,单体编号,最低单体温度(℃),"
                                   "从控编号,单体编号,压差(mV),温差(℃),均衡电池编号(状态)");

        for (int i = 0; i < BATTERY_NUM_PER_MODULE; i++) {
            header += QString::fromUtf8(",单体电压%1(V)").arg(i + 1);
        }

        for (int i = 0; i < TEMP_NUM_PER_MODULE; i++) {
            header += QString::fromUtf8(",单体温度%1(℃)").arg(i + 1);
        }

        header += "\n";
        out << header;
    }

    info.clear();
    for (uint i = 0;i < MODULE_NUM; i++) {
        if (!mw->module[i].heartbeat_bak) {
//            continue;
            if (0 < i && !mw->module[i - 1].heartbeat_bak) {
                continue;
            }
        }

//        if (0 == mw->module[i].maxV) {
//            continue;
//        }

        info = firstFound ? (today.toString("yyyy-MM-dd") + "," + now.toString("hh:mm:ss")) : " , ";
        info += QString(",%1,%2,%3,%4,%5,%6,%7,%8").arg(i+1).arg(mw->module[i].chargeState)
                .arg(mw->module[i].moduleSoc / 10.0, 0, 'f', 1).arg(mw->module[i].mainRelay).arg(mw->module[i].forceMode)
                .arg(mw->module[i].moduleVoltage / 10.0, 0, 'f', 1).arg(mw->module[i].moduleCurrent / 10.0, 0, 'f', 1)
                .arg(mw->module[i].insulationRes);
        info += QString(",%1,%2").arg(mw->module[i].chargeCurrentMax).arg(mw->module[i].dischargeCurrentMax);
        info += QString(",%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14")
                .arg(mw->module[i].maxV / 1000.0, 0, 'f', 3).arg(mw->module[i].maxVPackID).arg(mw->module[i].maxVID)
                .arg(mw->module[i].minV / 1000.0, 0, 'f', 3).arg(mw->module[i].minVPackID).arg(mw->module[i].minVID)
                .arg(mw->module[i].maxT / 10.0, 0, 'f', 1).arg(mw->module[i].maxTPackID).arg(mw->module[i].maxTID)
                .arg(mw->module[i].minT / 10.0, 0, 'f', 1).arg(mw->module[i].minTPackID).arg(mw->module[i].minTID)
                .arg(qAbs(mw->module[i].maxV - mw->module[i].minV))
                .arg(qAbs(mw->module[i].maxT - mw->module[i].minT) / 10.0, 0, 'f', 1);
        balance = ", ";

        for(int k = 0;k < MAX_PACK_NUM;k++)
        {
            if(mw->module[i].pack[k].balanceBat)
            balance += QString("%1(%2)|").arg((mw->module[i].pack[k].balanceBat-2)+(k*BAT_NUM)).arg(mw->module[i].pack[k].balanceState);
        }

        info += balance;
        for(int k = 0;k < BAT_NUM*MAX_PACK_NUM;k++)
        {
            info += QString(",%1").arg(mw->module[i].voltage[k] / 1000.0, 0, 'f', 3);
        }

        for(int k = 0;k < TEMP_NUM*MAX_PACK_NUM;k++)
        {
            info += QString(",%1").arg(mw->module[i].temp[k] / 10.0, 0, 'f', 1);
        }

        info += QString("\n");
        out << info;
        firstFound = false;
    }

    out.flush();
}

bool BgWorker::checkDisk()
{
    uint32_t free,used,total;
    struct statfs info;

    statfs(dir_SD.absolutePath().toLatin1().data(),&info);

    total = (info.f_blocks >> 10) * (info.f_bsize >> 10) ;
    free =  (info.f_bfree >> 10) * (info.f_bsize >> 10) ;
    used = total - free;

    if((float)used/total > 0.85)
        return true;
    else
        return false;
}

void BgWorker::clearDisk()
{
    QFileInfoList list = dir_SD.entryInfoList(QDir::Dirs,QDir::Name);

    foreach (QFileInfo fi, list)
    {
        QStringList name = fi.fileName().split("-");

        if(name.size() > 1)
        {
            if(name.at(0).toInt() != QDate::currentDate().year()
                                        || (QDate::currentDate().month()-name.at(1).toInt()) > 2)
            {
                QString cmd = QString("rm -rf ") + fi.absoluteFilePath();
                system(cmd.toStdString().c_str());
            }
        }
    }
}


void BgWorker::copyData(QString cmd)
{
    system(cmd.toStdString().c_str());
    emit processDone_Signal();
}

void BgWorker::updateMCU(QString path, int type)
{
    int i;
    int num = 0;
    int fileSize = 0;
//    int adress = 0x20000;
    int adress = 0x4000; // temp
    int increment = 0;
    uint8_t data[8] = {0};

    if(type != 0)
    {
        increment = 0x100;
    }

    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))
    {
        qWarning()<<"Update MCU: can't open file!";
        emit updatingMCU_Signal(-1);
        return;
    }

    QByteArray ba = file.readAll();
    fileSize = ba.size();
    if(!fileSize)
    {
        qWarning()<<"Update MCU: can't read file!";
        emit updatingMCU_Signal(-1);
        return;
    }

    //start update
    emit updatingMCU_Signal(0);

    //reset mcu
    writeCan(data,0,CAN::BOOTLOAD_RESET + increment);
    usleep(200000);
    //ping
    do{
        writeCan(data,0,CAN::BOOTLOAD_PING + increment);
        usleep(300000);

        if(++num > 2)
        {
            num = 0;
            qWarning()<<"Update MCU: ping mcu timeout!";
            emit updatingMCU_Signal(-1);
            return;
        }
    }while(!mw->mcu_ack);

    usleep(2000000);

    //download
    for(int i = 0;i < 4;++i)
    {
        data[i] = adress >> (3-i)*8;
        data[i+4] = fileSize >> (3-i)*8;
    }
    writeCan(data,8,CAN::BOOTLOAD_DOWNLOAD + increment);
    usleep(200000);

    //send data
    mw->mcu_ack = false;
    int offset = 0;

    do{
        for(i = 0;i < 8;i++)
        {
            if(offset > (fileSize-1))break;

            data[i] = ba[offset++];
        }

        writeCan(data,i,CAN::BOOTLOAD_SEND_DATA + increment);

        usleep(5000);
        emit updatingMCU_Signal(offset*99/fileSize);

    }while(offset < fileSize);

    while(!mw->mcu_ack)
    {
        usleep(200000);
        if(++num > 2)
        {
            num = 0;
            qWarning()<<"Update MCU: send data timeout!";
            emit updatingMCU_Signal(-1);
            return;
        }
    }

    //run
    for(int i = 0;i < 4;++i)
    {
        data[i] = adress >> (3-i)*8;
    }
    writeCan(data,4,CAN::BOOTLOAD_RUN + increment);

    emit updatingMCU_Signal(100);
}

void BgWorker::send2BMS()
{
    uint8_t data[8] = {0};
    QList<quint16> list;

    list << mw->hmi->peripheral.dcInsulationMonitoringDevice[0].total_bus_resistance;
    list << mw->hmi->peripheral.dcInsulationMonitoringDevice[0].positive_bus_resistance_to_ground;
    list << mw->hmi->peripheral.dcInsulationMonitoringDevice[0].negative_bus_resistance_to_ground;
    qSort(list.begin(), list.end());

    data[0] = 0x01;
    data[1] = mw->isFireAlarm ? 0x01 : 0x00;
    data[2] = mw->hmi->peripheral.airConditioner[0].cabinet_air_temp >> 8;
    data[3] = mw->hmi->peripheral.airConditioner[0].cabinet_air_temp;
    data[4] = list.first() >> 8;
    data[5] = list.last();

    writeCan(data, 6, CAN::BAMS_HEARTBEAT);
}

void BgWorker::pcs()
{
    int socket;
    modbus_t *ctx;
    modbus_mapping_t *mb_mapping;
    int rc;
    int use_backend;
    const char *mode = "rtu";

    /* TCP */
    if (2 > 1) {
        if (strcmp(mode, "tcp") == 0) {
            use_backend = 0;
        } else if (strcmp(mode, "rtu") == 0) {
            use_backend = 1;
        } else {
            printf("Usage:\n  %s [tcp|rtu] - Modbus client to measure data bandwith\n\n", mode);
            exit(1);
        }
    } else {
        /* By default */
        use_backend = 0;
    }

    if (use_backend == 0) {
        ctx = modbus_new_tcp("127.0.0.1", 1502);
        socket = modbus_tcp_listen(ctx, 1);
        modbus_tcp_accept(ctx, &socket);

    } else {
        ctx = modbus_new_rtu(COM3, 9600, 'N', 8, 1);
        modbus_set_slave(ctx, 2);
        modbus_connect(ctx);
    }

    mb_mapping = modbus_mapping_new(MODBUS_MAX_READ_BITS, 0,
                                    MODBUS_MAX_READ_REGISTERS, 0);
    if (mb_mapping == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        return;
    }

    for(;;) {
        uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];

        rc = modbus_receive(ctx, query);
        if (rc >= 0) {
            /* calculate max allowed charging current */
            QList<quint8> chargeCurrentList;

            for (uint i = 0; i < MODULE_NUM; ++i) {
                chargeCurrentList << mw->module[i].chargeCurrentMax;
            }
            qSort(chargeCurrentList.begin(), chargeCurrentList.end());
            mb_mapping->tab_registers[0] = chargeCurrentList.isEmpty()
                    ? 0 : chargeCurrentList.first() * MODULE_NUM * 10;
            modbus_reply(ctx, query, rc, mb_mapping);
        } else {
            /* Do not Connection closed by the client or server */
//            break;
        }
    }

    printf("Quit the loop: %s\n", modbus_strerror(errno));

    modbus_mapping_free(mb_mapping);
    close(socket);
    modbus_free(ctx);

    return;
}

void BgWorker::send2PCS()
{
    quint8 buf[0x21];         // 1+1+1+30
    checkFault();

#if defined(PROJECT_165)
    modbus_set_slave(mw->modbus,1);
    pcsDataProc(0);
    modbus_write_registers(mw->modbus,0x00,4,mw->pcs[0].data);
#elif defined(XINLONG)
    modbus_set_slave(mw->modbus, 1);

    for (unsigned i = 0; i < MODULE_NUM; ++i) {
        pcsDataProc(i);

        modbus_write_registers(mw->modbus, i << 4, 4, mw->pcs[i].data);
        usleep(period);
    }
#else
//    modbus_set_slave(mw->modbus,1);

    memset(buf, 0x00, sizeof(buf));
    for (uint i = 0; i < PCS_NUM; ++i) {
        pcsDataProc(i);

#if 0
        modbus_set_slave(mw->modbus, i + 1);
        modbus_write_registers(mw->modbus, 0, 12, mw->pcs[i].data);
#else
        buf[0] = i + 1;
        buf[1] = 0x04;
        buf[2] = 0x1E;
//        qMemCopy(buf + 2, mw->pcs[i].data, sizeof(mw->pcs[i].data));
//        qMemCopy(buf + 2, mw->pcs[i].data, sizeof(quint16) * 12);
        for (uint j = 0; j < 12; ++j) {
            buf[(j + 1) * 2 + 1] = mw->pcs[i].data[j] >> 8;
            buf[(j + 2) * 2] = mw->pcs[i].data[j] & 0xFF;
        }
//        quint16 crc = crc16(buf, sizeof(buf) - sizeof(quint16));
//        buf[0x20] = crc & 0xFF;
//        buf[0x21] = crc >> 8;

        modbus_send_raw_request(mw->modbus, buf, sizeof(buf));
        usleep(period);
    }
#endif
#endif

#ifdef XINLONG
    /****** PCS control ******/
    for (unsigned i = 0; i < MODULE_NUM; ++i) {
        if (mw->isMdFt(i + 1)) {
            qDebug() << __func__ << "module" << i + 1 << "fault:";
            mw->hmi->setIO(i, true);
        }
    }
#endif

    /****** broadcast external error ******/
    for (unsigned i = 0; i < MODULE_NUM; ++i) {
        if (mw->getMdFtByF(i + 1, W::InsulationRes | W::Level1)) {
            quint8 data[8] = {0x00};

            data[0] = 0x70;
            data[1] = 25;
            data[5] = 0x01;
            writeCan(data, 8, CAN::BAMS_SEND_CALIBRATION_PARA << 4);
            qDebug() << __func__ << "broadcast external error.";
        }
    }
}

void BgWorker::send2EMS()
{
    QMutexLocker locker(&mw->HeartMutex);

    // set values here, only cell volage and temp
    uint addr = 0x4001 - 0x4001;
    uint n = 0;
    uint nodeNum = 0;
    uint _maxVModule = 0;
    uint _minVModule = 0;
    uint _maxTModule = 0;
    uint _minTModule = 0;
    quint32 voltage = 0;
    qint32 current = 0;
    float chargingCapacity = 0.0f;
    float dischargingCapacity = 0.0f;
    float chargingCapacityOnce = 0.0f;
    float dischargingCapacityOnce = 0.0f;
    QList<quint8> chargeCurrentList;
    QList<quint8> dischargeCurrentList;
    QList<quint8> chargePowerList;
    QList<quint8> dischargePowerList;
    QList<quint16> socList;
    QList<quint16> sohList;
    Module *p = mw->module;

    if (1 > PCS_NUM || MODULE_NUM < PCS_NUM) {
        return;
    }

    /****** remote metering ******/
    setyc(addr, mw->hmi->peripheral.dcInsulationMonitoringDevice[0].positive_bus_resistance_to_ground);
    setyc(addr + 1, p[PREV].moduleSoe / 100.0f);

    addr = 0x4003 - 0x4001;
    for (uint i = 0; i < TEMP_HUMIDITY_METER_NUM; ++i) {
        setyc(addr + i * 2, mw->hmi->peripheral.tempHumidityMeter[i].temperature / 10.0f);
        setyc(addr + i * 2 + 1, mw->hmi->peripheral.tempHumidityMeter[i].humidity / 10.0f);
    }

    // To-DO: electricty meter
    addr = 0x400F - 0x4001;
    for (uint i = 0; i < AIR_CONDITIONER_NUM; ++i) {
        setyc(addr + i, mw->hmi->peripheral.airConditioner[i].cabinet_air_temp / 10.0f);
    }

    addr = 0x4019 - 0x4001;
    n = 0x4034 - 0x4019 + 1;
    for (uint i = 0; i < PCS_NUM; ++i) {
        nodeNum = i + 1 < PCS_NUM
                ? MODULE_NUM / PCS_NUM
                : MODULE_NUM - (MODULE_NUM / PCS_NUM) * (PCS_NUM - 1);

        voltage = 0;
        current = 0;
        chargingCapacity = 0.0f;
        dischargingCapacity = 0.0f;
        chargingCapacityOnce = 0.0f;
        dischargingCapacityOnce = 0.0f;

        chargeCurrentList.clear();
        dischargeCurrentList.clear();
        chargePowerList.clear();
        dischargePowerList.clear();
        socList.clear();
        sohList.clear();
        for (uint j = 0; j < nodeNum; ++j) {
            uint cur = (MODULE_NUM / PCS_NUM) * i + j;
            if (!(MODULE_NUM > cur) || !p[cur].heartbeat_bak) {
                continue;
            }

            chargeCurrentList << p[cur].chargeCurrentMax;
            dischargeCurrentList << p[cur].dischargeCurrentMax;
            chargePowerList << p[cur].chargePowerMax;
            dischargePowerList << p[cur].dischargePowerMax;
            socList << p[cur].moduleSoc;
            sohList << p[cur].moduleSoh;

            voltage += p[cur].moduleVoltage;
            current += p[cur].moduleCurrent;
            chargingCapacity += p[cur].chargingCapacity;
            dischargingCapacity += p[cur].dischargingCapacity;
            chargingCapacityOnce += p[cur].chargingCapacityOnce;
            dischargingCapacityOnce += p[cur].dischargingCapacityOnce;

            if (p[cur].maxV > p[_maxVModule].maxV) {
                _maxVModule = cur;
            }

            if (p[cur].maxT > p[_maxTModule].maxT) {
                _maxTModule = cur;
            }

            if (p[cur].minV < p[_minVModule].minV) {
                _minVModule = cur;
            }

            if (p[cur].minT < p[_minTModule].minT) {
                _minTModule = cur;
            }
        }

        if (0 < socList.size()) {
            qSort(chargeCurrentList.begin(), chargeCurrentList.end());
            qSort(dischargeCurrentList.begin(), dischargeCurrentList.end());
            qSort(chargePowerList.begin(), chargePowerList.end());
            qSort(dischargePowerList.begin(), dischargePowerList.end());
            qSort(socList.begin(), socList.end());
            qSort(sohList.begin(), sohList.end());
        }

        setyc(addr + n * i, socList.isEmpty() ? .0f : voltage / socList.size() / 10.0f);
        setyc(addr + n * i + 1, current / 10.0f);
//        setyc(addr + n * i + 2, socList.isEmpty() ? 0 : socList.first() / 10.0f);
        setyc(addr + n * i + 2, socList.isEmpty() ? 0 : mw->pcs[i].getPileSoc() / 10.0f);
        setyc(addr + n * i + 3, sohList.isEmpty() ? 0 : sohList.first() / 10.0f);
        setyc(addr + n * i + 4, p[_maxVModule].maxV / 1000.0f);
//        setyc(addr + n * i + 5, (_maxVModule % (MODULE_NUM / PCS_NUM)) * PACK_NUM + p[_maxVModule].maxVPackID);
//        setyc(addr + n * i + 6, p[_maxVModule].maxVID);
        setyc(addr + n * i + 5, _maxVModule % (MODULE_NUM / PCS_NUM) + 1);
        setyc(addr + n * i + 6, p[_maxVModule].maxVPackID);
        setyc(addr + n * i + 7, p[_minVModule].minV / 1000.0f);
        setyc(addr + n * i + 8, _minVModule % (MODULE_NUM / PCS_NUM) + 1);
        setyc(addr + n * i + 9, p[_minVModule].minVPackID);
        setyc(addr + n * i + 10, p[_maxTModule].maxT / 10.0f);
        setyc(addr + n * i + 11, _maxTModule % (MODULE_NUM / PCS_NUM) + 1);
        setyc(addr + n * i + 12, p[_maxTModule].maxTPackID);
        setyc(addr + n * i + 13, p[_minTModule].minT / 10.0f);
        setyc(addr + n * i + 14, _minTModule % (MODULE_NUM / PCS_NUM) + 1);
        setyc(addr + n * i + 15, p[_minTModule].minTPackID);
        setyc(addr + n * i + 16, chargingCapacity);
        setyc(addr + n * i + 17, dischargingCapacity);
        setyc(addr + n * i + 18, chargingCapacityOnce);
        setyc(addr + n * i + 19, dischargingCapacityOnce);

        setyc(addr + n * i + 24, chargeCurrentList.isEmpty() ? 0 : chargeCurrentList.first() * chargeCurrentList.size());
        setyc(addr + n * i + 25, dischargeCurrentList.isEmpty() ? 0 : dischargeCurrentList.first() * dischargeCurrentList.size());
        setyc(addr + n * i + 26, p[(MODULE_NUM / PCS_NUM) * i].chargeState);
    }

    addr = 0x4058 - 0x4001;
    n = 0x407B - 0x4058 + 1;
    for (uint i = 0; i < MODULE_NUM; ++i) {
        setyc(addr + n * i, p[i].mainRelay);
        setyc(addr + n * i + 1, p[i].totalNegativeRelay);
        setyc(addr + n * i + 2, p[i].breaker ? 1 : 0);
        setyc(addr + n * i + 3, p[i].moduleVoltage / 10.0f);
        setyc(addr + n * i + 4, p[i].moduleCurrent / 10.0f);
        setyc(addr + n * i + 5, 20.0f);        // temp
//        setyc(addr + n * i + 6, p[i].moduleSoc / 10.0f);
        setyc(addr + n * i + 6, mw->fictitiousSoc[i].soc / 10.0f);

        setyc(addr + n * i + 8, (p[i].maxV + p[i].minV) / 2000.0f);
        setyc(addr + n * i + 9, (p[i].maxT + p[i].minT) / 20.0f);
        setyc(addr + n * i + 10, p[i].maxV / 1000.0f);
        setyc(addr + n * i + 11, p[i].maxVID);
        setyc(addr + n * i + 12, p[i].minV / 1000.0f);
        setyc(addr + n * i + 13, p[i].minVID);
        setyc(addr + n * i + 14, p[i].maxT / 10.0f);
        setyc(addr + n * i + 15, p[i].maxTID);
        setyc(addr + n * i + 16, p[i].minT / 10.0f);
        setyc(addr + n * i + 17, p[i].minTID);

        setyc(addr + n * i + 22, p[i].chargingCapacity);
        setyc(addr + n * i + 23, p[i].dischargingCapacity);
        setyc(addr + n * i + 24, p[i].chargingCapacityOnce);
        setyc(addr + n * i + 25, p[i].dischargingCapacityOnce);

        setyc(addr + n * i + 28, p[i].moduleSoe / 100.0f);
        setyc(addr + n * i + 29, p[i].moduleSoh / 10.0f);
        setyc(addr + n * i + 30, p[i].tempRiseRate);
        setyc(addr + n * i + 31, (p[i].maxV - p[i].minV) / 1000.0f);
        setyc(addr + n * i + 32, p[i].chargeCurrentMax);
        setyc(addr + n * i + 33, p[i].dischargeCurrentMax);
    }

    addr = 0x429A - 0x4001;
    for (uint i = 0; i < MODULE_NUM; ++i) {
        for (int j = 0; j < BATTERY_NUM_PER_MODULE; ++j) {
            setyc(addr + (BATTERY_NUM_PER_MODULE + TEMP_NUM_PER_MODULE) * i + j
                  , p[i].voltage[j] / 1000.0f);
        }

        for (int j = 0; j < TEMP_NUM_PER_MODULE; ++j) {
            setyc(addr + (BATTERY_NUM_PER_MODULE + TEMP_NUM_PER_MODULE) * i
                  + BATTERY_NUM_PER_MODULE + j
                  , p[i].temp[j] / 10.0f);
        }
    }

    /****** remote signaling part2 ******/
    addr = 0x31 - 1;
    n = 0x66 - 0x31 + 1;
    for (unsigned i = 0; i < MODULE_NUM; ++i) {
        setyx(addr + n * i, mw->getMdFtByF(i + 1, W::ModuleLowVoltage | W::Level3));
        setyx(addr + n * i + 1, mw->getMdFtByF(i + 1, W::ModuleLowVoltage | W::Level2));
        setyx(addr + n * i + 2, mw->getMdFtByF(i + 1, W::ModuleLowVoltage | W::Level1));
        setyx(addr + n * i + 3, mw->getMdFtByF(i + 1, W::ModuleOverVoltage | W::Level3));
        setyx(addr + n * i + 4, mw->getMdFtByF(i + 1, W::ModuleOverVoltage | W::Level2));
        setyx(addr + n * i + 5, mw->getMdFtByF(i + 1, W::ModuleOverVoltage | W::Level1));

        setyx(addr + n * i + 6, mw->getMdFtByF(i + 1, W::ModuleOverCurrent | W::Level3));
        setyx(addr + n * i + 7, mw->getMdFtByF(i + 1, W::ModuleOverCurrent | W::Level2));
        setyx(addr + n * i + 8, mw->getMdFtByF(i + 1, W::ModuleOverCurrent | W::Level1));

        setyx(addr + n * i + 9, mw->getMdFtByF(i + 1, W::CellLowVoltage | W::Level3));
        setyx(addr + n * i + 10, mw->getMdFtByF(i + 1, W::CellLowVoltage | W::Level2));
        setyx(addr + n * i + 11, mw->getMdFtByF(i + 1, W::CellLowVoltage | W::Level1));
        setyx(addr + n * i + 12, mw->getMdFtByF(i + 1, W::CellOverVoltage | W::Level3));
        setyx(addr + n * i + 13, mw->getMdFtByF(i + 1, W::CellOverVoltage | W::Level2));
        setyx(addr + n * i + 14, mw->getMdFtByF(i + 1, W::CellOverVoltage | W::Level1));

        setyx(addr + n * i + 15, mw->getMdFtByF(i + 1, W::CellLowTemp | W::Level3));
        setyx(addr + n * i + 16, mw->getMdFtByF(i + 1, W::CellLowTemp | W::Level2));
        setyx(addr + n * i + 17, mw->getMdFtByF(i + 1, W::CellLowTemp | W::Level1));
        setyx(addr + n * i + 18, mw->getMdFtByF(i + 1, W::CellOverTemp | W::Level3));
        setyx(addr + n * i + 19, mw->getMdFtByF(i + 1, W::CellOverTemp | W::Level2));
        setyx(addr + n * i + 20, mw->getMdFtByF(i + 1, W::CellOverTemp | W::Level1));

        setyx(addr + n * i + 21, mw->getMdFtByF(i + 1, W::LowSoc | W::Level3));
        setyx(addr + n * i + 22, mw->getMdFtByF(i + 1, W::LowSoc | W::Level2));
        setyx(addr + n * i + 23, mw->getMdFtByF(i + 1, W::LowSoc | W::Level1));
        setyx(addr + n * i + 24, mw->getMdFtByF(i + 1, W::OverSoc | W::Level3));
        setyx(addr + n * i + 25, mw->getMdFtByF(i + 1, W::OverSoc | W::Level2));
        setyx(addr + n * i + 26, mw->getMdFtByF(i + 1, W::OverSoc | W::Level1));

        setyx(addr + n * i + 27, mw->getMdFtByF(i + 1, W::VoltDiff | W::Over | W::Level3));
        setyx(addr + n * i + 28, mw->getMdFtByF(i + 1, W::VoltDiff | W::Over | W::Level2));
        setyx(addr + n * i + 29, mw->getMdFtByF(i + 1, W::VoltDiff | W::Over | W::Level1));

        setyx(addr + n * i + 30, mw->getMdFtByF(i + 1, W::TempDiff | W::Over | W::Level3));
        setyx(addr + n * i + 31, mw->getMdFtByF(i + 1, W::TempDiff | W::Over | W::Level2));
        setyx(addr + n * i + 32, mw->getMdFtByF(i + 1, W::TempDiff | W::Over | W::Level1));

        setyx(addr + n * i + 33, mw->getMdFtByF(i + 1, W::Disconnect | W::Level1));
        setyx(addr + n * i + 34, p[i].heartbeat_bak ? 0 : 1);
        setyx(addr + n * i + 35, mw->getMdFtByF(i + 1, W::Sample | W::Level1));

        setyx(addr + n * i + 38, mw->getMdFtByF(i + 1, W::Precharge | W::Level1));
        setyx(addr + n * i + 39, mw->getMdFtByF(i + 1, W::Soh | W::Level1));
        setyx(addr + n * i + 40, mw->getMdFtByF(i + 1, W::PowerSupply | W::Level1));
        setyx(addr + n * i + 41, mw->getMdFtByF(i + 1, W::SelfCheck | W::Level1));
        setyx(addr + n * i + 42, mw->getMdFtByF(i + 1, W::PosRelay | W::Level1));
        setyx(addr + n * i + 43, mw->getMdFtByF(i + 1, W::NegRelay | W::Level1));
        setyx(addr + n * i + 44, p[i].fuseWire ? 1 : 0);
        setyx(addr + n * i + 45, p[i].breaker ? 1 : 0);
    }

    /****** remote control ******/
    addr = 0x7001;
//    qDebug() << "remote control:" << valc[0] << valc[1] << valc[2] << valc[3];
    for (int i = 0; i < NUMYAOKONG; ++i) {
        if (1 == getyk(i)) {
//            qWarning() << "*** EMS remote control addr:" << addr + i << "***";
            mw->hmi->setIO(i, true);
        }
    }
}

#ifdef XINLONG
void BgWorker::pcsDataProc(int id)
{
    bool rst = false;

//    QList<uint8_t> chargeCurrentList;
//    QList<uint8_t> dischargeCurrentList;

//    chargeCurrentList.clear();
//    dischargeCurrentList.clear();

//    if(mw->module[id*3].mainRelay)
//    {
//        chargeCurrentList.append(mw->module[id*3].chargeCurrentMax);
//        dischargeCurrentList.append(mw->module[id*3].dischargeCurrentMax);
//    }

//    if(mw->module[id*3+1].mainRelay)
//    {
//        chargeCurrentList.append(mw->module[id*3+1].chargeCurrentMax);
//        dischargeCurrentList.append(mw->module[id*3+1].dischargeCurrentMax);
//    }

//    if(mw->module[id*3+2].mainRelay)
//    {
//        chargeCurrentList.append(mw->module[id*3+2].chargeCurrentMax);
//        dischargeCurrentList.append(mw->module[id*3+2].dischargeCurrentMax);
//    }


//    if(chargeCurrentList.size() > 1)qSort(chargeCurrentList.begin(),chargeCurrentList.end());
//    if(dischargeCurrentList.size() > 1)qSort(dischargeCurrentList.begin(),dischargeCurrentList.end());

//    mw->pcs[id].data[1] = chargeCurrentList.size() > 0?(chargeCurrentList.at(0)*chargeCurrentList.size()*10):0;
//    mw->pcs[id].data[2] = dischargeCurrentList.size() > 0?(dischargeCurrentList.at(0)*dischargeCurrentList.size()*10):0;

//    rst = faultBit & (0x07 << (id*3));



    mw->pcs[id].data[1] = mw->module[id].mainRelay?mw->module[id].chargeCurrentMax*10:0;
    mw->pcs[id].data[2] = mw->module[id].mainRelay?mw->module[id].dischargeCurrentMax*10:0;
    mw->pcs[id].data[3] = mw->module[id].moduleSoc;

    rst = mw->isMdFt(id + 1);


    if(rst)
    {
        mw->pcs[id].data[0] = 0xAAAA; //fault
    }
    else if((mw->pcs[id].data[1] == 0) && mw->module[id].mainRelay)
    {
        mw->pcs[id].data[0] = 0x1111; //full
    }
    else if((mw->pcs[id].data[2] == 0) && mw->module[id].mainRelay)
    {
        mw->pcs[id].data[0] = 0x2222; //empty
    }
    else
    {
        mw->pcs[id].data[0] = 0xBBBB; //normal
    }


}
#else
void BgWorker::pcsDataProc(int id)
{
    QMutexLocker locker(&mw->HeartMutex);

    static int heartbeat = 0;
    static unsigned hasNonRecovFaultCount[PCS_NUM] = {0};
    bool hasNonRecovFault = false;
    bool hasCriticalAutoRecovFault = false;
    bool hasMainRelayOff = false;
    int chrgNodeNum = 0;
    unsigned nodeNum = 0;
    quint32 voltage = 0;
    quint32 current = 0;
    QList<quint8> chargeCurrentList;
    QList<quint8> dischargeCurrentList;
    QList<quint8> chargePowerList;
    QList<quint8> dischargePowerList;
    QList<quint16> socList;
    QList<quint16> sohList;
    QList<int> autoRecovFilter;
    QList<int> criticalAutoRecovFilter;

    chargeCurrentList.clear();
    dischargeCurrentList.clear();
    chargePowerList.clear();
    dischargePowerList.clear();
    chargeCurrentList.clear();
    dischargeCurrentList.clear();
    initAutoRecovFilter(&autoRecovFilter);
    initCriticalAutoRecovFilter(&criticalAutoRecovFilter);

    if (1 > PCS_NUM || MODULE_NUM < PCS_NUM) {
        return;
    }

    nodeNum = id + 1 < PCS_NUM
            ? MODULE_NUM / PCS_NUM
            : MODULE_NUM - (MODULE_NUM / PCS_NUM) * (PCS_NUM - 1);

    for (uint i = 0; i < nodeNum; ++i) {
        uint cur = (MODULE_NUM / PCS_NUM) * id + i;
        if (!(MODULE_NUM > cur) || !mw->module[cur].heartbeat_bak) {
            continue;
        }

//        if (!mw->pcs[id].checkList.contains(cur)) {
        if (!mw->checkList.contains(cur)) {
            continue;
        }

        hasNonRecovFault |= mw->isMdFtExcept(cur + 1 , autoRecovFilter);
        hasCriticalAutoRecovFault |= mw->isMdFt(cur + 1, criticalAutoRecovFilter);
        hasMainRelayOff |= mw->isMdMainRelayOff(cur);

        if (0x01 == mw->module[cur].chargeState) {
            ++chrgNodeNum;
        } else if (0x02 == mw->module[cur].chargeState) {
            --chrgNodeNum;
        }

        chargeCurrentList << mw->module[cur].chargeCurrentMax;
        dischargeCurrentList << mw->module[cur].dischargeCurrentMax;
        chargePowerList << mw->module[cur].chargePowerMax;
        dischargePowerList << mw->module[cur].dischargePowerMax;
        socList << mw->module[cur].moduleSoc;
        sohList << mw->module[cur].moduleSoh;

        voltage += mw->module[cur].moduleVoltage;
        current += mw->module[cur].moduleCurrent;
    }

//    qDebug() << chargeCurrentList;
//    qDebug() << dischargeCurrentList;
//    qDebug() << chargePowerList;
//    qDebug() << dischargePowerList;
    if (0 < socList.size()) {
        qSort(chargeCurrentList.begin(), chargeCurrentList.end());
        qSort(dischargeCurrentList.begin(), dischargeCurrentList.end());
        qSort(chargePowerList.begin(), chargePowerList.end());
        qSort(dischargePowerList.begin(), dischargePowerList.end());
        qSort(socList.begin(), socList.end());
        qSort(sohList.begin(), sohList.end());
    }

    if (chrgNodeNum > 0) {
        mw->pcs[id].isCharging = true;
    } else if (0 > chrgNodeNum) {
        mw->pcs[id].isCharging = false;
    }

//    if (mw->pcs[id].isCharging) {
//        mw->pcs[id].pileSoc = socList.isEmpty() ? 0 : socList.last();
//    } else {
//        mw->pcs[id].pileSoc = socList.isEmpty() ? 0 : socList.first();
//    }
    calcPileSoc(mw->pcs[id], socList);
//    qDebug() << __func__ << id + 1 << "soc" << mw->pcs[id].pileSoc[1] << mw->pcs[id].getPileSoc() << qAbs(mw->pcs[id].pileSoc[1] - mw->pcs[id].getPileSoc());

    // to make soc looks more comfortable
    for (unsigned i = 0; i < nodeNum; ++i) {
        const quint16 MIN_SOC = 0;
        const quint16 MAX_SOC = 100 * 10;
        unsigned cur = (MODULE_NUM / PCS_NUM) * id + i;
        qint8 drift = 0;
        qint16 moduleSoc = 0;
        Module *p = mw->module + cur;

        if (!(MODULE_NUM > cur)) {
            continue;
        }

        if (/*!p->heartbeat && */!p->heartbeat_bak) {
            continue;
        }

        drift = mw->pcs[id].isCharging
                ? 0 - mw->fictitiousSoc[cur].drift
                : mw->fictitiousSoc[cur].drift;

        moduleSoc = (qint16)mw->pcs[id].getPileSoc() + drift;
        if (MAX_SOC < moduleSoc) {
            moduleSoc = 2 * MAX_SOC - moduleSoc;
        }

        if (MIN_SOC > moduleSoc) {
            moduleSoc = 0 - moduleSoc;
        }

//        if (mw->pcs[id].isCharging && p->moduleSoc == socList.last() && 1 < socList.size()) {
//            moduleSoc = p->moduleSoc;
//        }

//        if (!mw->pcs[id].isCharging && p->moduleSoc == socList.first() && 1 < socList.size()) {
//            moduleSoc = p->moduleSoc;
//        }

        mw->fictitiousSoc[cur].soc = moduleSoc;
    }

    // dry contact to pcs
//    QTime t;
//    t.start();
    if (!mw->isEmergency) {
        mw->hmi->setIO(id + 4, (hasNonRecovFault && hasMainRelayOff));
    }
//    qDebug() << __func__ << "dry contact to pcs cost(ms)" << t.elapsed();

    // delay sending fault to pcs, make it more stable
#if 0
    if (hasNonRecovFault) {
        if (3 > hasNonRecovFaultCount[id]) {
            ++hasNonRecovFaultCount[id];
        }
    } else {
        hasNonRecovFaultCount[id] = 0;
    }

    if (3 > hasNonRecovFaultCount[id]) {
        hasNonRecovFault = false;
    }
#else
    hasNonRecovFault = checkCount(hasNonRecovFault, hasNonRecovFaultCount[id], 5);
#endif

    // 0 <-> 1
    if (0 == id) {
        heartbeat ^= 1;
    }

    qDebug() << __func__ << id << hasNonRecovFault << hasCriticalAutoRecovFault << hasNonRecovFaultCount[id];
    qMemSet(mw->pcs[id].data, 0x00, sizeof(mw->pcs[id].data));
    mw->pcs[id].data[0] = chargeCurrentList.isEmpty() ? 0 : chargeCurrentList.first() * chargeCurrentList.size() * 10;
    mw->pcs[id].data[1] = dischargeCurrentList.isEmpty() ? 0 : dischargeCurrentList.first() * dischargeCurrentList.size() * 10;
    mw->pcs[id].data[2] = socList.isEmpty() ? 0 : voltage / socList.size();
    mw->pcs[id].data[3] = current;
//    mw->pcs[id].data[4] = socList.isEmpty() ? 0 : qRound(socList.first() / 10.0f);
    mw->pcs[id].data[4] = qRound(mw->pcs[id].getPileSoc() / 10.0f);
    mw->pcs[id].data[5] = sohList.isEmpty() ? 0 : qRound(sohList.first() / 10.0f);
    mw->pcs[id].data[6] = (chargePowerList.isEmpty() || 0 == mw->pcs[id].data[0]) ? 0 : chargePowerList.first() * chargePowerList.size() * 10;
    mw->pcs[id].data[7] = (dischargePowerList.isEmpty() || 0 == mw->pcs[id].data[1]) ? 0 : dischargePowerList.first() * dischargePowerList.size() * 10;
    mw->pcs[id].data[10] |= (hasNonRecovFault && hasMainRelayOff) ? 0x01 : 0;
    mw->pcs[id].data[10] |= (hasNonRecovFault || hasCriticalAutoRecovFault || 0 == mw->pcs[id].data[0]) ? 0x01 << 1 : 0;
    mw->pcs[id].data[10] |= (hasNonRecovFault || hasCriticalAutoRecovFault || 0 == mw->pcs[id].data[1]) ? 0x01 << 2 : 0;
    mw->pcs[id].data[10] |= mw->dcSideSwitch[id] ? 0x01 << 3 : 0;
    mw->pcs[id].data[10] |= 0x01 << 4;
    mw->pcs[id].data[10] |= (0 == mw->pcs[id].data[0]) ? (0x01 << 5) : 0;
    mw->pcs[id].data[10] |= (0 == mw->pcs[id].data[1]) ? (0x01 << 6) : 0;
    mw->pcs[id].data[10] |= heartbeat << 15;
    mw->pcs[id].data[11] = 0x0000;
    // voltage, current oversize?
    // data integrety
    // peripheral data shrink?
    // insulation res detected by module
    // eth0 eth1 ip settings
    // cancel debug print in IEC104
}
#endif

bool BgWorker::isMaxAllowedCurrentZero(unsigned m, bool isCharging)
{
    return 0 == (isCharging ? mw->module[m].chargeCurrentMax : mw->module[m].dischargeCurrentMax);
}

void BgWorker::fdbk2BMSOne(bool *hasChrgZero
                           , bool *hasDischrgZero
                           , bool *hasChrgCurrent
                           , bool *hasDischrgCurrent)
{
    QMutexLocker locker(&mw->HeartMutex);

    if (1 > PCS_NUM || MODULE_NUM < PCS_NUM) {
        return;
    }

    for (unsigned i = 0; i < PCS_NUM; ++i) {
        bool allOnLine = true;
        unsigned nodeNum = i + 1 < PCS_NUM
                ? MODULE_NUM / PCS_NUM
                : MODULE_NUM - (MODULE_NUM / PCS_NUM) * (PCS_NUM - 1);

        hasChrgZero[i] = false;
        hasDischrgZero[i] = false;
        for (unsigned j = 0; j < nodeNum; ++j) {
            unsigned cur = (MODULE_NUM / PCS_NUM) * i + j;
            if (!(MODULE_NUM > cur) || !mw->module[cur].heartbeat_bak) {
                allOnLine = false;
                continue;
            }

            if (isMaxAllowedCurrentZero(cur, true)) {
                hasChrgZero[i] = true;
//                qDebug() << __func__ << "module" << cur + 1 << "has zero chrg current";
            }

            if (isMaxAllowedCurrentZero(cur, false)) {
                hasDischrgZero[i] = true;
//                qDebug() << __func__ << "module" << cur + 1 << "has zero dischrg current";
            }
        }

        if (!allOnLine) {
            continue;
        }

//        qDebug() << __func__ << i + 1 << hasChrgZero[i] << hasDischrgCurrent[i];
//        qDebug() << __func__ << i + 1 << hasDischrgZero[i] << hasChrgCurrent[i];
        for (unsigned j = 0; j < nodeNum; ++j) {
            unsigned addr = (MODULE_NUM / PCS_NUM) * i + j + 1;
            quint8 data[8] = {0x00};

            if (hasChrgZero[i]) {
                data[5] = 0x01;
            }

            if (hasDischrgCurrent[i]) {
                data[5] = 0x00;
            }

            data[0] = 0x70;
            data[1] = 47;
            writeCan(data, 8, CAN::BAMS_SEND_CALIBRATION_PARA << 4 | addr);
            usleep(1000);

            qMemSet(data, 0x00, sizeof(data));
            if (hasDischrgZero[i]) {
                data[5] = 0x01;
            }

            if (hasChrgCurrent[i]) {
                data[5] = 0x00;
            }

            data[0] = 0x70;
            data[1] = 48;
            writeCan(data, 8, CAN::BAMS_SEND_CALIBRATION_PARA << 4 | addr);
            usleep(1000);
        }
    }
}

bool BgWorker::groupHasCurrent(unsigned id, bool isCharging)
{
    bool ret = true;
    const quint8 currentThreshold = 100;        // 10Amper

    if (1 > PCS_NUM || MODULE_NUM < PCS_NUM) {
        return false;
    }

    for (unsigned i = 0; i < PCS_NUM; ++i) {
        unsigned nodeNum = i + 1 < PCS_NUM
                ? MODULE_NUM / PCS_NUM
                : MODULE_NUM - (MODULE_NUM / PCS_NUM) * (PCS_NUM - 1);

        if (id != i) {
            break;
        }

        for (unsigned j = 0; j < nodeNum; ++j) {
            qint16 current = 0;
            unsigned cur = (MODULE_NUM / PCS_NUM) * i + j;
            if (!(MODULE_NUM > cur) || !mw->module[cur].heartbeat_bak) {
                ret = false;
                break;
            }

            current = mw->module[cur].moduleCurrent;
            if (isCharging && 0 - currentThreshold < current) {
                ret = false;
                break;
            }

            if (!isCharging && currentThreshold > current) {
                ret = false;
                break;
            }
        }
    }

//    qDebug() << __func__ << "pile" << id + 1 << (isCharging ? "ischarging" : "isdischarging") << ret;
    return ret;
}

void BgWorker::feedBack2BMS()       // run this function every second
{
    const quint8 fdbkOneThreshold = 5;
    const quint8 fdbkZeroThreshold = 10;

    static quint8 fdbkOneCount = 0;
    static bool hasChrgZero[PCS_NUM] = {false};
    static bool hasDischrgZero[PCS_NUM] = {false};
    static bool hasChrgCurrent[PCS_NUM] = {false};
    static bool hasDischrgCurrent[PCS_NUM] = {false};
    static bool _hasChrgCurrent[PCS_NUM] = {true};
    static bool _hasDischrgCurrent[PCS_NUM] = {true};
    static QDateTime chrgZeroDetected[PCS_NUM];
    static QDateTime dischrgZeroDetected[PCS_NUM];

    /* threshold arrived, reset cound */
    if (fdbkOneThreshold - 1 < fdbkOneCount) {
        fdbkOneCount = 0;
        fdbk2BMSOne(hasChrgZero, hasDischrgZero, hasChrgCurrent, hasDischrgCurrent);
    }

    for (unsigned i = 0; i < PCS_NUM; ++i) {
        QDateTime now = QDateTime::currentDateTime();

        if (!chrgZeroDetected[i].isNull() && fdbkZeroThreshold < chrgZeroDetected[i].secsTo(now)) {
            const QDateTime tmp;
            chrgZeroDetected[i] = tmp;
            hasDischrgCurrent[i] = _hasDischrgCurrent[i];
            qDebug() << __func__ << "pile" << i + 1 << "***hasDischrgCurrent" << hasDischrgCurrent[i];
        }

        if (!dischrgZeroDetected[i].isNull() && fdbkZeroThreshold < dischrgZeroDetected[i].secsTo(now)) {
            const QDateTime tmp;
            dischrgZeroDetected[i] = tmp;
            hasChrgCurrent[i] = _hasChrgCurrent[i];
        }

        if (hasChrgZero[i] && chrgZeroDetected[i].isNull()) {
            chrgZeroDetected[i] = QDateTime::currentDateTime();
            _hasDischrgCurrent[i] = true;
//            hasDischrgCurrent[i] = false;
            qDebug() << __func__ << "pile" << i + 1 << "***hasChrgZero" << hasChrgZero[i];
        }

        if (hasDischrgZero[i] && dischrgZeroDetected[i].isNull()) {
            dischrgZeroDetected[i] = QDateTime::currentDateTime();
            _hasChrgCurrent[i] = true;
//            hasChrgCurrent[i] = false;
        }

        _hasChrgCurrent[i] = _hasChrgCurrent[i] && groupHasCurrent(i, true);
        _hasDischrgCurrent[i] = _hasDischrgCurrent[i] && groupHasCurrent(i, false);
    }

    /* increse counts */
    ++fdbkOneCount;
}

void BgWorker::initAutoRecovFilter(QList<int> *filter)
{
    (*filter).clear();

    *filter << (W::Module | W::Over | W::TempDiff | W::Level1)
            << (W::Module | W::Over | W::VoltDiff | W::Level1)
            << (W::Module | W::LowSoc | W::Level1)
            << (W::Module | W::Over | W::AmbientTemp | W::Level1)
            << (W::Module | W::Low | W::AmbientTemp | W::Level1)
            << (W::Module | W::Soh | W::Level2)
            << (W::Module | W::Disconnect | W::Level1)
            << (W::Module | W::Sample | W::Level1);
}

void BgWorker::initCriticalAutoRecovFilter(QList<int> *filter)
{
    (*filter).clear();

    *filter << (W::Module | W::Disconnect | W::Level1)
            << (W::Module | W::Sample | W::Level1);
}

void BgWorker::checkFault()
{
    QMutexLocker locker(&mw->HeartMutex);
    QMutexLocker locker2(&mw->MapMutex);

    faultBit = 0;
    quint16 *buf = new quint16[MODULE_NUM]();

    if(mw->fault.size() > 0)
    {
        QMap<int, int>::iterator i;

        for (i = mw->fault.begin(); i != mw->fault.end(); ++i)
        {
            int module = ((i.key() & 0xFF0000) >> 16)-1;
//            int pack = ((i.key() & 0xFF00) >> 8)-1;
            int code = (i.key() & 0xFF);

            if(code == 0x0D || mw->module[module].forceMode)continue;

            //write bit
            faultBit |= 0x01 << module;

            //write array
            buf[module] |= 0x01 << code;
        }
    }


    for(uint i = 0;i < MODULE_NUM;i++)
    {
        if(!mw->module[i].heartbeat_bak){

            //write bit
            faultBit |= 0x01 << i;

            //write array
            buf[i] |= 0x01;
        }
    }

    memcpy(mw->faultArray,buf,sizeof(*buf));
    delete[] buf;
}

void BgWorker::modbusDataMap()
{
//    QMutexLocker locker(&mw->MapMutex);

    QList<quint8> chargeCurrentList;
    QList<quint16> socList;
    processData();
//    qDebug() << __func__ << mw->fault_bak;

#ifdef XINYI_EMS
    for (uint i = 0; i < MODULE_NUM; ++i) {
        chargeCurrentList << mw->module[i].chargeCurrentMax;
        socList << mw->module[i].moduleSoc;
    }
    qSort(chargeCurrentList.begin(), chargeCurrentList.end());
    qSort(socList.begin(), socList.end());

    // heap
    if (1 == mw->systemType) {      // UPS_A
        mw->mb_mapping[0]->tab_input_registers[0] = qRound(socList.first() / 10.0f);
        mw->mb_mapping[0]->tab_input_registers[1] = 100;

        mw->mb_mapping[0]->tab_input_registers[2] = qRound(mw->heap.bat_voltage_min / 100);
        mw->mb_mapping[0]->tab_input_registers[3] = qRound(mw->heap.bat_voltage_max / 100);
        mw->mb_mapping[0]->tab_input_registers[4] = mw->heap.bat_temp_min;
        mw->mb_mapping[0]->tab_input_registers[5] = mw->heap.bat_temp_max;

        mw->mb_mapping[0]->tab_input_registers[6] = mw->heap.chargeableQ;
        mw->mb_mapping[0]->tab_input_registers[7] = mw->heap.chargeableQ >> 16;

        mw->mb_mapping[0]->tab_input_registers[8] = mw->heap.remainQ;
        mw->mb_mapping[0]->tab_input_registers[9] = mw->heap.remainQ >> 16;

        mw->mb_mapping[0]->tab_input_registers[10] = 0;     // buzzer
        mw->mb_mapping[0]->tab_input_registers[11] = chargeCurrentList.isEmpty()
                ? 0 : chargeCurrentList.first() * MODULE_NUM * 10;
    } else {
        mw->mb_mapping[0]->tab_input_registers[0] = mw->heap.soc;
        mw->mb_mapping[0]->tab_input_registers[1] = mw->heap.ir;
        mw->mb_mapping[0]->tab_input_registers[2] = mw->heap.voltage;
        mw->mb_mapping[0]->tab_input_registers[3] = mw->heap.current;

        mw->mb_mapping[0]->tab_input_registers[4] = mw->heap.bat_voltage_max;
        mw->mb_mapping[0]->tab_input_registers[5] = mw->heap.bat_voltage_min;
        mw->mb_mapping[0]->tab_input_registers[6] = mw->heap.bat_temp_max;
        mw->mb_mapping[0]->tab_input_registers[7] = mw->heap.bat_temp_min;

        mw->mb_mapping[0]->tab_input_registers[8] = mw->heap.chargeableQ;
        mw->mb_mapping[0]->tab_input_registers[9] = mw->heap.chargeableQ >> 16;

        mw->mb_mapping[0]->tab_input_registers[10] = mw->heap.remainQ;
        mw->mb_mapping[0]->tab_input_registers[11] = mw->heap.remainQ >> 16;
    }

    // module
    for (unsigned i = 0; i < MODULE_NUM; ++i) {
        // input bits
        mw->mb_mapping[i + 1]->tab_input_bits[0] = mw->getMdFtByF(i + 1, W::CellOverVoltage | W::Level1);
        mw->mb_mapping[i + 1]->tab_input_bits[1] = mw->getMdFtByF(i + 1, W::CellLowVoltage | W::Level1);
        mw->mb_mapping[i + 1]->tab_input_bits[2] = mw->getMdFtByF(i + 1, W::CellChargingOverTemp | W::Level1);
        mw->mb_mapping[i + 1]->tab_input_bits[3] = mw->getMdFtByF(i + 1, W::CellChargingLowTemp | W::Level1);
        mw->mb_mapping[i + 1]->tab_input_bits[4] = mw->getMdFtByF(i + 1, W::ModuleOverCurrent | W::Level1);
        mw->mb_mapping[i + 1]->tab_input_bits[5] = mw->getMdFtByF(i + 1, W::InsulationRes | W::Level1);
        mw->mb_mapping[i + 1]->tab_input_bits[6] = mw->getMdFtByF(i + 1, W::ModuleOverVoltage | W::Level1);
        mw->mb_mapping[i + 1]->tab_input_bits[7] = mw->getMdFtByF(i + 1, W::ModuleLowVoltage | W::Level1);
        mw->mb_mapping[i + 1]->tab_input_bits[8] = mw->getMdFtByF(i + 1, W::PosRelay | W::Level1);
        mw->mb_mapping[i + 1]->tab_input_bits[9] = mw->getMdFtByF(i + 1, W::Sample | W::Level1);
        mw->mb_mapping[i + 1]->tab_input_bits[10] = mw->getMdFtByF(i + 1, W::Disconnect | W::Level1);
        mw->mb_mapping[i + 1]->tab_input_bits[11] = 0;

        // input registers
        mw->mb_mapping[i+1]->tab_input_registers[0] = mw->module[i].chargeState;
        mw->mb_mapping[i+1]->tab_input_registers[1] = mw->module[i].moduleSoc;
        mw->mb_mapping[i+1]->tab_input_registers[2] = mw->module[i].moduleVoltage;
        mw->mb_mapping[i+1]->tab_input_registers[3] = mw->module[i].moduleCurrent;

        mw->mb_mapping[i+1]->tab_input_registers[4] = PACK_NUM * BAT_NUM;
        mw->mb_mapping[i+1]->tab_input_registers[5] = PACK_NUM * TEMP_NUM;
        mw->mb_mapping[i+1]->tab_input_registers[6] = mw->module[i].minV;
        mw->mb_mapping[i+1]->tab_input_registers[7] = mw->module[i].maxV;
        mw->mb_mapping[i+1]->tab_input_registers[8] = mw->module[i].minT;
        mw->mb_mapping[i+1]->tab_input_registers[9] = mw->module[i].maxT;

        mw->mb_mapping[i+1]->tab_input_registers[10] = mw->module[i].minVID + 1;
        mw->mb_mapping[i+1]->tab_input_registers[11] = mw->module[i].maxVID + 1;
        mw->mb_mapping[i+1]->tab_input_registers[12] = mw->module[i].minTID + 1;
        mw->mb_mapping[i+1]->tab_input_registers[13] = mw->module[i].maxTID + 1;

        mw->mb_mapping[i+1]->tab_input_registers[14] = mw->module[i].chargeableQ;
        mw->mb_mapping[i+1]->tab_input_registers[15] = mw->module[i].chargeableQ >> 16;

        mw->mb_mapping[i+1]->tab_input_registers[16] = mw->module[i].remainQ;
        mw->mb_mapping[i+1]->tab_input_registers[17] = mw->module[i].remainQ >> 16;

        mw->mb_mapping[i+1]->tab_input_registers[18] = (uint32_t)mw->quantity[i].singleChargeQuantity;
        mw->mb_mapping[i+1]->tab_input_registers[19] = (uint32_t)mw->quantity[i].singleChargeQuantity >> 16;

        mw->mb_mapping[i+1]->tab_input_registers[20] = (uint32_t)mw->quantity[i].singleDischargeQuantity;
        mw->mb_mapping[i+1]->tab_input_registers[21] = (uint32_t)mw->quantity[i].singleDischargeQuantity >> 16;

        mw->mb_mapping[i+1]->tab_input_registers[22] = (int32_t)mw->quantity[i].totalCharge;
        mw->mb_mapping[i+1]->tab_input_registers[23] = (int32_t)mw->quantity[i].totalCharge >> 16;

        mw->mb_mapping[i+1]->tab_input_registers[24] = (int32_t)mw->quantity[i].totalDischarge;
        mw->mb_mapping[i+1]->tab_input_registers[25] = (int32_t)mw->quantity[i].totalDischarge >> 16;

        mw->mb_mapping[i+1]->tab_input_registers[26] = mw->module[i].mainRelay;
        mw->mb_mapping[i+1]->tab_input_registers[27] = mw->module[i].totalNegativeRelay;


        for (int j = 0; j < PACK_NUM * BAT_NUM; j++) {
            mw->mb_mapping[i + 1]->tab_input_registers[j + 100] = mw->module[i].voltage[j];
        }

        for (int j = 0; j < PACK_NUM * TEMP_NUM; j++) {
            mw->mb_mapping[i + 1]->tab_input_registers[j + 500] = mw->module[i].temp[j];
        }

        // holding registers
        mw->mb_mapping[i + 1]->tab_registers[0] = mw->module[i].mainRelay ? mw->module[i].chargeCurrentMax * 10 : 0;
        mw->mb_mapping[i + 1]->tab_registers[1] = mw->module[i].mainRelay ? mw->module[i].dischargeCurrentMax * 10 : 0;
    }
#else
    //heap
    mw->mb_mapping[0]->tab_input_registers[502] = mw->heap.soc/10;
    mw->mb_mapping[0]->tab_input_registers[503] = 100;
    mw->mb_mapping[0]->tab_input_registers[510] = mw->heap.bat_voltage_max;
    mw->mb_mapping[0]->tab_input_registers[513] = mw->heap.bat_voltage_min;
    mw->mb_mapping[0]->tab_input_registers[516] = (mw->heap.bat_temp_max/10) + 40;
    mw->mb_mapping[0]->tab_input_registers[519] = (mw->heap.bat_temp_min/10) + 40;

    mw->mb_mapping[0]->tab_input_registers[530] = mw->heap.chargeableQ;
    mw->mb_mapping[0]->tab_input_registers[531] = mw->heap.chargeableQ >> 16;

    mw->mb_mapping[0]->tab_input_registers[532] = mw->heap.remainQ;
    mw->mb_mapping[0]->tab_input_registers[533] = mw->heap.remainQ >> 16;

    //module
    for(int i = 0;i < MODULE_NUM;i++)
    {
        //input_bits
        mw->mb_mapping[i+1]->tab_input_bits[0] = (getBit(mw->faultArray[i],8) << 2);

        mw->mb_mapping[i+1]->tab_input_bits[1] = (getBit(mw->faultArray[i],6) << 0);

        mw->mb_mapping[i+1]->tab_input_bits[3] = (getBit(mw->faultArray[i],7) << 1) | (getBit(mw->faultArray[i],5) << 4);

        mw->mb_mapping[i+1]->tab_input_bits[5] = (getBit(mw->faultArray[i],1) << 0) | (getBit(mw->faultArray[i],2) << 3);

        mw->mb_mapping[i+1]->tab_input_bits[6] = (getBit(mw->faultArray[i],3) << 1) | (getBit(mw->faultArray[i],4) << 4);

        //input_registers
        mw->mb_mapping[i+1]->tab_input_registers[0] = mw->module[i].mainRelay;
        mw->mb_mapping[i+1]->tab_input_registers[1] = mw->module[i].mainRelay;
        mw->mb_mapping[i+1]->tab_input_registers[5] = PACK_NUM*BAT_NUM;

        mw->mb_mapping[i+1]->tab_input_registers[7] = mw->module[i].moduleVoltage;
        mw->mb_mapping[i+1]->tab_input_registers[8] = mw->module[i].moduleCurrent;
        mw->mb_mapping[i+1]->tab_input_registers[10] = mw->module[i].insulationRes;
        mw->mb_mapping[i+1]->tab_input_registers[14] = mw->module[i].moduleSOC/10;

        mw->mb_mapping[i+1]->tab_input_registers[24] = mw->module[i].maxV;
        mw->mb_mapping[i+1]->tab_input_registers[25] = mw->module[i].maxVID+1;
        mw->mb_mapping[i+1]->tab_input_registers[26] = mw->module[i].minV;
        mw->mb_mapping[i+1]->tab_input_registers[27] = mw->module[i].minVID+1;

        mw->mb_mapping[i+1]->tab_input_registers[30] = (mw->module[i].maxT/10) + 40;
        mw->mb_mapping[i+1]->tab_input_registers[31] = mw->module[i].maxTID+1;
        mw->mb_mapping[i+1]->tab_input_registers[32] = (mw->module[i].minT/10) + 40;
        mw->mb_mapping[i+1]->tab_input_registers[33] = mw->module[i].minTID+1;

        mw->mb_mapping[i+1]->tab_input_registers[46] = PACK_NUM*TEMP_NUM;
        mw->mb_mapping[i+1]->tab_input_registers[47] = mw->module[i].chargeState;

        mw->mb_mapping[i+1]->tab_input_registers[149] = (uint32_t)(mw->quantity[i].singleChargeQuantity*10);
        mw->mb_mapping[i+1]->tab_input_registers[150] = ((uint32_t)(mw->quantity[i].singleChargeQuantity*10)) >> 16;

        mw->mb_mapping[i+1]->tab_input_registers[151] = (uint32_t)(mw->quantity[i].singleDischargeQuantity*10);
        mw->mb_mapping[i+1]->tab_input_registers[152] = ((uint32_t)(mw->quantity[i].singleDischargeQuantity*10)) >> 16;

        mw->mb_mapping[i+1]->tab_input_registers[153] = (uint32_t)(mw->module[i].chargeableQ*10);
        mw->mb_mapping[i+1]->tab_input_registers[154] = ((uint32_t)(mw->module[i].chargeableQ*10)) >> 16;

        mw->mb_mapping[i+1]->tab_input_registers[155] = (uint32_t)(mw->module[i].remainQ*10);
        mw->mb_mapping[i+1]->tab_input_registers[156] = ((uint32_t)(mw->module[i].remainQ*10)) >> 16;

        for(int j = 0;j < PACK_NUM*BAT_NUM;j++)
        {
            mw->mb_mapping[i+1]->tab_input_registers[j+1000] = mw->module[i].voltage[j];
        }
        for(int j = 0;j < PACK_NUM*TEMP_NUM;j++)
        {
            mw->mb_mapping[i+1]->tab_input_registers[j+1400] = mw->module[i].temp[j];
        }
    }
#endif
}

int BgWorker::getBit(uint16_t data,uint8_t bit)
{
    if(bit < 16)
        return (data & (0x01 << bit))?1:0;
    else
        return -1;

}

void BgWorker::processData()
{
    QMutexLocker locker(&mw->HeartMutex);
    quint16 pileSocSum = 0;

    QList<uint16_t> list_soc;
    QList<uint16_t> list_soh;
    QList<uint16_t> list_soe;
    QList<uint16_t> list_ir;
    QList<int16_t> list_voltage;
    QList<int16_t> list_temp;

    mw->heap.soc = 0;
    mw->heap.ir = 0;
    mw->heap.bat_voltage_min = 0;
    mw->heap.bat_voltage_max = 0;
    mw->heap.bat_temp_min = 0;
    mw->heap.bat_temp_max = 0;
    mw->heap.remainQ = 0;
    mw->heap.chargeableQ = 0;
    mw->heap.voltage = 0;
    mw->heap.current = 0;
    mw->heap.chargingCapacity = 0;
    mw->heap.dischargingCapacity = 0;

    // module
    for(uint i = 0;i < MODULE_NUM;i++)
    {
        if (!mw->module[i].heartbeat_bak) {
            continue;
        }

//        calcBatteryExtremum(i);

        list_soc.append(mw->module[i].moduleSoc);
        list_soh.append(mw->module[i].moduleSoh);
        list_soe.append(mw->module[i].moduleSoe);
        list_ir.append(mw->module[i].insulationRes);
        list_voltage.append(mw->module[i].minV);
        list_voltage.append(mw->module[i].maxV);
        list_temp.append(mw->module[i].minT);
        list_temp.append(mw->module[i].maxT);

        mw->module[i].remainQ = (Q_MAX*mw->module[i].moduleSoc)/10000;
        mw->module[i].chargeableQ = Q_MAX/10 - mw->module[i].remainQ;

        mw->heap.remainQ += mw->module[i].remainQ;
        mw->heap.chargeableQ += mw->module[i].chargeableQ;

        mw->heap.voltage += mw->module[i].moduleVoltage;
        mw->heap.current += mw->module[i].moduleCurrent;

        mw->heap.chargingCapacity += mw->module[i].chargingCapacity;
        mw->heap.dischargingCapacity += mw->module[i].dischargingCapacity;
    }

    // pile
    for (int i = 0; i < PCS_NUM; ++i) {
        qDebug() << __func__ << i + 1 << mw->pcs[i].getPileSoc();
        pileSocSum += mw->pcs[i].getPileSoc();
    }

    // heap
    if(list_soc.count() > 0){

        if(list_soc.count() > 1)
        {
            qSort(list_soc.begin(),list_soc.end());
            qSort(list_soh.begin(), list_soh.end());
            qSort(list_soe.begin(), list_soe.end());
            qSort(list_ir.begin(),list_ir.end());
            qSort(list_voltage.begin(),list_voltage.end());
            qSort(list_temp.begin(),list_temp.end());
        }

//        mw->heap.soc = list_soc.at(0);
        mw->heap.soc = qRound(pileSocSum * 1.0f / PCS_NUM);
        mw->heap.soh = list_soh.at(0);
        mw->heap.soe = list_soe.at(0);
        mw->heap.ir = list_ir.at(0);

        mw->heap.bat_voltage_min = list_voltage.first();
        mw->heap.bat_voltage_max = list_voltage.last();
        mw->heap.bat_temp_min = list_temp.first();
        mw->heap.bat_temp_max = list_temp.last();

        mw->heap.voltage = mw->heap.voltage/list_soc.count();
    }
}

/*void BgWorker::checkWarning()
{
    QMutexLocker locker(&mw->MapMutex);
    mw->warning.clear();

    int key,value;
    for (int id = 0; id < MODULE_NUM; ++id) {

        Module *m = mw->module + id;
        Para *p = mw->para + id;

        if(!m->heartbeat_bak)continue;

        //soc
        key = 0;
        do {
            if(m->moduleSOC > p->SOC_high_limit_1.toFloat()*10){
                key = ((id+1) << 16) + SOC_HIGH_1;
                break;
            }

            if(m->moduleSOC > p->SOC_high_limit_2.toFloat()*10){
                key = ((id+1) << 16) + SOC_HIGH_2;
                break;
            }

            if(m->moduleSOC > p->SOC_high_limit_3.toFloat()*10){
                key = ((id+1) << 16) + SOC_HIGH_3;
                break;
            }

            if(m->moduleSOC < p->SOC_low_limit_1.toFloat()*10){
                key = ((id+1) << 16) + SOC_LOW_1;
                break;
            }

            if(m->moduleSOC < p->SOC_low_limit_2.toFloat()*10){
                key = ((id+1) << 16) + SOC_LOW_2;
                break;
            }

            if(m->moduleSOC < p->SOC_low_limit_3.toFloat()*10){
                key = ((id+1) << 16) + SOC_LOW_3;
                break;
            }
        } while (0);

        if(key)mw->warning.insert(key,1);

        //module current
        key = 0;
//        qDebug() << "***";
//        qDebug() << __func__ << "*module over current*"
//                 << "L1(c):" << p->charge_over_current.toFloat()*10
//                 << "L1(d):" << p->discharge_over_current.toFloat()*10
//                 << "L2:" << p->module_over_current_2.toFloat()*10
//                 << "L3:" << p->module_over_current_3.toFloat()*10;
        do {
            if (0 < mw->getModuleFaultValueByCode(id + 1, MODULE_OVER_CURRENT)) {
                break;
            }

            if (qAbs(m->moduleCurrent) > p->module_over_current_2.toFloat()*10) {
                key = ((id+1) << 16) + MODULE_OVER_CURRENT_2;
                break;
            }

            if (qAbs(m->moduleCurrent) > p->module_over_current_3.toFloat()*10) {
                key = ((id+1) << 16) + MODULE_OVER_CURRENT_3;
                break;
            }

        } while (0);
//        qDebug() << __func__ << "loop break with:" << qAbs(m->moduleCurrent)
//                 << "key:" << QString::number(key, 16);

        if(key)mw->warning.insert(key,1);

        //module insulation
        key = 0;
        do {
            if(0 < mw->getModuleFaultValueByCode(id + 1, MODULE_INSULATION)){
                break;
            }

            if(m->insulationRes < p->insulation_limit_2.toFloat()){
                key = ((id+1) << 16) + MODULE_INSULATION_2;
                break;
            }

            if(m->insulationRes < p->insulation_limit_3.toFloat()){
                key = ((id+1) << 16) + MODULE_INSULATION_3;
                break;
            }

        } while (0);

        if(key)mw->warning.insert(key,1);

        //module voltage
        key = 0;
        do {
            if(0 < mw->getModuleFaultValueByCode(id + 1, MODULE_OVER_VOLTAGE)){
                break;
            }

            if(m->moduleVoltage > p->module_over_voltage_2.toFloat()*10){
                key = ((id+1) << 16) + MODULE_OVER_VOLTAGE_2;
                break;
            }

            if(m->moduleVoltage > p->module_over_voltage_3.toFloat()*10){
                key = ((id+1) << 16) + MODULE_OVER_VOLTAGE_3;
                break;
            }

            if(0 < mw->getModuleFaultValueByCode(id + 1, MODULE_UNDER_VOLTAGE)){
                break;
            }

            if(m->moduleVoltage < p->module_under_voltage_2.toFloat()*10){
                key = ((id+1) << 16) + MODULE_UNDER_VOLTAGE_2;
                break;
            }

            if(m->moduleVoltage < p->module_under_voltage_3.toFloat()*10){
                key = ((id+1) << 16) + MODULE_UNDER_VOLTAGE_3;
                break;
            }

        } while (0);

        if(key)mw->warning.insert(key,1);

        //voltage consistency
        key = 0;
        do {

            if(abs(m->maxV-m->minV) > p->voltage_consistency_2.toFloat()){
                key = ((id+1) << 16) + VOLTAGE_CONSISTENCY_2;
                break;
            }

            if(abs(m->maxV-m->minV) > p->voltage_consistency_3.toFloat()){
                key = ((id+1) << 16) + VOLTAGE_CONSISTENCY_3;
                break;
            }

        } while (0);

        if(key)mw->warning.insert(key,1);

        //temp consistency
        key = 0;
        do {

            if(abs(m->maxT-m->minT) > p->temp_consistency_2.toFloat()*10){
                key = ((id+1) << 16) + TEMP_CONSISTENCY_2;
                break;
            }

            if(abs(m->maxT-m->minT) > p->temp_consistency_3.toFloat()*10){
                key = ((id+1) << 16) + TEMP_CONSISTENCY_3;
                break;
            }

        } while (0);

        if(key)mw->warning.insert(key,1);

        //cell under voltage
        key = 0;
        if (0 == mw->getModuleFaultValueByCode(id + 1, UNDER_VOLTAGE)) {
            for (int i = 0; i < PACK_NUM*BAT_NUM; ++i) {
                if(m->voltage[i] < p->under_voltage_2.toFloat()*1000){
                    key = ((id+1) << 16) + ((i/BAT_NUM+1) << 8) + UNDER_VOLTAGE_2;
                    value = i%BAT_NUM + 1;
                    break;
                }

                if(m->voltage[i] < p->under_voltage_3.toFloat()*1000){
                    key = ((id+1) << 16)+ ((i/BAT_NUM+1) << 8) + UNDER_VOLTAGE_3;
                    value = i%BAT_NUM + 1;
                    break;
                }
            }
        }

        if(key)mw->warning.insert(key,value);

        //cell over voltage
        key = 0;
        if (0 == mw->getModuleFaultValueByCode(id + 1, OVER_VOLTAGE)) {
            for (int i = 0; i < PACK_NUM*BAT_NUM; ++i) {
                if (m->voltage[i] > p->over_voltage_2.toFloat()*1000) {
                    key = ((id+1) << 16) + ((i/BAT_NUM+1) << 8) + OVER_VOLTAGE_2;
                    value = i%BAT_NUM + 1;
                    break;
                }

                if (m->voltage[i] > p->over_voltage_3.toFloat()*1000) {
                    key = ((id+1) << 16)+ ((i/BAT_NUM+1) << 8) + OVER_VOLTAGE_3;
                    value = i%BAT_NUM + 1;
                    break;
                }
            }
        }

        if(key)mw->warning.insert(key,value);

        //cell under temp
//        qDebug() << "***";
//        qDebug() << __func__ << "*cell under temp*"
//                 << "L1:" << p->charge_under_temp.toFloat()*10
//                 << "L2" << p->under_temp_2.toFloat()*10
//                 << "L3" << p->under_temp_3.toFloat()*10;
        key = 0;
        int i = 0;
        if (0 == mw->getModuleFaultValueByCode(id + 1, CHARGE_UNDER_TEMP)
                || 0 == mw->getModuleFaultValueByCode(id + 1, DISCHARGE_UNDER_TEMP)) {
            for (i = 0; i < PACK_NUM*TEMP_NUM; ++i) {
                if(m->temp[i] < p->under_temp_2.toFloat()*10){
                    key = ((id+1) << 16) + ((i/TEMP_NUM+1) << 8) + UNDER_TEMP_2;
                    value = i%TEMP_NUM + 1;
                    break;
                }

                if(m->temp[i] < p->under_temp_3.toFloat()*10){
                    key = ((id+1) << 16)+ ((i/TEMP_NUM+1) << 8) + UNDER_TEMP_3;
                    value = i%TEMP_NUM + 1;
                    break;
                }
            }
        }
//        qDebug() << __func__ << "loop break with:" << m->temp[i]
//                 << "key:" << QString::number(key, 16);

        if(key)mw->warning.insert(key,value);

        //cell over temp
//        qDebug() << "***";
//        qDebug() << __func__ << "*cell over temp*"
//                 << "L1:" << p->charge_over_temp.toFloat()*10
//                 << "L2" << p->over_temp_2.toFloat()*10
//                 << "L3" << p->over_temp_3.toFloat()*10;
        key = 0;
        if (0 == mw->getModuleFaultValueByCode(id + 1, CHARGE_OVER_TEMP)
                || 0 == mw->getModuleFaultValueByCode(id + 1, DISCHARGE_OVER_TEMP)) {
            for (i = 0; i < PACK_NUM*TEMP_NUM; ++i) {
                if(m->temp[i] > p->over_temp_2.toFloat()*10){
                    key = ((id+1) << 16) + ((i/TEMP_NUM+1) << 8) + OVER_TEMP_2;
                    value = i%TEMP_NUM + 1;
                    break;
                }

                if(m->temp[i] > p->over_temp_3.toFloat()*10){
                    key = ((id+1) << 16)+ ((i/TEMP_NUM+1) << 8) + OVER_TEMP_3;
                    value = i%TEMP_NUM + 1;
                    break;
                }
            }
        }
//        qDebug() << __func__ << "loop break with:" << m->temp[i]
//                 << "key:" << QString::number(key, 16);

        if(key)mw->warning.insert(key,value);
    }
}*/

void BgWorker::modbusTcpServer()
{
    uint deviceID = 0;
    int server_socket,master_socket,rc,fdmax;
    int header_length;
    fd_set refset,rdset;

    uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
    modbus_t *ctx = mw->modbus_tcp;
    header_length = modbus_get_header_length(ctx);

    //modbus_set_debug(ctx,true);
    server_socket = modbus_tcp_listen(ctx, 5);
    if(server_socket == -1)
    {
        qWarning("Modbus_tcp server init failed!");
        return;
    }
    /* Clear the reference set of socket */
    FD_ZERO(&refset);
    /* Add the server socket */
    FD_SET(server_socket, &refset);
    fdmax = server_socket;

    while(!isStop)
    {
        rdset = refset;
        if (select(fdmax+1, &rdset, NULL, NULL, NULL) == -1) {
            qWarning("Server select failed!");
            break;
        }

        for (master_socket = 0; master_socket <= fdmax; master_socket++)
        {
            if (!FD_ISSET(master_socket, &rdset)) continue;

            if (master_socket == server_socket)
            {
                /* A client is asking a new connection */
                socklen_t addrlen;
                struct sockaddr_in clientaddr;

                /* Handle new connections */
                addrlen = sizeof(clientaddr);
                memset(&clientaddr, 0, sizeof(clientaddr));
                int newfd = accept(server_socket, (struct sockaddr *)&clientaddr, &addrlen);
                if (newfd == -1)
                {
                    qWarning("Server accept error!");
                }
                else
                {
                    FD_SET(newfd, &refset);
                    if (newfd > fdmax)fdmax = newfd;
                }
            }
            else
            {
                modbus_set_socket(ctx, master_socket);
                rc = modbus_receive(ctx, query);
                if (rc > 0)
                {
                    deviceID = query[header_length-1];
                    if(deviceID > 0 && deviceID < (MODULE_NUM+2))
                    {
                        modbus_reply(ctx, query, rc, mw->mb_mapping[deviceID-1]);
                    }
                    else
                    {
                        qWarning("Modbus_tcp deviceID error!");
                    }
                }
                else if (rc == -1)
                {
                    /* connection closing or any errors. */
                    close(master_socket);
                    /* Remove from reference set */
                    FD_CLR(master_socket, &refset);

                    if (master_socket == fdmax) fdmax--;
                 }
             }
         }
    }

    close(server_socket);
}

void BgWorker::doQueryPeripheral()
{
    while (!isStop) {
        const int n = 5;
        int slave = 1;

//        modbus_connect(mw->hmi->modbus4IO);
        for (int i = 0; i < TEMP_HUMIDITY_METER_NUM; ++i, ++slave) {
            mw->hmi->getTempHumidity(slave, i);
        }

        for (int i = 0; i < DC_INSULATION_MONITORING_DEVICE_NUM; ++i, ++slave) {
            mw->hmi->getDcInsulation(slave, i);
        }

        mw->hmi->getIO(true, 0, DIGITAL_OUTPUT_BLOCK * n, slave);
        mw->hmi->getIO(false, 0, DIGITAL_INPUT_BLOCK * n, slave);
        slave += n;

        for (int i = 0; i < AIR_CONDITIONER_NUM; ++i, ++slave) {
            mw->hmi->getAirConditionerInfo(slave, i);
        }

//        for (int i = 0; i < ELECTRICITY_METER_NUM; ++i, ++slave) {
//            mw->hmi->getElectricityMeter(slave, i);
//        }
//        modbus_close(mw->hmi->modbus4IO);

//        // read phase A voltage with baudrate 2400
//        (sp_get_port_by_name("/dev/ttyAMA0", &port));

//        printf("Opening port.\n");
//        (sp_open(port, SP_MODE_READ_WRITE));

//        printf("Setting port to 2400 8N1, no flow control.\n");
//        (sp_set_baudrate(port, 2400));
//        (sp_set_bits(port, 8));
//        (sp_set_parity(port, SP_PARITY_NONE));
//        (sp_set_stopbits(port, 1));
//        (sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE));

//        uint8_t read_buf[4];
//        memset(read_buf, 0, 4);
//        uint8_t test_addr[6] = {0x81,0x00,0x00,0x64,0x09,0x70};

//        //设置从机地址
//        dlt645_set_addr(&dlt645,test_addr);

//        //if(dlt645_read_data(&dlt645,DLT645_1997_READ_TEST_CODE,read_buf,DLT645_1997) > 0) //1997采集测试
//        if(dlt645_read_data(&dlt645,0x02010100,read_buf,DLT645_2007) > 0)  //2007采集测试
//        {
//            printf("读取成功,A相电压值为: %.2f \r\n",*(float *)read_buf);
//        }
//        else
//        {
//            printf("读取失败\r\n");
//        }

//        sp_close(port);
    }
}

void BgWorker::runIEC104Slave()
{
    ycinterval = 1000;
    setaddr(0x01);
    dorun();
}

void BgWorker::writeCan(uint8_t *data,int len,int dest)
{
    can_frame frame;
    memset(frame.data,0,sizeof(frame.data));

    frame.can_dlc = len;
    if(len)memcpy(frame.data,data,len);

    frame.can_id = dest;

    mw->hmi->WriteCan(&frame);

}

void BgWorker::stopThread()
{
    isStop = true;

    if(file.isOpen())
    {
        file.flush();
        file.close();
    }

    thread.terminate();
}

int BgWorker::calcBatteryExtremum(int n)
{
    quint16 maxVoltage = 0;
    quint16 minVoltage = 0;
    qint16 maxTemp = 0;
    qint16 minTemp = 0;
    quint16 tmpVoltage[BATTERY_NUM_PER_MODULE] = {0};
    qint16 tmpTemp[TEMP_NUM_PER_MODULE] = {0};
    QList<quint16> listBatteryVoltage;
    QList<qint16> listBatteryTemp;

    memcpy(tmpVoltage, mw->module[n].voltage, sizeof(tmpVoltage));
    memcpy(tmpTemp, mw->module[n].temp, sizeof(tmpTemp));

    listBatteryVoltage.clear();
    for (int i = 0; i < BATTERY_NUM_PER_MODULE; ++i) {
        if (0 != tmpVoltage[i]) {
            if (tmpVoltage[i] > maxVoltage) {
                if (0 == minVoltage) {
                    minVoltage = tmpVoltage[i];
                }

                maxVoltage = tmpVoltage[i];
            }

            if (tmpVoltage[i] < minVoltage) {
                if (0 == maxVoltage) {
                    maxVoltage = tmpVoltage[i];
                }

                minVoltage = tmpVoltage[i];
            }
        }

        listBatteryVoltage.append(tmpVoltage[i]);
    }

    listBatteryTemp.clear();
    for (int i = 0; i < TEMP_NUM_PER_MODULE; ++i) {
        if (0 != tmpTemp[i]) {
            if (tmpTemp[i] > maxTemp) {
                if (0 == minTemp) {
                    minTemp = tmpTemp[i];
                }

                maxTemp = tmpTemp[i];
            }

            if (tmpTemp[i] < minTemp) {
                if (0 == maxTemp) {
                    maxTemp = tmpTemp[i];
                }

                minTemp = tmpTemp[i];
            }
        }

        listBatteryTemp.append(tmpTemp[i]);
    }

    if (0 == maxVoltage || 0 == minVoltage || 0 == maxTemp || 0 == minTemp) {
        return false;
    }

    mw->module[n].maxV = maxVoltage;
    mw->module[n].minV = minVoltage;
    mw->module[n].maxT = maxTemp;
    mw->module[n].minT = minTemp;
    mw->module[n].maxVID = listBatteryVoltage.indexOf(maxVoltage);
    mw->module[n].minVID = listBatteryVoltage.indexOf(minVoltage);
    mw->module[n].maxTID = listBatteryTemp.indexOf(maxTemp);
    mw->module[n].minTID = listBatteryTemp.indexOf(minTemp);

    return true;
}

/** 2 special situation should be handled carefully
  * one is fist time to initialize pile Soc
  * the other is disconnected
  */
void BgWorker::calcPileSoc(MODBUS::PCS & pile, const QList<quint16> & socList)
{
    bool isInitialized = true;
    quint16 *p = pile.pileSoc;
    quint16 *f = pile.antialiasingSoc;

    if (0xFFFF == p[PREV] && 0xFFFF == p[NEXT] && 0xFFFF == f[PREV] && 0xFFFF == f[NEXT]) {
        isInitialized = false;
    }

    if (!isInitialized && !socList.isEmpty()) {
        p[PREV] = p[NEXT] = f[PREV] = f[NEXT] = pile.isCharging ? socList.last() : socList.first();
    }

    if (isInitialized && socList.isEmpty()) {           // no heartbeat
        p[PREV] = p[NEXT] = f[PREV] = f[NEXT] = 0xFFFF;
//        p[PREV] = p[NEXT];
//        p[NEXT] = 0;
    }

    if (isInitialized && !socList.isEmpty()) {
        p[PREV] = p[NEXT];
        p[NEXT] = pile.isCharging ? socList.last() : socList.first();
    }

    if (isInitialized) {
        handlePileSoc(pile);
    }

    return;
}

void BgWorker::handlePileSoc(MODBUS::PCS & pile)
{
    const quint16 thredshold = 10;

    quint16 *p = pile.pileSoc;
    quint16 *f = pile.antialiasingSoc;

    f[0] = f[1];
    if (thredshold > qAbs(p[0] - p[1]) && thredshold > qAbs(p[0] - f[0])) {
        f[1] = p[1];
        return;
    }

    if (thredshold <= qAbs(p[0] - p[1])) {
        f[1] = f[0];
        return;
    }

    if (p[0] > p[1]) {
        f[1] = qRound(f[0] - (p[0] - p[1]) * f[0] / (p[0] - 0.0f));
        return;
    }

    if (0 != (1000 - p[0])) {
        f[1] = qRound(f[0] + (p[1] - p[0]) * (1000 - f[0]) / (1000.0f - p[0]));
    } else {
        f[1] = f[0];
    }

    return;
}

QString BgWorker::removeOldestDataDir()
{
    QString subDirName = "";
    QDir dir(APP_DATA_DIRPATH);

    foreach (QFileInfo fileInfo, dir.entryInfoList(QStringList("*-*"), QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
        subDirName = fileInfo.fileName();
//        qDebug() << "read2rm" << subDirName << dir.rmdir(subDirName);
        break;
    }

    if (dir.cd(subDirName)) {
        foreach (QFileInfo fileInfo, dir.entryInfoList(QStringList("*-*-*.csv"), QDir::Files)) {
            qDebug() << __func__ << "*** ready2rm:" << fileInfo.fileName()
                     <<fileInfo.dir().remove(fileInfo.fileName());
        }
        if (dir.cdUp()) {
            qDebug() << __func__ << "*** ready2rm" << subDirName << dir.rmdir(subDirName);
        } else {
            qDebug() << __func__ << "dir.cdUp() failed. ***";
        }
    }

    return subDirName;
}

QString BgWorker::removeOldestDataFile()
{
    QString fileName = "";
    QDir dir(APP_DATA_DIRPATH);

    foreach (QFileInfo fileInfo, dir.entryInfoList(QStringList("*-*"), QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
        if (!dir.cd(fileInfo.fileName())) {
            continue;
        }

        qDebug() << __func__ << dir.canonicalPath() << dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot).count()
                 << dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
        fileName = fileInfo.fileName();
        if (0 < dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot).count()) {
            break;
        }

        qDebug() << __func__ << fileName << "dir is empty";
        if (!dir.cdUp()) {
            fileName = "";
            qDebug() << __func__ << fileName << "cdUp failed";
            break;
        }

        qDebug() << __func__ << fileName << "cdUp ok";
        if (dir.rmdir(fileName)) {
            qDebug() << __func__ << fileName;
        }
    }

    foreach (QFileInfo fileInfo, dir.entryInfoList(QStringList("*-*-*.csv"), QDir::Files)) {
        fileName = fileInfo.fileName();

        if (fileInfo.dir().remove(fileName)) {
            qDebug() << __func__ << fileName;
            break;
        }
    }

    return fileName;
}

#if 0
QString BgWorker::clearDiskIfNeed()
{
    quint64 blockSize = 0;
    quint64 freeSpace = 0;
    struct statfs fs;
    QString dirName = "";

    if (0 == statfs(APP_DATA_DIRPATH, &fs)) {
        blockSize = fs.f_bsize;
        freeSpace = fs.f_bfree * blockSize;

        qDebug() << __func__ << "free space:" << (freeSpace >> 20) << "MB";
        if (freeSpace < DISK_CLEAR_THRESHOLD) {
            dirName = removeOldestDataDir();
            qDebug() << __func__ << "dir name:" << dirName;
        }
    }

    return dirName;
}
#else
void BgWorker::clearDiskIfNeed()
{
    quint64 blockSize = 0;
    quint64 freeSpace = 0;
    struct statfs fs;

    if (0 == statfs(APP_DATA_DIRPATH, &fs)) {
        /** f_bfree: free blocks in fs
          * f_bavail: free blocks available to non-superuser
          * f_avail is less 5% than f_bfree
          * df use f_avail
          */
        blockSize = fs.f_bsize;
        freeSpace = fs.f_bavail * blockSize;

        if (freeSpace < DISK_CLEAR_THRESHOLD
                && !removeOldestDataFile().isEmpty()) {
            clearDiskIfNeed();
        }
    }

    return;
}
#endif

bool BgWorker::checkCount(bool isCount, unsigned &curCount, unsigned maxCount)
{
    if (!isCount) {
        curCount = 0;
    } else if (curCount < maxCount) {
        ++curCount;
    }

    return curCount == maxCount;
}

quint16 BgWorker::crc16(const void *s, int n)
{
    unsigned short c = 0xffff;
    for (int k=0;k<n;k++) {
        unsigned short b=((unsigned char *)s)[k];
        for (int i=0;i<8;i++){
            c=((b^c)&1)?(c>>1)^0xA001:(c>>1);
            b>>=1;
        }
    }
    return (c<<8)|(c>>8);
}
