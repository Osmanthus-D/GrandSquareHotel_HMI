#include "hmi.h"

HMI::HMI() : modbus4IO(NULL)
{
#ifdef WEIQIAN
    struct timeval t;
    Config config_others(CONFIG_PATH_OTHERS);
    QString device;

    config_others.load_others();
    device = port2Dev(config_others.periphComPort);
    data4Di = (quint8 *)qMalloc(MAX_INPUT_NUM * sizeof(quint8));
    data4Do = (quint8 *)qMalloc(MAX_OUTPUT_NUM * sizeof(quint8));
    modbus4IO = modbus_new_rtu(qPrintable(device), 9600, 'N', 8, 1);

    if (NULL == modbus4IO) {
        qWarning("modbus4IO init failed.");
    } else {
        t.tv_sec = 0;
        t.tv_usec = 1000000;
        modbus_set_response_timeout(modbus4IO, &t);

        if (0 < modbus_connect(modbus4IO)) {
            qWarning() << "modbus4IO connect failed:" << modbus_strerror(errno);
        }
    }
#endif

    canOpened = false;
    isDhcp = false;

    memset(ip, 0x00, sizeof(ip));
    memset(mask, 0x00, sizeof(mask));
    memset(gateway, 0x00, sizeof(gateway));
    memset(dns, 0x00, sizeof(dns));
    memset(macAddr,0x00, sizeof(macAddr));
    memset(&peripheral, 0x00, sizeof(peripheral));

    peripheral.dcInsulationMonitoringDevice[0].total_bus_resistance = 0xFFFF;
    peripheral.dcInsulationMonitoringDevice[0].positive_bus_resistance_to_ground = 0xFFFF;
    peripheral.dcInsulationMonitoringDevice[0].negative_bus_resistance_to_ground = 0xFFFF;
}

void HMI::Beep()
{
#if defined WEIQIAN
    WeiqianFunctions::Beep();
#endif
}

void HMI::OpenCan(int baudRate)
{
    Q_UNUSED(baudRate)
#if defined ARM_LINUX
    qDebug() << __func__ << WeiqianFunctions::OpenCan(baudRate * 1000);
    this->canOpened = true;
#endif
}

int HMI::ReadCan(can_frame *frame)
{
    QMutexLocker locker(&mutex4CanBus);

#if defined ARM_LINUX
    if (canOpened) {
        return WeiqianFunctions::CanRead(&frame->can_id,(int *) &frame->can_dlc, (char *)frame->data);
    } else {
        return -255;
    }
#else
    Q_UNUSED(frame)
    return -255;
#endif
}

int HMI::WriteCan(can_frame *frame)
{
    QMutexLocker locker(&mutex4CanBus);

#if defined ARM_LINUX
    if (canOpened) {
        int ret;

#ifdef WEIQIAN
        frame->can_id &= ~CAN_EFF_FLAG;     // erase CAN_EFF_FLAG
#endif

        ret = WeiqianFunctions::CanWrite(frame->can_id, (const char *)frame->data, frame->can_dlc);
        return ret;
    } else {
        return -255;
    }
#else
    Q_UNUSED(frame)
    return -255;
#endif
}

void HMI::CloseCan()
{
#if defined ARM_LINUX
    WeiqianFunctions::CloseCan();
    this->canOpened = false;
#endif
}

void HMI::RestartCan(int baudRate)
{
    QMutexLocker locker(&mutex4CanBus);

#if defined ARM_LINUX
    if (canOpened) {
        WeiqianFunctions::CloseCan();
        usleep(5 * 1000);
        qDebug() << __func__ << WeiqianFunctions::OpenCan(baudRate * 1000);
    }
#else
    Q_UNUSED(baudRate)
#endif
}


bool HMI::SetWDog(int interval)
{
#if defined ARM_LINUX
    return WeiqianFunctions::SetWatchDog(interval);
#else
    return false;
#endif
}

bool HMI::StartWDog()
{
#if defined ARM_LINUX
    return WeiqianFunctions::StarWatchDog();
#else
    return false;
#endif
}

bool HMI::FeedWDog()
{
#if defined ARM_LINUX
    return WeiqianFunctions::FeedWatchDog();
#else
    return false;
#endif
}

bool HMI::StopWDog()
{
#if defined ARM_LINUX
    WeiqianFunctions::StopWatchDog();
    return true;
#else
    return false;
#endif
}

bool HMI::getNetWork(const char * iFace, bool *isDhcp,char *ip,char *subnetmask,char *gateway,char *dns,char *macAddr)
{
#if defined ARM_LINUX
    return WeiqianFunctions::GetNetWorkCfg(iFace, isDhcp, ip, subnetmask, gateway, dns, macAddr);
#else
    return false;
#endif
}

bool HMI::setNetWork(const char * iFace, bool isDhcp,char *ip,char *subnetmask,char *gateway,char *dns)
{
#if defined ARM_LINUX
    return WeiqianFunctions::SetNetWorkCfg(iFace, isDhcp, ip, subnetmask, gateway, dns);
#else
    return false;
#endif
}

void HMI::getBacklightState(int *level,bool *autoClose,int *timeout)
{
    Q_UNUSED(level)
    Q_UNUSED(autoClose)
    Q_UNUSED(timeout)
//#if defined ARM_LINUX
//    *level = api->GetDefaultBackLightLevel();
//    *autoClose = api->GetEnableBackLightAuto();
//    *timeout = api->GetBacklightTimeOut();
//#endif
}

void HMI::setBacklightLevel(int level)
{
#if defined(WEIQIAN)
    int brightness = -1;

    switch (level) {
    case 0:
        brightness = 0;
        break;
    case 1:
        brightness = 9;
        break;
    case 2:
        brightness = 25;
        break;
    case 3:
        brightness = 64;
        break;
    case 4:
        brightness = 100;
        break;
    default:
        break;
    }

    if(0 <= brightness && 100 >= brightness)
    {
        WeiqianFunctions::SetBacklightBrightness(brightness);
    }
#elif defined(YCTEK)
    WeiqianFunctions::SetBackLightLevel(level);
#else
    return;
#endif
}

int HMI::getBacklightLevel()
{
#if defined(WEIQIAN)
    int level = -1;
    int brightness = WeiqianFunctions::GetBacklightBrightness();
    qreal squareRoot = 0;

    if(0 > brightness)
    {
        return -1;
    }

    squareRoot = qSqrt(brightness);

    if(squareRoot <= 3)
    {
        level = 0;
    }
    else if (squareRoot <= 5) {
        level = 1;
    }
    else if (squareRoot <= 8) {
        level = 2;
    }
    else if (squareRoot <= 10) {
        level = 3;
    }

    return level;
#elif defined(YCTEK)
#else
    return -1;
#endif
}

void HMI::powerOffBackLight()
{
    qDebug() << "power off backlight.";
    WeiqianFunctions::SetBackLight(0);
}

void HMI::setBacklightAutoClose(bool autoClose,int timeout)
{
    Q_UNUSED(autoClose)
    Q_UNUSED(timeout)
//#if defined ARM_LINUX
//    api->EnableBackLightAuto(autoClose);
//    api->SetBacklightTimeOut(timeout);
//#endif
}

int HMI::setIO(quint8 addr, bool open)       // num start from 0
{
#ifdef WEIQIAN
    QtConcurrent::run(this, &HMI::doWriteDigitalOutput, addr, open);
    return 0;
#endif
}

int HMI::setIO(quint8 index, quint8 addr, quint8 nb, bool open)
{
#ifdef WEIQIAN
    QtConcurrent::run(this, &HMI::doWriteDigitalOutput, index, addr, nb, open);
    return 0;
#endif
}

int HMI::getIO(bool input, quint8 addr, quint8 len, quint8 slave)
{
#ifdef WEIQIAN
    QFutureWatcher<int> futureWatcher;
    QFuture<int> future = QtConcurrent::run(this, &HMI::doReadDigitalInputOutput, input, addr, len, slave);
    futureWatcher.setFuture(future);
    future.waitForFinished();
    return futureWatcher.result();
#endif
}

int HMI::getTempHumidity(int slave, int offset)
{
#ifdef WEIQIAN
    QFutureWatcher<int> futureWatcher;
    QFuture<int> future =QtConcurrent::run(this, &HMI::doReadTempHumidity, slave, offset);
    futureWatcher.setFuture(future);
    future.waitForFinished();
    return futureWatcher.result();
#endif
}

int HMI::getDcInsulation(int slave, int offset)
{
#ifdef WEIQIAN
    QFutureWatcher<int> futureWatcher;
    QFuture<int> future =QtConcurrent::run(this, &HMI::doReadDcInsulation, slave, offset);
    futureWatcher.setFuture(future);
    future.waitForFinished();
    return futureWatcher.result();
#endif
}

int HMI::getAirConditionerInfo(int slave, int offset)
{
#ifdef WEIQIAN
    QFutureWatcher<int> futureWatcher;
    QFuture<int> future =QtConcurrent::run(this, &HMI::doReadAirConditioner, slave, offset);
    futureWatcher.setFuture(future);
    future.waitForFinished();
    return futureWatcher.result();
#endif
}

int HMI::getElectricityMeter(int slave, int offset)
{
#ifdef WEIQIAN
    QFutureWatcher<int> futureWatcher;
    QFuture<int> future =QtConcurrent::run(this, &HMI::doReadElectricityMeter, slave, offset);
    futureWatcher.setFuture(future);
    future.waitForFinished();
    return futureWatcher.result();
#endif
}

#ifdef WEIQIAN
int HMI::getDataDI(quint8 *buf, int len)
{
    if (len > MAX_INPUT_NUM) {
        return -1;
    }

    qMemCopy(buf, data4Di, len);
    return 0;
}

int HMI::getDataDO(quint8 *buf, int len)
{
    if (len > MAX_OUTPUT_NUM) {
        return -1;
    }

    qMemCopy(buf, data4Do, len);
    return 0;
}

int HMI::doWriteDigitalOutput(quint8 addr, bool open)
{
    QMutexLocker locker(&mutex4Modbus);

    int ret = -1;
    if (NULL != modbus4IO) {
        if (MAX_OUTPUT_NUM > addr) {
            if (0 == modbus_set_slave(modbus4IO, addr / DIGITAL_OUTPUT_BLOCK + 1 + 6)) {
                ret = modbus_write_bit(modbus4IO, addr % DIGITAL_OUTPUT_BLOCK, open ? 1 : 0);
            } else {
                qWarning() << __func__ << "modbus_set_slave failed:" << modbus_strerror(errno);
            }
        } else {
            qWarning() << __func__ << "addr larger than maximum.";
        }
    }

    return ret;
}

int HMI::doWriteDigitalOutput(quint8 index, quint8 addr, quint8 nb, bool open)
{
    QMutexLocker locker(&mutex4Modbus);

    quint8 buf[32] = {0x00};

    if (NULL == modbus4IO) {
        return -1;
    }

    if (0 != modbus_set_slave(modbus4IO, index)) {
        qWarning() << __func__ << "modbus_set_slave failed: " << modbus_strerror(errno);
    }

    modbus_set_bits_from_byte(buf, 0, 0x3F);
    return modbus_write_bits(modbus4IO, 0, 6, buf);
}

int HMI::doReadDigitalInputOutput(bool input, quint8 addr, quint8 len, quint8 slave)
{
    QMutexLocker locker(&mutex4Modbus);

    int ret = 0;
    quint8 read = 0;
    quint8 nb = 0;
    quint8 block = 0;
    quint8 dataLen = 0;
    quint8 *data = NULL;
    QBitArray bitArray;

    if (NULL == modbus4IO) {
        return -1;
    }

    if (MAX_INPUT_NUM >= addr + len) {
        block = input ? DIGITAL_INPUT_BLOCK : DIGITAL_OUTPUT_BLOCK;
        dataLen = input ? MAX_INPUT_NUM : MAX_OUTPUT_NUM;
        data = (quint8 *)qMalloc(dataLen * sizeof(quint8));

        while (read < len) {
            if (0 > modbus_set_slave(modbus4IO, (addr + read) / block + slave)) {
                qDebugPcs() << __func__ << "modbus_set_slave failed: " << modbus_strerror(errno);
                ret = -1;
                break;
            }

            if ((addr + read) / block < (addr + len) / block) {
                nb = block - ((addr + read) % block);
            } else {
                nb = len - read;
            }

            if (input) {
                if (0 < modbus_read_input_bits(modbus4IO, (addr + read) % block, nb, data + read)) {
                    read += nb;
                } else {
                    qDebugPcs() << __func__ << "modbus_read_input_bits failed:" << modbus_strerror(errno);
                    ret = -1;
                    break;
                }
            } else {
                if (0 < modbus_read_bits(modbus4IO, (addr + read) % block, nb, data + read)) {
                    read += nb;
                } else {
                    qDebugPcs() << __func__ << "modbus_read_bits failed:" << modbus_strerror(errno);
                    ret = -1;
                    break;
                }
            }
            usleep(50 * 1000);
        }

        bitArray.resize(read);
        for (int i = 0; i < read; ++i) {
            bitArray.setBit(i, 0 != data[addr + i]);
        }

        if (input) {
//            if (read == len) {
//                qMemCopy(data4Di, data, dataLen);
//            }
            qMemCopy(data4Di + addr, data + addr, read);
            emit readyReadDigiInput(addr, len, bitArray);
        } else {
//            if (read == len) {
//                qMemCopy(data4Do, data, dataLen);
//            }
            qMemCopy(data4Do + addr, data + addr, read);
            emit readyReadDigiOutput(addr, len, bitArray);
        }

        qFree(data);
        return ret;
    } else {
        qWarning() << __func__ << "request too may digital inputs or outputs.";
        return -1;
    }
}

int HMI::doReadTempHumidity(int slave, int offset)
{
    QMutexLocker locker(&mutex4Modbus);

    const int type = 1;
    int ret = 0;
    if (0 > modbus_set_slave(modbus4IO, slave)) {
        qDebugPcs() << __func__ << "modbus_set_slave failed: " << modbus_strerror(errno);
        return -1;
    }

    if (0 > modbus_read_registers(modbus4IO, 0, 2, (quint16 *)&peripheral.tempHumidityMeter[offset])) {
        qDebugPcs() << __func__ << "modbus_read_registers failed: " << modbus_strerror(errno);
        ret = -1;
    }

    emit readyReadPeripheral(type, offset, ret);
    usleep(50 * 1000);
    return ret;
}

int HMI::doReadDcInsulation(int slave, int offset)
{
    QMutexLocker locker(&mutex4Modbus);

    const int type = 2;
    int ret = 0;
    if (0 > modbus_set_slave(modbus4IO, slave)) {
        qDebugPcs() << __func__ << "modbus_set_slave failed: " << modbus_strerror(errno);
        return -1;
    }

    if (0 > modbus_read_registers(modbus4IO, 0x10, 4, (quint16 *)&peripheral.dcInsulationMonitoringDevice[offset])) {
        qDebugPcs() << __func__ << "modbus_read_registers failed: " << modbus_strerror(errno);
        ret = -1;
    }

    emit readyReadPeripheral(type, offset, ret);
    usleep(50 * 1000);
    return ret;
}

int HMI::doReadAirConditioner(int slave, int offset)
{
    QMutexLocker locker(&mutex4Modbus);

    const int type = 3;
    int ret = 0;
    if (0 > modbus_set_slave(modbus4IO, slave)) {
        qDebugPer() << __func__ << "modbus_set_slave failed: " << modbus_strerror(errno);
        return -1;
    }

    if (0 > modbus_read_registers(modbus4IO, 0x1000, 9, (quint16 *)&peripheral.airConditioner[offset])) {
        qDebugPer() << __func__ << "modbus_read_registers failed: " << modbus_strerror(errno);
        ret = -1;
    }

    emit readyReadPeripheral(type, offset, ret);
    usleep(50 * 1000);
    return ret;
}

int HMI::doReadElectricityMeter(int slave, int offset)
{
    QMutexLocker locker(&mutex4Modbus);

    const int type = 4;
    int ret = 0;
    if (0 > modbus_set_slave(modbus4IO, slave)) {
        qDebugPer() << __func__ << "modbus_set_slave failed: " << modbus_strerror(errno);
        return -1;
    }

    if (0 > modbus_read_registers(modbus4IO, 0x68, 1, (quint16 *)&peripheral.electricityMeter[offset])) {
        qDebugPer() << __func__ << "modbus_read_registers failed: " << modbus_strerror(errno);
        ret = -1;
    }

    emit readyReadPeripheral(type, offset, ret);
    usleep(50 * 1000);
    return ret;
}
#endif

HMI::~HMI()
{
#ifdef WEIQIAN
    if (NULL != modbus4IO) {
        modbus_close(modbus4IO);
        modbus_free(modbus4IO);
    }

    qFree(data4Di);
    qFree(data4Do);
#endif
}
