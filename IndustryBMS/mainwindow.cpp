#include "mainwindow.h"
#include "ui_mainwindow.h"

using namespace QsLogging;

QFile g_log(QString(LOG_PATH) + QString("/log"));
QFile g_logBak(QString(LOG_PATH) + QString("/log.bak"));
QFile g_log2(QString(LOG_PATH) + QString("/log2"));
QFile g_log2Bak(QString(LOG_PATH) + QString("/log2.bak"));

MainWindow::MainWindow(QWidget *parent, int n) :
    QMainWindow(parent),
    Base(n),
    hmi(new HMI),
    ui(new Ui::MainWindow)
{
//    qWarning() << Q_FUNC_INFO;
    ui->setupUi(this);

    mcu_ack = false;
    mcu_updating = false;
    isFireAlarm = false;
    isEmergency = false;

    logType = 0;
    transparency = 0;
    tableRow = 0;
    sendTime = 0;

    m_page = 1;
    p_page = 1;
    para_page = 1;

    extDiskUsingFlag = false;

#ifdef FONT_POINT_SIZE_TEST
    font_point_size_inc = 0;
#endif

    modbus = NULL;
    modbus_tcp = NULL;
    mb_mapping = NULL;
    faultArray = NULL;
    module = NULL;
    fictitiousSoc = NULL;
    quantity = NULL;
    para = NULL;
    fs = NULL;

    for (unsigned i = 0; i < PCS_NUM; ++i) {
        dcSideSwitch[i] = false;
    }

//    WeiqianFunctions::SetBackLight(1);
    portMgrDialog = new PortMgrDialog(this);
    factoryDialog = new FactoryDialog(this);
    systemTypeDialog = new SystemTypeDialog(this);
    fs = new CustomLabel;
    fs->hide();

//    foreach(QObject *obj, ui->tableView_v->children())
//    {
//        if(obj->objectName() == "qt_scrollarea_viewport")
//        {
//            obj->installEventFilter(this);
//        }
//    }

    date = new QLabel(this);
    date->setMinimumSize(250,20);
    date->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
    date->setFrameShape(QFrame::NoFrame);

    genGroupBoxAck(ui->para_ACK, MODULE_NUM);
    genTopoGraph(ui->topologyGraph, MODULE_NUM);
    moduleChange();
    paramClusterChanged();
    connectSignal();

    initOprtParamPage = ui->stackedWidgetOperationPara->count();
    initWarnParamPage = ui->stackedWidgetWarningPara->count();
    initClbrParamPage = ui->stackedWidgetCalibrationPara->count();

    // load param from xml file
    paramLoader = new ParamLoader("/home/asd/myparamset.xml", this);
    paramLoader->load(ui->stackedWidgetOperationPara, ParamLoader::Operating);
    paramLoader->load(ui->stackedWidgetWarningPara, ParamLoader::Warning);
    paramLoader->load(ui->stackedWidgetCalibrationPara, ParamLoader::Calibration);

    // load warnings from xml file
    paramLoader->load();

#if (defined(WEIQIAN) || defined(WZD)) && !defined(FONT_POINT_SIZE_TEST)
    calibrateFontSize(-4);
#endif

    qDebug() << "font family:" << this->font().family() << "point size:" << this->font().pointSize() << "pixel size:" << this->font().pixelSize();
    setWindowFlags(Qt::FramelessWindowHint);
    setWindowModality(Qt::WindowModal);
    setAttribute(Qt::WA_DeleteOnClose);

    //init
    init();
    initModbus();
    initTablePeripheral();
    createThread();

    //start timer
    timerID_0 = startTimer(100);
    timerID_1 = startTimer(1000);
    timerID_1_8 = startTimer(1800);
    timerID_2 = startTimer(2000);
    timerID_5 = startTimer(5000);

    timerID_10 = startTimer(10000);
    timerID_15 = startTimer(15000);
    timerID_60 = startTimer(60*1000);
    timerID_X = startTimer(config_others->dataSaveInterval.toInt()*1000);
    timerID_XX = startTimer(30*1000);

    //watchDog
    hmi->SetWDog(5);
    hmi->StartWDog();
    hmi->FeedWDog();
    hmi->StopWDog();
    setAttribute(Qt::WA_AcceptTouchEvents, true);
}

#if (defined(WEIQIAN) || defined(WZD)) && !defined(FONT_POINT_SIZE_TEST)
void MainWindow::calibrateFontSize(int increment)
{
    QWidget *w = NULL;
    QGroupBox *b = NULL;
    QList<QObject *> objList = this->findChildren<QObject *>();

    foreach (QObject *obj, objList) {
        if(obj->inherits("QWidget"))
        {
            w = qobject_cast<QWidget *>(obj);

            /* number 4 is experimental value, define FONT_POINT_SIZE_TEST then use
               reset button and reboot button to tweak font size, mainwindow.ui has no
               layout, so we need to change font size one by one. */

            QFont font("BENMO Jingyuan Regular", w->font().pointSize() + increment);

            w->setFont(font);

            if(0 == qstrcmp(obj->metaObject()->className(), "QGroupBox"))
            {
                b = qobject_cast<QGroupBox *>(obj);

                QList<QObject *> childrenOfBox = b->findChildren<QObject *>();

                foreach (QObject *child, childrenOfBox) {
                    if(child->inherits("QWidget"))
                    {
                        w = qobject_cast<QWidget *>(child);

                        QFont font("BENMO Jingyuan Regular", w->font().pointSize());
                        w->setFont(font);
                    }
                }
            }
        }
    }

    update();
}
#endif

void MainWindow::init()
{
    QStringList moduleNumList;

    ui->tabWidgetBody->setCurrentIndex(0);
    ui->stackedWidgetBodyInfo->setCurrentIndex(0);
    ui->stackedWidgetBodyPara->setCurrentIndex(0);
    ui->stackedWidgetBodySystem->setCurrentIndex(0);
    ui->packViewer->setCurrentIndex(0);
    ui->stackedWidgetOperationPara->setCurrentIndex(0);
    ui->stackedWidgetWarningPara->setCurrentIndex(0);

#ifdef XINLONG
    pcs = new MODBUS::PCS[MODULE_NUM];
#endif

    mb_mapping = new modbus_mapping_t*[MODULE_NUM + 1]();
    faultArray = new quint16[MODULE_NUM]();
    module = new Module[MODULE_NUM]();
    fictitiousSoc = new FictitiousSoc[MODULE_NUM]();
    quantity = new Quantity[MODULE_NUM]();
    para = new Para[MODULE_NUM]();
    operatingParam = new quint16[MODULE_NUM][MAX_PARAM_NUM_PER_MODULE];
    warningParam = new quint16[MODULE_NUM][MAX_PARAM_NUM_PER_MODULE];
    calibParam = new quint32[MODULE_NUM][MAX_PARAM_NUM_PER_MODULE];

    qMemSet(&heap, 0x00, sizeof(heap));
    qMemSet(module, 0x00, sizeof(*module) * MODULE_NUM);
    qMemSet(fictitiousSoc, 0x00, sizeof(*fictitiousSoc) * MODULE_NUM);

    for (unsigned i = 0; i < MODULE_NUM; ++i) {
        QTime currentTime = QTime::currentTime();
        qsrand(currentTime.msec() + currentTime.second() * 1000 + i);
        fictitiousSoc[i].drift = qrand() % 10;
    }

    // init log machenism
    Logger& logger = Logger::instance();
    logger.setLoggingLevel(QsLogging::TraceLevel);

    const QString sLogPath(QDir(LOG_PATH).filePath("alarm.log"));
    hisRecordModel = new HisRecordModel(this, this);
    hisRecordModel->initRecordModelFromFile(sLogPath);
    ui->tableHis->setModel(hisRecordModel);
    DestinationPtr fileDestination(DestinationFactory::MakeFileDestination(
                                       sLogPath, EnableLogRotation, MaxSizeBytes(1024 * 1024), MaxOldLogCount(5)));
//    DestinationPtr debugDestination(DestinationFactory::MakeDebugOutputDestination());
//    logger.setIncludeTimestamp(false);
    logger.addDestination(fileDestination);
//    logger.addDestination(debugDestination);

    // cppcheck-suppress memsetClassFloat
    qMemSet(quantity, 0x00, sizeof(*quantity) * MODULE_NUM);
//    qMemSet(pcs, 0x00, sizeof(pcs));
#if 0
    for (unsigned i = 0; i < PCS_NUM; ++i) {
        unsigned nodeNum = i + 1 < PCS_NUM
                ? MODULE_NUM / PCS_NUM
                : MODULE_NUM - (MODULE_NUM / PCS_NUM) * (PCS_NUM - 1);

        if (1 == PCS_NUM) {
            nodeNum = MODULE_NUM;
        }

        for (unsigned j = 0; j < nodeNum; ++j) {
            unsigned cur = (MODULE_NUM / PCS_NUM) * i + j;
            pcs[i].checkList << cur;
        }

        qMemSet(pcs[id].pileSoc, 0xFF, sizeof(pcs[id].pileSoc);
        qMemSet(pcs[id].antialiasingSoc, 0xFF, sizeof(pcs[id].antialiasingSoc);
    }
#else
    for (unsigned i = 0; i < MODULE_NUM; ++i) {
        checkList << i;
    }
#endif

    qMemSet(operatingParam, 0x00, MODULE_NUM * MAX_PARAM_NUM_PER_MODULE);
    qMemSet(warningParam, 0x00, MODULE_NUM * MAX_PARAM_NUM_PER_MODULE);
    qMemSet(calibParam, 0x00, MODULE_NUM * MAX_PARAM_NUM_PER_MODULE);

    // history table
    ui->tableHis->horizontalHeader()->setVisible(true);
    ui->tableHis->verticalHeader()->setVisible(true);
    ui->tableHis->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    ui->tableHis->horizontalHeader()->setResizeMode(2, QHeaderView::ResizeToContents);
    ui->tableHis->horizontalHeader()->setResizeMode(3, QHeaderView::ResizeToContents);

    // QCombox
    ui->balanceMode->setView(new QListView);
    ui->balancePackID->setView(new QListView);
    ui->balanceID->setView(new QListView);
    ui->balanceDir->setView(new QListView);
    ui->canRate->setView(new QListView);
    ui->device->setView(new QListView);

    // install event filter for using costum keyboard
    myKeyboard = new Mykeyboard(this);
    installKeyPad(ui->dataSaveInterval);
    installKeyPad(ui->ip);
    installKeyPad(ui->gateway);
    installKeyPad(ui->mask);
    installKeyPad(ui->dns);
    installKeyPad(ui->ip_2);
    installKeyPad(ui->gateway_2);
    installKeyPad(ui->mask_2);
    installKeyPad(ui->dns_2);
    installKeyPad(ui->calibration);
    installKeyPad(ui->dateTime);
    installKeyPad(ui->balanceTime);

    // install event filter for using touching scroll area
    installTouchScroll(ui->tableView_v);
    installTouchScroll(ui->tableView_t);
    installTouchScroll(ui->tableWidgetOverview);
    installTouchScroll(ui->tableWidgetTempHumidity);
    installTouchScroll(ui->tableWidgetDcInsulation);
    installTouchScroll(ui->tableWidgetAirConditioner);

    // statusbar
    ui->statusBar->setStyleSheet("QStatusBar{background-color: rgba(207, 207, 207, 100)}\n"
                                 "QStatusBar::item{border:0px}");
    ui->statusBar->addPermanentWidget(date);

    // faultReport table
    ui->table->horizontalHeader()->setVisible(true);
    ui->table->verticalHeader()->setVisible(true);
    ui->table->setColumnWidth(0,150);
    ui->table->setColumnWidth(1,150);
    ui->table->setColumnWidth(2,580);

    // log slider
    ui->log_slider->setMinimum(1);
    ui->log_slider->setMaximum(LOG_MAX_PAGE);

    // parameters
    ui->canRate->addItems(QStringList()<<"100"<<"125"<<"250"<<"500");
    ui->isDHCP->insertItem(0,QString("NO"));
    ui->isDHCP->insertItem(1,QString("YES"));
    ui->isDHCP->setEnabled(false);

    for(int i = 0; i < MAX_MODULE_NUM; ++i)
    {
        moduleNumList << QString::number(i + 1);
    }

    ui->comboBoxModuleNum->addItems(moduleNumList);
    ui->checkBoxRealSoc->setVisible(false);

    // gif
    movie = new QMovie(":/image/loading.gif");
    ui->sending->setMovie(movie);
    movie->start();
    ui->sending->setVisible(false);

    movie1 = new QMovie(":/image/charging.gif");
    movie2 = new QMovie(":/image/discharging.gif");
    movie3 = new QMovie(":/image/fan.gif");
    movie4 = new QMovie(":/image/fan_stop.png");

    // pack view
    myModel_v = new MyModel(0, this);
    myModel_t = new MyModel(1, this);
    ui->tableView_v->setModel(myModel_v);
    ui->tableView_t->setModel(myModel_t);

//    ui->m_para_1->installEventFilter(this);
//    ui->m_para_2->installEventFilter(this);
//    ui->f_para->installEventFilter(this);

    // manualBalance
    ui->balanceMode->insertItem(0, QString("disable"));
    ui->balanceMode->insertItem(1, QString("auto"));
    ui->balanceMode->insertItem(2, QString("manual"));
    ui->balanceMode->insertItem(3, QString("test"));
    ui->balanceMode->insertItem(4, QString("stop"));
    for (int i = 0; i < MAX_PACK_NUM; ++i) {
        ui->balancePackID->insertItem(i, QString::number(i + 1));
    }

    for(int i = 0; i < BAT_NUM; ++i) {
        ui->balanceID->insertItem(i, QString::number(i + 1));
    }
    ui->balanceDir->insertItem(0, QString("charge"));
    ui->balanceDir->insertItem(1, QString("discharge"));

    // update
    appDir = QDir(qApp->applicationDirPath());
    ui->device->insertItem(0, QString("HMI"));
    ui->device->insertItem(1, QString("MCU"));
    ui->device->insertItem(2, QString("SLAVE"));

    // load config
    config = new Config(CONFIG_PATH, MODULE_NUM, this);
    config->load(para);

    config_others = new Config(CONFIG_PATH_OTHERS, MODULE_NUM, this);
    config_others->load_others();
    systemType = config_others->systemType;

    // init quantity
    config_others->beginReadArray("module");
    for(uint i = 0; i < MODULE_NUM; ++i)
    {
        config_others->setArrayIndex(i);
        quantity[i].totalCharge = config_others->value("totalCharge", 0.0).toFloat();
        quantity[i].totalDischarge = config_others->value("totalDischarge", 0.0).toFloat();
    }
    config_others->endArray();

    // version and build time
    ui->labelVersion->setText(VERSION);
    ui->labelBuildDateTime->setText(" Build on "
                                    + buildDateTime().toString("MMM d yyyy hh:mm:ss"));

    viewPara();
    viewSysPara();
}



void MainWindow::initModbus()
{
    QString device = port2Dev(config_others->pcsDevComPort);

    modbus = modbus_new_rtu(qPrintable(device), 9600, 'N', 8, 1);
    if(modbus == NULL)
    {
        qWarning("Modbus comm init failed!");
    }

    if(0 < modbus_connect(modbus))
    {
        qWarning("Modbus comm connect failed!");
    }

    modbus_tcp = modbus_new_tcp(net.ip,MODBUS_TCP_DEFAULT_PORT);

    if(modbus_tcp == NULL)
    {
        qWarning("Modbus_tcp comm init failed!");
    }

    mb_mapping[0] = modbus_mapping_new(0,0,550,550);
    for(uint i = 0;i < MODULE_NUM;i++)
    {
        mb_mapping[i+1] = modbus_mapping_new(0,50,150,2000);
    }

    struct timeval t;
    t.tv_sec = 0;
    t.tv_usec = 100000;         // 100ms
    modbus_set_response_timeout(modbus,&t);

    t.tv_usec = 1000000;        // 1000ms
    modbus_set_response_timeout(modbus_tcp,&t);

    //map registers init
    mb_mapping[0]->tab_registers[500] = MODULE_NUM;

}

void MainWindow::initTablePeripheral()
{
    QStringList headers;

    ui->tableWidgetTempHumidity->setColumnCount(1 + sizeof(temp_humidity_meter_t) / 2);
    ui->tableWidgetTempHumidity->setRowCount(TEMP_HUMIDITY_METER_NUM);
    headers.clear();
    headers << "COMM" << QString::fromUtf8("湿度") << QString::fromUtf8("温度");
    ui->tableWidgetTempHumidity->setHorizontalHeaderLabels(headers);

    ui->tableWidgetDcInsulation->setColumnCount(1 + sizeof(dc_insulation_monitoring_device_t) / 2);
    ui->tableWidgetDcInsulation->setRowCount(DC_INSULATION_MONITORING_DEVICE_NUM);
    headers.clear();
    headers << "COMM" << QString::fromUtf8("母线电压") << QString::fromUtf8("母线总电阻")
                << QString::fromUtf8("正母线对地电阻") << QString::fromUtf8("负母线对地电阻");
    ui->tableWidgetDcInsulation->setHorizontalHeaderLabels(headers);

    ui->tableWidgetAirConditioner->setColumnCount(1 + sizeof(air_conditioner_t) / 2);
    ui->tableWidgetAirConditioner->setRowCount(AIR_CONDITIONER_NUM);
    headers.clear();
    headers << "COMM" << QString::fromUtf8("设备工作状态") << QString::fromUtf8("室内风机状态") << QString::fromUtf8("室外风机状态")
                << QString::fromUtf8("压缩风机状态") << QString::fromUtf8("柜内回风温度") << QString::fromUtf8("水泵状态") << QString::fromUtf8("柜外温度")
                    << QString::fromUtf8("冷凝器温度") << QString::fromUtf8("蒸发器温度");
    ui->tableWidgetAirConditioner->setHorizontalHeaderLabels(headers);

    ui->tableWidgetElectricityMeter->setColumnCount(1 + sizeof(electricity_meter_t) / 2);
    ui->tableWidgetElectricityMeter->setRowCount(ELECTRICITY_METER_NUM);
    headers.clear();
    headers << "COMM" << QString::fromUtf8("电度数");
    ui->tableWidgetElectricityMeter->setHorizontalHeaderLabels(headers);
}

#if 1
void MainWindow::createThread()
{
    // thread0
    hmi->OpenCan(ui->canRate->currentText().toInt());
    bgWorker[0] = new BgWorker(this, MODULE_NUM, 20);
    connect(&bgWorker[0]->thread,SIGNAL(started()),bgWorker[0],SLOT(readCan()));
    connect(bgWorker[0],SIGNAL(response_Signal(int)),this,SLOT(paraACK(int)));
    bgWorker[0]->thread.start();

    // thread1
    bgWorker[1] = new BgWorker(this, MODULE_NUM);
    connect(this,SIGNAL(readLog_Signal()),bgWorker[1],SLOT(readLog()));
    connect(this,SIGNAL(saveData_Signal()),bgWorker[1],SLOT(saveData()));

    bgWorker[1]->dir_SD.setPath(SD_PATH);
    bgWorker[1]->file.setFileName(DATA_PATH);
    bgWorker[1]->file.open(QIODevice::ReadWrite | QIODevice::Append);
    bgWorker[1]->thread.start();

    // thread2
    bgWorker[2] = new BgWorker(this, MODULE_NUM, 100);
    connect(this,SIGNAL(send2BMS_Signal()),bgWorker[2],SLOT(send2BMS()));
    bgWorker[2]->thread.start();

    // thread3
    bgWorker[3] = new BgWorker(this, MODULE_NUM);
    /*connect(this,SIGNAL(checkWarning_Signal()),bgWorker[3],SLOT(checkWarning()));*/
    connect(this,SIGNAL(modbusDataMap_Signal()),bgWorker[3],SLOT(modbusDataMap()));
    bgWorker[3]->thread.start();

    // thread4
    bgWorker[4] = new BgWorker(this, MODULE_NUM);
    connect(&bgWorker[4]->thread,SIGNAL(started()),bgWorker[4],SLOT(modbusTcpServer()));
    bgWorker[4]->thread.start();

    // thread5
    bgWorker[5] = new BgWorker(this, MODULE_NUM);
    connect(this,SIGNAL(copyData_Signal(QString)),bgWorker[5],SLOT(copyData(QString)));
    connect(bgWorker[5],SIGNAL(processDone_Signal()),this,SLOT(processDone()));
    connect(this,SIGNAL(updateMCU_Signal(QString, int)),bgWorker[5],SLOT(updateMCU(QString, int)));
    connect(bgWorker[5],SIGNAL(updatingMCU_Signal(int)),this,SLOT(updatingMCU(int)));
    bgWorker[5]->thread.start();

    // thread6
    bgWorker[6] = new BgWorker(this, MODULE_NUM, 500);
    if (2 == systemType) {
        modbus_close(modbus);
        modbus_free(modbus);
        modbus = NULL;
        connect(&bgWorker[6]->thread ,SIGNAL(started()), bgWorker[6], SLOT(pcs()));
    } else {
        connect(this, SIGNAL(send2PCS_Signal()), bgWorker[6], SLOT(send2PCS()));
    }
    bgWorker[6]->thread.start();

    // thread7
    bgWorker[7] = new BgWorker(this, MODULE_NUM);
    connect(&bgWorker[7]->thread, SIGNAL(started()), bgWorker[7], SLOT(doQueryPeripheral()));
    bgWorker[7]->thread.start();

    // thread8
    bgWorker[8] = new BgWorker(this, MODULE_NUM);
    connect(&bgWorker[8]->thread, SIGNAL(started()), bgWorker[8], SLOT(runIEC104Slave()));
    bgWorker[8]->thread.start();

    // thread9
    bgWorker[9] = new BgWorker(this, MODULE_NUM);
    connect(this, SIGNAL(send2EMS_Signal()), bgWorker[9], SLOT(send2EMS()));
    bgWorker[9]->thread.start();

    // thread10
    bgWorker[10] = new BgWorker(this, MODULE_NUM);
    if (0 == systemType) {
        connect(this, SIGNAL(feedBack2BMS_Signal()), bgWorker[10], SLOT(feedBack2BMS()));
    }
    bgWorker[10]->thread.start();

    // file copy thread
    copier = new FileCopier;
    copyThread = new QThread;
    connect(this, SIGNAL(startCopyRsquested()), copier, SLOT(startCopying()));
    connect(this, SIGNAL(cancelCopuRequested()), copier, SLOT(cancelCopying()));
    connect(copier, SIGNAL(errorOccurred()),this, SLOT(errorHandleSlot()));
    connect(copier, SIGNAL(percentCopied(double)), this, SLOT(updateCopyProgress(double)));
//    connect(copier, SIGNAL(finished()), this, SLOT(copyFinishSlot()));
    connect(copier, SIGNAL(finished()), this, SLOT(dataDirCpyWrapper()));
    copier->moveToThread(copyThread);
    copyThread->start();
}
#else
void MainWindow::createThread()
{
    // thread1
    bgWorker[1] = new BgWorker(this, MODULE_NUM);
    connect(this,SIGNAL(readLog_Signal()),bgWorker[1],SLOT(readLog()));
    bgWorker[1]->thread.start();

    // file copy thread
    copier = new FileCopier;
    copyThread = new QThread;
    connect(this, SIGNAL(startCopyRsquested()), copier, SLOT(startCopying()));
    connect(this, SIGNAL(cancelCopuRequested()), copier, SLOT(cancelCopying()));
    connect(copier, SIGNAL(errorOccurred()),this, SLOT(errorHandleSlot()));
    connect(copier, SIGNAL(percentCopied(double)), this, SLOT(updateCopyProgress(double)));
//    connect(copier, SIGNAL(finished()), this, SLOT(copyFinishSlot()));
    connect(copier, SIGNAL(finished()), this, SLOT(dataDirCpyWrapper()));
    copier->moveToThread(copyThread);
    copyThread->start();
}
#endif

#if 1
void MainWindow::connectSignal()
{
    // switch page
    connect(ui->pushButton0_0,SIGNAL(clicked()),this,SLOT(switchPage()));
    connect(ui->pushButton0_1,SIGNAL(clicked()),this,SLOT(switchPage()));
    connect(ui->pushButton0_2,SIGNAL(clicked()),this,SLOT(switchPage()));
    connect(ui->pushButton0_3,SIGNAL(clicked()),this,SLOT(switchPage()));
    connect(ui->pushButton0_4,SIGNAL(clicked()),this,SLOT(switchPage()));
    connect(ui->pushButton0_5,SIGNAL(clicked()),this,SLOT(switchPage()));
    connect(ui->pushButton1_0,SIGNAL(clicked()),this,SLOT(switchPage()));
    connect(ui->pushButton1_1,SIGNAL(clicked()),this,SLOT(switchPage()));
    connect(ui->pushButton1_2,SIGNAL(clicked()),this,SLOT(switchPage()));
    connect(ui->pushButton2_0,SIGNAL(clicked()),this,SLOT(switchPage()));
    connect(ui->pushButton2_1,SIGNAL(clicked()),this,SLOT(switchPage()));
    connect(ui->pushButton2_2,SIGNAL(clicked()),this,SLOT(switchPage()));

    // module change
    connect(ui->m_left,SIGNAL(clicked()),this,SLOT(moduleChange()));
    connect(ui->m_right,SIGNAL(clicked()),this,SLOT(moduleChange()));
    connect(ui->m_left_2,SIGNAL(clicked()),this,SLOT(moduleChange()));
    connect(ui->m_right_2,SIGNAL(clicked()),this,SLOT(moduleChange()));

    // pack change
    connect(ui->p_voltage,SIGNAL(clicked()),this,SLOT(packChange()));
    connect(ui->p_temp,SIGNAL(clicked()),this,SLOT(packChange()));
    connect(ui->p_fan,SIGNAL(clicked()),this,SLOT(packChange()));

    // peripheral change
    connect(ui->peripheral_1, SIGNAL(clicked()), this, SLOT(peripheralChange()));
    connect(ui->peripheral_2, SIGNAL(clicked()), this, SLOT(peripheralChange()));
    connect(ui->peripheral_3, SIGNAL(clicked()), this, SLOT(peripheralChange()));
    connect(ui->peripheral_4, SIGNAL(clicked()), this, SLOT(peripheralChange()));

    // mute
    connect(ui->mute,SIGNAL(clicked()),this,SLOT(muteClicked()));

    // parameter change
    connect(ui->para_left,SIGNAL(clicked()),this,SLOT(paramClusterChanged()));
    connect(ui->para_right,SIGNAL(clicked()),this,SLOT(paramClusterChanged()));
    connect(ui->pushButtonOperationParaPageUp, SIGNAL(clicked()), this, SLOT(paramPageFlip()));
    connect(ui->pushButtonOperationParaPageDown, SIGNAL(clicked()), this, SLOT(paramPageFlip()));
    connect(ui->pushButtonWarningParaPageUp, SIGNAL(clicked()), this, SLOT(paramPageFlip()));
    connect(ui->pushButtonWarningParaPageDown, SIGNAL(clicked()), this, SLOT(paramPageFlip()));
    connect(ui->pushButtonCalibrationParaPageUp, SIGNAL(clicked()), this, SLOT(paramPageFlip()));
    connect(ui->pushButtonCalibrationParaPageDown, SIGNAL(clicked()), this, SLOT(paramPageFlip()));

    // log change
    connect(ui->log_left,SIGNAL(clicked()),this,SLOT(logChange()));
    connect(ui->log_right,SIGNAL(clicked()),this,SLOT(logChange()));
    connect(ui->log_slider,SIGNAL(valueChanged(int)),this,SLOT(logChange()));
    connect(ui->log_1,SIGNAL(clicked()),this,SLOT(logChange()));
    connect(ui->log_2,SIGNAL(clicked()),this,SLOT(logChange()));
    connect(ui->log_num,SIGNAL(textChanged(QString)),this,SLOT(logNumChange(QString)));
    connect(ui->pushButton2_1,SIGNAL(clicked()),this,SLOT(viewLog()));

    // send relay
    connect(ui->m_mRelay_btn,SIGNAL(clicked()),this,SLOT(sendRelay()));
    connect(ui->m_nRelay_btn,SIGNAL(clicked()),this,SLOT(sendRelay()));
    connect(ui->m_pRelay_btn,SIGNAL(clicked()),this,SLOT(sendRelay()));
    connect(ui->m_fMode_btn,SIGNAL(clicked()),this,SLOT(sendRelay()));
    // start balance
    connect(ui->startBalance,SIGNAL(clicked()),this,SLOT(startBalance()));

    // send command
    connect(ui->pushButtonClbr11_0, SIGNAL(clicked()), this, SLOT(sendCommand()));
    connect(ui->pushButtonClbr11_1, SIGNAL(clicked()), this, SLOT(sendCommand()));
    connect(ui->pushButtonClbr11_2, SIGNAL(clicked()), this, SLOT(sendCommand()));
    connect(ui->pushButtonClbr11_3, SIGNAL(clicked()), this, SLOT(sendCommand()));
    connect(ui->pushButtonClbr11_4, SIGNAL(clicked()), this, SLOT(sendCommand()));
    connect(ui->pushButtonClbr11_5, SIGNAL(clicked()), this, SLOT(sendCommand()));
    connect(ui->pushButtonClbr11_6, SIGNAL(clicked()), this, SLOT(sendCommand()));
    connect(ui->pushButtonClbr11_7, SIGNAL(clicked()), this, SLOT(sendCommand()));

    // send parameter
    connect(ui->pushButtonSendOprtParam,SIGNAL(clicked()),this,SLOT(sendPara()));
    connect(ui->pushButtonSendWarnParam,SIGNAL(clicked()),this,SLOT(sendPara()));
    connect(ui->pushButtonSendClbrParam,SIGNAL(clicked()),this,SLOT(sendPara()));
    connect(ui->c_send1,SIGNAL(clicked()),this,SLOT(sendPara()));
    connect(ui->checkBoxSelectAllOprtParam,SIGNAL(clicked()),this,SLOT(selectAllPara()));
    connect(ui->checkBoxSelectAllWarnParam,SIGNAL(clicked()),this,SLOT(selectAllPara()));
    connect(ui->checkBoxSelectAllClbrParam,SIGNAL(clicked()),this,SLOT(selectAllPara()));

    // param stacked widget
    connect(ui->stackedWidgetOperationPara, SIGNAL(currentChanged(int)), this, SLOT(paramPageChanged(int)));
    connect(ui->stackedWidgetWarningPara, SIGNAL(currentChanged(int)), this, SLOT(paramPageChanged(int)));
    connect(ui->stackedWidgetCalibrationPara, SIGNAL(currentChanged(int)), this, SLOT(paramPageChanged(int)));

    // save parameter
//    connect(ui->w_save_1,SIGNAL(clicked()),this,SLOT(savePara()));
//    connect(ui->w_save_2,SIGNAL(clicked()),this,SLOT(savePara()));

    // SysPara
    connect(ui->apply1,SIGNAL(clicked()),this,SLOT(saveSysPara()));
    connect(ui->apply2,SIGNAL(clicked()),this,SLOT(saveSysPara()));
    connect(ui->apply2_2,SIGNAL(clicked()),this,SLOT(saveSysPara()));
    connect(ui->apply3,SIGNAL(clicked()),this,SLOT(saveSysPara()));
    connect(ui->apply4,SIGNAL(clicked()),this,SLOT(saveSysPara()));
    connect(ui->reset,SIGNAL(clicked()),this,SLOT(viewSysPara()));
    connect(ui->reboot,SIGNAL(clicked()),this,SLOT(reboot()));
    connect(ui->pushButtonPortMgr, SIGNAL(clicked()), portMgrDialog, SLOT(show()));

    connect(ui->d0,SIGNAL(clicked()),this,SLOT(changeBacklight()));
    connect(ui->d1,SIGNAL(clicked()),this,SLOT(changeBacklight()));
    connect(ui->d2,SIGNAL(clicked()),this,SLOT(changeBacklight()));
    connect(ui->d3,SIGNAL(clicked()),this,SLOT(changeBacklight()));

    // save data
    connect(ui->usb,SIGNAL(clicked()),this,SLOT(checkDisk()));
    connect(ui->sd,SIGNAL(clicked()),this,SLOT(checkDisk()));
    connect(ui->save,SIGNAL(clicked()),this,SLOT(processData()));

    // update
    connect(ui->openFile,SIGNAL(clicked()),this,SLOT(openFile()));
    connect(ui->update,SIGNAL(clicked()),this,SLOT(do_update()));

    // facotry dialog
    connect(ui->pushButtonFactoryCalibration, SIGNAL(clicked()), this, SLOT(showFactoryDialog()));
//    connect(ui->iec104RemoteSignalingTest, SIGNAL(clicked()), this, SLOT(showFactoryDialog()));
    connect(factoryDialog, SIGNAL(sendFactoryCalib(int, int)), this, SLOT(doSendFactoryCalib(int, int)));

    // request parallel operation
    for (unsigned i = 0; i < MODULE_NUM; ++i) {
        connect(t->labelM[i], SIGNAL(clicked()), this, SLOT(doParallelOperation()));
    }

    // digital input/output, peripheral
    connect(hmi, SIGNAL(readyReadDigiInput(quint8, quint8, QBitArray))
            , this, SLOT(readDigitalInput(quint8, quint8, QBitArray)));
    connect(hmi, SIGNAL(readyReadDigiOutput(quint8, quint8, QBitArray))
            , this, SLOT(readDigitalOutput(quint8, quint8, QBitArray)));
    connect(hmi, SIGNAL(readyReadDigiInput(quint8, quint8, QBitArray))
            , factoryDialog, SLOT(readDigitalInput(quint8, quint8, QBitArray)));
    connect(hmi, SIGNAL(readyReadDigiOutput(quint8, quint8, QBitArray))
            , factoryDialog, SLOT(readDigitalOutput(quint8, quint8, QBitArray)));
    connect(hmi, SIGNAL(readyReadDigiInput(quint8, quint8, QBitArray))
            , this, SLOT(readDigitalInput(quint8, quint8, QBitArray)));
//    connect(hmi, SIGNAL(readyReadDigiOutput(quint8, quint8, QBitArray))
//            , this, SLOT(readDigitalOutput(quint8, quint8, QBitArray)));
    connect(hmi, SIGNAL(readyReadPeripheral(int, int, int))
            , this, SLOT(readPeripheral(int, int, int)));

    // system type
    connect(ui->systemType, SIGNAL(clicked()), systemTypeDialog, SLOT(show()));
    connect(systemTypeDialog, SIGNAL(finished(int)), ui->systemType, SLOT(setNum(int)));

    connect(portMgrDialog, SIGNAL(finished(int)), this, SLOT(viewSysPara()));
}
#else
void MainWindow::connectSignal()
{
    // switch page
    connect(ui->pushButton0_0,SIGNAL(clicked()),this,SLOT(switchPage()));
    connect(ui->pushButton0_1,SIGNAL(clicked()),this,SLOT(switchPage()));
    connect(ui->pushButton0_2,SIGNAL(clicked()),this,SLOT(switchPage()));
    connect(ui->pushButton0_3,SIGNAL(clicked()),this,SLOT(switchPage()));
    connect(ui->pushButton0_4,SIGNAL(clicked()),this,SLOT(switchPage()));
    connect(ui->pushButton1_0,SIGNAL(clicked()),this,SLOT(switchPage()));
    connect(ui->pushButton1_1,SIGNAL(clicked()),this,SLOT(switchPage()));
    connect(ui->pushButton1_2,SIGNAL(clicked()),this,SLOT(switchPage()));
    connect(ui->pushButton2_0,SIGNAL(clicked()),this,SLOT(switchPage()));
    connect(ui->pushButton2_1,SIGNAL(clicked()),this,SLOT(switchPage()));
    connect(ui->pushButton2_2,SIGNAL(clicked()),this,SLOT(switchPage()));

    // module change
    connect(ui->m_left,SIGNAL(clicked()),this,SLOT(moduleChange()));
    connect(ui->m_right,SIGNAL(clicked()),this,SLOT(moduleChange()));
    connect(ui->m_left_2,SIGNAL(clicked()),this,SLOT(moduleChange()));
    connect(ui->m_right_2,SIGNAL(clicked()),this,SLOT(moduleChange()));

    // pack change
    connect(ui->p_voltage,SIGNAL(clicked()),this,SLOT(packChange()));
    connect(ui->p_temp,SIGNAL(clicked()),this,SLOT(packChange()));
    connect(ui->p_fan,SIGNAL(clicked()),this,SLOT(packChange()));

    // peripheral change
    connect(ui->peripheral_1, SIGNAL(clicked()), this, SLOT(peripheralChange()));
    connect(ui->peripheral_2, SIGNAL(clicked()), this, SLOT(peripheralChange()));
    connect(ui->peripheral_3, SIGNAL(clicked()), this, SLOT(peripheralChange()));

    // mute
    connect(ui->mute,SIGNAL(clicked()),this,SLOT(muteClicked()));

    // parameter change
    connect(ui->para_left,SIGNAL(clicked()),this,SLOT(paramClusterChanged()));
    connect(ui->para_right,SIGNAL(clicked()),this,SLOT(paramClusterChanged()));
    connect(ui->pushButtonOperationParaPageUp, SIGNAL(clicked()), this, SLOT(paramPageFlip()));
    connect(ui->pushButtonOperationParaPageDown, SIGNAL(clicked()), this, SLOT(paramPageFlip()));
    connect(ui->pushButtonWarningParaPageUp, SIGNAL(clicked()), this, SLOT(paramPageFlip()));
    connect(ui->pushButtonWarningParaPageDown, SIGNAL(clicked()), this, SLOT(paramPageFlip()));
    connect(ui->pushButtonCalibrationParaPageUp, SIGNAL(clicked()), this, SLOT(paramPageFlip()));
    connect(ui->pushButtonCalibrationParaPageDown, SIGNAL(clicked()), this, SLOT(paramPageFlip()));

    // log change
    connect(ui->log_left,SIGNAL(clicked()),this,SLOT(logChange()));
    connect(ui->log_right,SIGNAL(clicked()),this,SLOT(logChange()));
    connect(ui->log_slider,SIGNAL(valueChanged(int)),this,SLOT(logChange()));
    connect(ui->log_1,SIGNAL(clicked()),this,SLOT(logChange()));
    connect(ui->log_2,SIGNAL(clicked()),this,SLOT(logChange()));
    connect(ui->log_num,SIGNAL(textChanged(QString)),this,SLOT(logNumChange(QString)));
    connect(ui->pushButton2_1,SIGNAL(clicked()),this,SLOT(viewLog()));

    // save data
    connect(ui->usb,SIGNAL(clicked()),this,SLOT(checkDisk()));
    connect(ui->sd,SIGNAL(clicked()),this,SLOT(checkDisk()));
    connect(ui->save,SIGNAL(clicked()),this,SLOT(processData()));
}
#endif

void MainWindow::initPara(QString flag,int dest)
{
    //read para
    readPara(flag,dest);

    QTimer::singleShot(2000,this,SLOT(viewPara()));
}

void MainWindow::switchPage()
{

    QPushButton *btn = static_cast<QPushButton *>(sender());

    QFrame *frame = static_cast<QFrame *>(btn->parent());
    if(frame != NULL)
    {
        frame->setStyleSheet(QString::fromUtf8("QPushButton{background-color:rgba(255, 255, 255, 0);}\n"
          "QPushButton#%1{background-color:rgb(202, 248, 255);}").arg(btn->objectName()));
    }

    QByteArray byteArray = btn->objectName().toLatin1();
    char a = byteArray.at(10);
    char b = byteArray.at(12);
    if(a == '0')
        ui->stackedWidgetBodyInfo->setCurrentIndex(chartonum(b));
    else if(a == '1')
        ui->stackedWidgetBodyPara->setCurrentIndex(chartonum(b));
    else if(a == '2')
        ui->stackedWidgetBodySystem->setCurrentIndex(chartonum(b));

}


void MainWindow::showMsg(QString text, int timeout)
{
    ui->statusBar->showMessage(text, timeout);
}

void MainWindow::freshUI()
{
    /*************fault**********************/
    faultReport();
    /*************System**********************/
    viewSystem();
    /*************Module**********************/
    viewModule();
    /*************Pack**********************/
    viewPack();
    /*************log**********************/
    viewLog();
}

void MainWindow::viewSystem()
{
    for(uint i = 0;i < MODULE_NUM;i++)
    {
        quint16 moduleSoc = config_others->value("isRealSocEnabled", false).toBool()
                ? module[i].moduleSoc
                : fictitiousSoc[i].soc;

        viewSOC(i, moduleSoc);
        viewState(i);
        viewVC(i);
        viewFault(i);
    }

    ui->tableWidgetOverview->setItem(0, 1, new QTableWidgetItem(QString::fromUtf8("  %L1 kΩ").arg((float)heap.ir,0,'f',1)));
    ui->tableWidgetOverview->setItem(0, 3, new QTableWidgetItem(QString::fromUtf8("  %1 V").arg((float)heap.bat_voltage_max/1000,0,'f',3)));
    ui->tableWidgetOverview->setItem(0, 5, new QTableWidgetItem(QString::fromUtf8("  %1 ℃").arg((float)heap.bat_temp_max/10,0,'f',1)));
    ui->tableWidgetOverview->setItem(0, 7, new QTableWidgetItem(QString::fromUtf8("  %1 %").arg((float)heap.soh/10,0,'f',1)));
    ui->tableWidgetOverview->setItem(0, 11, new QTableWidgetItem(QString::fromUtf8("  %1 KWh").arg((float)heap.chargingCapacity,0,'f',1)));
    ui->tableWidgetOverview->setItem(1, 1, new QTableWidgetItem(QString::fromUtf8("  %1 %").arg((float)heap.soc/10,0,'f',1)));
    ui->tableWidgetOverview->setItem(1, 3, new QTableWidgetItem(QString::fromUtf8("  %1 V").arg((float)heap.bat_voltage_min/1000,0,'f',3)));
    ui->tableWidgetOverview->setItem(1, 5, new QTableWidgetItem(QString::fromUtf8("  %1 ℃").arg((float)heap.bat_temp_min/10,0,'f',1)));
    ui->tableWidgetOverview->setItem(1, 7, new QTableWidgetItem(QString::fromUtf8("  %1 Wh").arg((float)heap.soe,0,'f',1)));
    ui->tableWidgetOverview->setItem(1, 11, new QTableWidgetItem(QString::fromUtf8("  %1 KWh").arg((float)heap.dischargingCapacity,0,'f',1)));
    ui->tableWidgetOverview->resizeColumnsToContents();
}

void MainWindow::viewSOC(int id,uint16_t soc)
{
    if(MAX_MODULE_NUM > id && 0 <= id)
    {
        if(NULL == t || NULL == t->labelSOC[id] || NULL == t->labelQuantity[id])
        {
            return;
        }

        t->labelSOC[id]->setText(QString::fromUtf8("%1 %").arg((float)soc/10,0,'f',1));
        t->labelQuantity[id]->setText(QString::fromUtf8("%1 kWh").arg((float)(Q_MAX*soc)/10000,0,'f',1));
    }

    updateBackgroundBySOC(id, soc);
}

void MainWindow::viewState(int id)
{
    QLabel *label = NULL;

    if(MAX_MODULE_NUM > id && 0 <= id)
    {
        label = t->labelM[id];
    }

    if(label != NULL)
    {
        if(!module[id].mainRelay) {
            label->setPixmap(QPixmap(":/image/disconnect.png"));
            return;
        }

        if(module[id].chargeState == 0x01)
        {
            if(label->movie() != movie1)
            {
                label->setMovie(movie1);
                label->movie()->start();
            }
            label->show();
        }
        else if(module[id].chargeState == 0x02)
        {
            if(label->movie() != movie2)
            {
                label->setMovie(movie2);
                label->movie()->start();
            }
            label->show();
        }
        else
        {
           label->setPixmap(QPixmap(":/image/connect.png"));
        }
    }
}

void MainWindow::viewFault(int id)
{
    QLabel *label = NULL;

    if(MAX_MODULE_NUM > id && 0 <= id)
    {
        label = t->labelWarn[id];
    }

    if(label != NULL)
    {
        do {
            if (faultBit & 0x01 << id) {
                label->setPixmap(QPixmap(":/image/error.png"));
                label->setVisible(true);
                label->raise();
                break;
            }

            if (warningBit & 0x01 << id) {
                label->setPixmap(QPixmap(":/image/warning.png"));
                label->setVisible(true);
                label->raise();
            } else {
                label->setVisible(false);
            }
        } while (0);
    }
}

void MainWindow::viewVC(int id)
{
    QLabel *label1 = NULL;
    QLabel *label2 = NULL;

    if(MAX_MODULE_NUM > id && 0 <= id)
    {
        label1 = t->labelVoltage[id];
        label2 = t->labelCurrent[id];
    }

    if(label1 != NULL)
    {
        label1->setText(QString::fromUtf8("%1 V").arg((float)module[id].moduleVoltage/10,0,'f',1));
    }

    if(label2 != NULL)
    {
        label2->setText(QString::fromUtf8("%1 A").arg((float)module[id].moduleCurrent/10,0,'f',1));
    }
}

void MainWindow::checkCOMM()
{
    QMutexLocker locker(&HeartMutex);

    static int *count = (int *)qMalloc(sizeof(int) * MODULE_NUM);
    static bool _atLeastOneHeartbeat = false;
    bool atLeastOneHeartbeat = false;
    QLabel *label = NULL;
    QList<unsigned> clearList;

    clearList.clear();
    for (uint id = 0; id < MODULE_NUM; id++) {
        if (MAX_MODULE_NUM > id) {
            label = t->labelCanComm[id];
        }

        if (label != NULL) {
            if(module[id].heartbeat)
            {
                atLeastOneHeartbeat = true;
                label->setVisible(false);
                count[id] = 0;
//                module[id].heartbeat_bak = module[id].heartbeat;
            }
            else
            {
                ++count[id];
                clearList << id;
            }

            if (1 < count[id]) {
                if(module[id].heartbeat ^ module[id].heartbeat_bak)
                {
                    qWarning("Fault: 0x%06X, Info: %s",(id+1)<<16,"Can Communication error ");
                }

                module[id].heartbeat_bak = module[id].heartbeat;
            }

            module[id].heartbeat = false;
        }
    }

    if (!atLeastOneHeartbeat) {
        hmi->RestartCan(500);
    }

    qDebug() << __func__ << atLeastOneHeartbeat << _atLeastOneHeartbeat << clearList.size() << MODULE_NUM;
    if (0 < MODULE_NUM - clearList.size()) {
        foreach (unsigned i, clearList) {
            if (1 < count[i]) {
                count[i] = 0;
                clearModuleData(i);
                clearModuleWarning(i + 1);
            }

            if (MAX_MODULE_NUM > i) {
                label = t->labelCanComm[i];
            }

            if (NULL == label) {
                continue;
            }

            label->setVisible(true);
            label->raise();
        }
    } else if (_atLeastOneHeartbeat) {
        for (unsigned i = 0; i < MODULE_NUM; ++i) {
            if (MAX_MODULE_NUM > i) {
                label = t->labelCanComm[i];
            }

            if (NULL == label) {
                continue;
            }

            label->setVisible(true);
            label->raise();
        }
    } else {
        for (unsigned i = 0; i < MODULE_NUM; ++i) {
            clearModuleData(i);
            clearModuleWarning(i + 1);
        }
    }
    _atLeastOneHeartbeat = atLeastOneHeartbeat;
}

void MainWindow::moduleChange()
{
    QObject *obj = sender();

    if(NULL != obj)
    {
        if((obj->objectName() == "m_left") || (obj->objectName() == "m_left_2"))
        {
            if(--m_page < 1)m_page = MODULE_NUM;
        }
        else if((obj->objectName() == "m_right") || (obj->objectName() == "m_right_2"))
        {
            if(++m_page > MODULE_NUM)m_page = 1;
        }
    }

    ui->m_num->setText(QString("%1 / %2").arg(m_page).arg(MODULE_NUM));
    ui->m_num_2->setText(QString("%1 / %2").arg(m_page).arg(MODULE_NUM));

    viewModule();
    viewPack();
}

void MainWindow::packChange()
{
    QPushButton *btn = static_cast<QPushButton *>(sender());

    if(btn->objectName() == "p_voltage"){
        ui->packViewer->setCurrentIndex(0);
    }
    else if(btn->objectName() == "p_temp"){
        ui->packViewer->setCurrentIndex(1);
    }
    else if(btn->objectName() == "p_fan"){
        ui->packViewer->setCurrentIndex(2);
    }
}

void MainWindow::peripheralChange()
{
    QObject *obj = this->sender();

    if (NULL == obj) {
        return;
    }

    if (obj->objectName().contains("1", Qt::CaseInsensitive)) {
        ui->stackedWidgetPeriphral->setCurrentIndex(0);
    } else if (obj->objectName().contains("2", Qt::CaseInsensitive)) {
        ui->stackedWidgetPeriphral->setCurrentIndex(1);
    } else if (obj->objectName().contains("3", Qt::CaseInsensitive)) {
        ui->stackedWidgetPeriphral->setCurrentIndex(2);
    } else if (obj->objectName().contains("4", Qt::CaseInsensitive)) {
        ui->stackedWidgetPeriphral->setCurrentIndex(3);
    }
}

void MainWindow::paramClusterChanged()
{
   QObject *obj = this->sender();

   if (NULL != obj) {
       if (obj->objectName() == "para_left") {
           if (--para_page < 1) {
               para_page = MODULE_NUM;
           }
       }
       else if (obj->objectName() == "para_right") {
           if (++para_page > MODULE_NUM) {
               para_page = 1;
           }
       }
   }

   ui->para_num->setText(QString("%1 / %2").arg(para_page).arg(MODULE_NUM));
   viewPara();
}

void MainWindow::paramPageFlip()
{
    QString objName;
    QStackedWidget *stkWidget = NULL;
    int currentPageIndex = 0;
    int lastPageIndex = 0;

    if (NULL == sender()) {
        return;
    }

    objName = sender()->objectName();
    if (objName.contains("operation", Qt::CaseInsensitive)) {
        stkWidget = ui->stackedWidgetOperationPara;
    } else if(objName.contains("warning", Qt::CaseInsensitive)) {
        stkWidget = ui->stackedWidgetWarningPara;
    } else if (objName.contains("calibration", Qt::CaseInsensitive)) {
        stkWidget = ui->stackedWidgetCalibrationPara;
    }

    if(NULL == stkWidget)
    {
        return;
    }

    currentPageIndex = stkWidget->currentIndex();
    lastPageIndex = stkWidget->count() - 1;

    if(objName.contains("up", Qt::CaseInsensitive) && currentPageIndex > 0)
    {
        stkWidget->setCurrentIndex(--currentPageIndex);
    }
    else if(objName.contains("down", Qt::CaseInsensitive) && currentPageIndex < lastPageIndex) {
        stkWidget->setCurrentIndex(++currentPageIndex);
    }

//    if(ui->stackedWidgetOperationPara == stkWidget)
//    {
//        if(currentPageIndex < lastPageIndex)
//        {
//            ui->label_para->setText(tr("Operating"));
//        }
//        else {
//            ui->label_para->setText(tr("Fault"));
//        }
//    }
}

void MainWindow::paramPageChanged(int index)
{
    QString objName;
    QStackedWidget *stkWidget = NULL;

    if (NULL == sender()) {
        return;
    }

    objName = sender()->objectName();
    if (objName.contains("operation", Qt::CaseInsensitive)) {
        stkWidget = ui->stackedWidgetOperationPara;
        ui->checkBoxFreezeOprtParam->setVisible((stkWidget->count() - index) > initOprtParamPage);
        ui->checkBoxSelectAllOprtParam->setVisible((stkWidget->count() - index) > initOprtParamPage);
        ui->pushButtonSendOprtParam->setVisible((stkWidget->count() - index) > initOprtParamPage);
    } else if(objName.contains("warning", Qt::CaseInsensitive)) {
        stkWidget = ui->stackedWidgetWarningPara;
        ui->checkBoxFreezeWarnParam->setVisible((stkWidget->count() - index) > initWarnParamPage);
        ui->checkBoxSelectAllWarnParam->setVisible((stkWidget->count() - index) > initWarnParamPage);
        ui->pushButtonSendWarnParam->setVisible((stkWidget->count() - index) > initWarnParamPage);
    } else if (objName.contains("calibration", Qt::CaseInsensitive)) {
        stkWidget = ui->stackedWidgetCalibrationPara;
//        ui->checkBoxFreezeClbrParam->setVisible((stkWidget->count() - index) > initClbrParamPage);
        ui->checkBoxSelectAllClbrParam->setVisible((stkWidget->count() - index) > initClbrParamPage);
        ui->pushButtonSendClbrParam->setVisible((stkWidget->count() - index) > initClbrParamPage);
    }
}

void MainWindow::logChange()
{
   QObject *obj = sender();
   if(obj->objectName() == "log_left"){
       ui->log_num->setText(QString::number((ui->log_num->text().toInt() - 1) < 1 ? LOG_MAX_PAGE : (ui->log_num->text().toInt() - 1)));
   }else if(obj->objectName() == "log_right"){
       ui->log_num->setText(QString::number((ui->log_num->text().toInt() + 1) > LOG_MAX_PAGE ? 1 : (ui->log_num->text().toInt() + 1)));
   }else if(obj->objectName() == "log_slider"){
       ui->log_num->setText(QString::number(ui->log_slider->value()));
       ui->log_percent->setText(QString("%1%").arg((float)ui->log_slider->value()*100/ui->log_slider->maximum(),0,'f',1));
   }else if(obj->objectName() == "log_1"){
       logType = 0;
   }else if(obj->objectName() == "log_2"){
       logType = 1;
   }

   viewLog();
}

void MainWindow::logNumChange(QString num)
{
    ui->log_slider->setValue(num.toInt());
}

void MainWindow::showFactoryDialog()
{
    int page = 0;

    if (NULL == sender()) {
        return;
    }

    if (sender() == ui->pushButtonFactoryCalibration) {
//        page = CoefCali;
        page = DigitalIO;
    } /*else if (sender() == ui->iec104RemoteSignalingTest) {
//        page = RmtSigl;
        page = DbgMsgType;
    }*/

    factoryDialog->showPage(page);
}

void MainWindow::viewModule()
{
    Module *m = NULL;
    quint16 moduleSoc = 0;

    if(NULL == this->module) {
        return;
    }

    m = this->module + (m_page-1);
    moduleSoc = config_others->value("isRealSocEnabled", false).toBool()
            ? m->moduleSoc
            : fictitiousSoc[m_page-1].soc;

    if(m->chargeState == 0x01)
    {
        ui->m_state->setText(tr("charging"));
    }
    else if(m->chargeState == 0x02)
    {
        ui->m_state->setText(tr("discharging"));
    }
    else if(m->chargeState == 0x03)
    {
        ui->m_state->setText(tr("open-circuit"));
    }
    else if(m->chargeState == 0x04)
    {
        ui->m_state->setText(tr("fault"));
    }
    else
    {
        ui->m_state->setText(tr("standby"));
    }

    ui->m_soc->setText(QString::fromUtf8("%1 %").arg((float)moduleSoc/10,0,'f',1));
    ui->m_soh->setText(QString::fromUtf8("%1 %").arg((float)m->moduleSoh/10,0,'f',1));
    ui->m_soe->setText(QString::fromUtf8("%1 %").arg(m->moduleSoe));
    ui->m_voltage->setText(QString::fromUtf8("%1 V").arg((float)m->moduleVoltage/10,0,'f',1));
    ui->m_current->setText(QString::fromUtf8("%1 A").arg((float)m->moduleCurrent/10,0,'f',1));
    ui->m_ir->setText(QString::fromUtf8("%L1 kΩ").arg((float)m->insulationRes,0,'f',1));

    // TO-DO: use real value to calculate instead of a constant.
    // real_capacity=nominal_capacity*soh
    // residual_capacity=real_capacity*soc
    ui->m_realCap->setText(QString::fromUtf8("%1 Ah").arg(BATTERY_NORMINAL_CAPACITY / 1.0, 0, 'f', 1));
    ui->m_resCap->setText(QString::fromUtf8("%1 Ah").arg(BATTERY_NORMINAL_CAPACITY * m->moduleSoc * 0.001, 0, 'f', 1));
    ui->m_nomCap->setText(QString::fromUtf8("%1 Ah").arg(BATTERY_NORMINAL_CAPACITY / 1.0, 0, 'f', 1));

    ui->m_prechargevoltage->setText(QString("%1 V").arg((float)m->afterPrechargeVoltage / 10.0, 0, 'f', 1));
    ui->m_tempriserate->setText(QString::fromUtf8("%1 ℃").arg((float)m->tempRiseRate/10,0,'f',1));

    ui->m_sc->setText(QString::fromUtf8("%1 kWh").arg(m->chargingCapacityOnce));
    ui->m_sd->setText(QString::fromUtf8("%1 kWh").arg(m->dischargingCapacityOnce));
    ui->m_tc->setText(QString::fromUtf8("%L1 KWh").arg(m->chargingCapacity));
    ui->m_td->setText(QString::fromUtf8("%L1 KWh").arg(m->dischargingCapacity));

    ui->m_sw->setText(QString::fromUtf8("V%1 (V%2)").arg((float)m->BCU_swVer/100,0,'f',2).arg((float)m->BMU_swVer/100,0,'f',2));
    ui->m_hw->setText(QString::fromUtf8("V%1 (V%2)").arg((float)m->BCU_hwVer/100,0,'f',2).arg((float)m->BMU_hwVer/100,0,'f',2));

    ui->m_minV->setText(QString::fromUtf8("%1 V (%2-%3)").arg((float)m->minV/1000,0,'f',3)
                        .arg(m->minVPackID).arg(m->minVID));
    ui->m_maxV->setText(QString::fromUtf8("%1 V (%2-%3)").arg((float)m->maxV/1000,0,'f',3)
                        .arg(m->maxVPackID).arg(m->maxVID));
    ui->m_minT->setText(QString::fromUtf8("%1 ℃ (%2-%3)").arg((float)m->minT/10,0,'f',1)
                        .arg(m->minTPackID).arg(m->minTID));
    ui->m_maxT->setText(QString::fromUtf8("%1 ℃ (%2-%3)").arg((float)m->maxT/10,0,'f',1)
                        .arg(m->maxTPackID).arg(m->maxTID));

    ui->m_rangeV->setText(QString::fromUtf8("%1 mv").arg(abs(m->maxV-m->minV)));
    ui->m_rangeT->setText(QString::fromUtf8("%1 ℃").arg((float)abs(m->maxT-m->minT)/10,0,'f',1));
    ui->m_max_chrg_current->setText(QString::fromUtf8("%1 A").arg(m->chargeCurrentMax));
    ui->m_max_dischrg_current->setText(QString::fromUtf8("%1 A").arg(m->dischargeCurrentMax));

    ui->m_breaker->setText(m->breaker ? "ON" : "OFF");
    ui->m_fusewire->setText(m->fuseWire ? "ON" : "OFF");
    ui->m_buzzer->setText(m->buzzer ? "ON" : "OFF");
    ui->m_selfcheck->setText(m->selfChecking ? "PASSED" : "FAILED");
//    ui->m_totalpositiverelay->setText(m->mainRelay ? "ON" : "OFF");
//    ui->m_totalnegativerelay->setText(m->totalNegativeRelay ? "ON" : "OFF");

    if(m->mainRelay && ui->m_mRelay->text() == "OFF")
    {
        ui->m_mRelay->setText(QString("ON"));
        ui->m_mRelay_btn->setIcon(QIcon(":/image/on.png"));
    }
    else if(!m->mainRelay && ui->m_mRelay->text() == "ON")
    {
        ui->m_mRelay->setText(QString("OFF"));
        ui->m_mRelay_btn->setIcon(QIcon(":/image/off.png"));
    }

    if(m->totalNegativeRelay && ui->m_nRelay->text() == "OFF")
    {
        ui->m_nRelay->setText(QString("ON"));
        ui->m_nRelay_btn->setIcon(QIcon(":/image/on.png"));
    }
    else if(!m->totalNegativeRelay && ui->m_nRelay->text() == "ON")
    {
        ui->m_nRelay->setText(QString("OFF"));
        ui->m_nRelay_btn->setIcon(QIcon(":/image/off.png"));
    }

    if(m->preChargeRelay && ui->m_pRelay->text() == "OFF")
    {
        ui->m_pRelay->setText(QString("ON"));
        ui->m_pRelay_btn->setIcon(QIcon(":/image/on.png"));
    }
    else if(!m->preChargeRelay && ui->m_pRelay->text() == "ON")
    {
        ui->m_pRelay->setText(QString("OFF"));
        ui->m_pRelay_btn->setIcon(QIcon(":/image/off.png"));
    }

    if(m->forceMode && ui->m_fMode->text() == tr("OFF"))
    {
        ui->m_fMode->setText(QString("ON"));
        ui->m_fMode_btn->setIcon(QIcon(":/image/on.png"));
    }
    else if(!m->forceMode && ui->m_fMode->text() == tr("ON"))
    {
        ui->m_fMode->setText(QString("OFF"));
        ui->m_fMode_btn->setIcon(QIcon(":/image/off.png"));
    }

}

void MainWindow::viewPack()
{
    const QColor noneBalanceColor       = Qt::transparent;
    const QColor chargeBalanceColor     = Qt::green;
    const QColor dischargeBalanceColor  = Qt::red;

    QPalette palette;
    Module *m = NULL;
    Pack *p = NULL;

    if(NULL == this->module) {
        return;
    }

    m = this->module + (m_page-1);
    p = m->pack + (p_page-1);

    //voltage temperature
    myModel_v->refresh();
    myModel_t->refresh();

    //fan
    for (int i = 0; i < MAX_PACK_NUM; ++i) {

        QLabel *label = ui->fanGroup->findChild<QLabel *>(QString("fan_%1").arg(i+1));

        if(label != NULL) {
            if(m->fanState & (0x01 << i)) {

                if(label->movie() != movie3) {
                    label->setMovie(movie3);
                    label->movie()->start();
//                    qDebug()<<"set movie pack:"<<i+1;
                }
            }
            else {
                label->setPixmap(QPixmap(":/image/fan_stop.png"));
            }
        }
    }

    //balance
    ui->packID->setText(QString::number(p_page));

    int batID = p->balanceBat ? (p->balanceBat - 0) : 0;
    if(batID > 0)
        ui->batID->setText(QString::number(batID));
    else
        ui->batID->setText("-");

    if(p->balanceState == 0x01)
        ui->balanceState->setText(tr("charging"));
    else if(p->balanceState == 0x02)
        ui->balanceState->setText(tr("discharging"));
    else
        ui->balanceState->setText(tr("standby"));

    for(int i = 0; i < MAX_PACK_NUM; i++)
    {
        QChar mode;
        QLabel *square = ui->balanceGroup->findChild<QLabel *>(QString("balance_%1").arg(i + 1));
        QLabel *num = ui->balanceGroup->findChild<QLabel *>(QString("balance_bat_%1").arg(i + 1));

        if(square)
        {
            switch (m->pack[i].balanceState) {
            case 1:
                palette.setColor(QPalette::Background, chargeBalanceColor);
                break;
            case 2:
                palette.setColor(QPalette::Background, dischargeBalanceColor);
                break;
            default:
                palette.setColor(QPalette::Background, noneBalanceColor);
                break;
            }

            switch (m->pack[i].balanceMode) {
            case 1:
                mode = QChar('A');
                break;
            case 2:
                mode = QChar('M');
                break;
            case 3:
                mode = QChar('T');
                break;
            case 4:
                mode = QChar('S');
                break;
            default:
                mode = QChar('?');
                break;
            }

            square->setPalette(palette);

            if(m->pack[i].balanceState && num)
            {
                num->setText(QString("%1 | %2").arg(m->pack[i].balanceBat).arg(mode));
            }
            else
            {
                num->setText("-");
            }
        }
    }
}

void MainWindow::viewPara()
{
    if (NULL == this->para) {
        return;
    }

    if (!ui->checkBoxFreezeOprtParam->isChecked()) {
        viewPara(ui->stackedWidgetOperationPara, ParamLoader::Operating);
    }

    if (!ui->checkBoxFreezeWarnParam->isChecked()) {
        viewPara(ui->stackedWidgetWarningPara, ParamLoader::Warning);
    }

    if (!ui->checkBoxFreezeClbrParam->isChecked()) {
        viewPara(ui->stackedWidgetCalibrationPara, ParamLoader::Calibration);
    }

    //OCV-SOC
    QStringList list1 = para[para_page-1].OCV.split(",");
    QStringList list2 = para[para_page-1].SOC.split(",");
    ui->OCV->clear();
    ui->SOC->clear();
    for(int i = 0;i < list1.size();i++)
    {
       ui->OCV->appendPlainText(list1.at(i));
    }
    for(int i = 0;i < list1.size();i++)
    {
       ui->SOC->appendPlainText(list2.at(i));
    }
}

/*void MainWindow::viewPara(QGroupBox *box)
{
    QList<QCheckBox*> list = box->findChildren<QCheckBox*>();

    if(NULL == this->para) {
        return;
    }

    foreach(QCheckBox *cb,list)
    {

        QStringList name = cb->objectName().split("_");
        if(name.size() < 2)continue;

        QString flag = name.at(0);
        int id = (int)name.at(1).toInt();

        QString key = config->getStrkey(flag,id);
        QLineEdit *edit = box->findChild<QLineEdit *>(key);

        if(edit != NULL){
            QString *p = getPtr(flag,id,para_page-1);
            if(p != NULL && *p != "")
                edit->setText(*p);
            else
                edit->setText("-");
        }
    }
}*/

void MainWindow::viewPara(QStackedWidget *stackedWidget, ParamLoader::ParamType pType)
{
    bool ok = false;
    bool isUnsigned = true;
    int id = 0;
    int precision = 0;
    int disp = 0;
    quint16 ui16 = 0;
    quint32 ui32 = 0;
    QLineEdit *lineEdit = NULL;
    QList<QCheckBox *>checkBoxes;
    SwitchButton *switchButton = NULL;

    if (NULL == stackedWidget) {
        return;
    }

    checkBoxes = stackedWidget->findChildren<QCheckBox *>();
    foreach (QCheckBox *checkBox, checkBoxes) {
        QString objName = checkBox->objectName();
        id = objName.remove("checkBox").remove(ParamLoader::paramType2Abbr(pType)).toInt(&ok);
        if (ok) {
            objName = checkBox->objectName();
            disp = paramLoader->getDispByType(id, pType);
            lineEdit = NULL;
            switchButton = NULL;
            switch (disp) {
            case ParamLoader::OnlyOne:
                break;
            case ParamLoader::NoDisplay:
            case ParamLoader::Decimal:
            case ParamLoader::Hex:
                lineEdit = stackedWidget->findChild<QLineEdit *>(objName.replace("checkBox", "lineEdit"));
                break;
            case ParamLoader::ZeroAndOne:
            case ParamLoader::MinusOneAndOne:
                switchButton = stackedWidget->findChild<SwitchButton *>(objName.replace("checkBox", "switchButton"));
                break;
            default:
                break;
            }

            if (NULL == lineEdit && NULL == switchButton) {
                continue;
            }

            isUnsigned = paramLoader->getTypeByType(id, pType);
            precision = paramLoader->getPrecisionByType(id, pType);
            if (id < MAX_PARAM_NUM_PER_MODULE) {
                switch (pType) {
                case ParamLoader::Operating:
                    ui16 = operatingParam[para_page - 1][id];
                case ParamLoader::Warning:
                    if (ParamLoader::Warning == pType) {
                        ui16 = warningParam[para_page - 1][id];
                    }

                    if (NULL != lineEdit) {
                        if (ParamLoader::Hex == disp) {
                            lineEdit->setText(BitCalculator::hex2Str(ui16));
                        } else {
                            if (isUnsigned) {
                                lineEdit->setText(QString("%1").arg(ui16 * qPow(0.1, precision), 0, 'f', precision));
                            } else {
                                lineEdit->setText(QString("%1").arg((qint16)ui16 * qPow(0.1, precision), 0, 'f', precision));
                            }
                        }
                    } else if (NULL != switchButton) {
                        switchButton->setChecked(0x01 == ui16);
                    }
                    break;
                case ParamLoader::Calibration:
                    ui32 = calibParam[para_page - 1][id];
                    if (NULL != lineEdit) {
                        if (ParamLoader::Hex == disp) {
                            lineEdit->setText(BitCalculator::hex2Str(ui32));
                        } else {
                            if (isUnsigned) {
                                lineEdit->setText(QString("%1").arg(ui32 * qPow(0.1, precision), 0, 'f', precision));
                            } else {
                                lineEdit->setText(QString("%1").arg((qint32)ui32 * qPow(0.1, precision), 0, 'f', precision));
                            }
                        }

                    } else if (NULL != switchButton) {
                        switchButton->setChecked(0x01 == ui32);
                    }
                    break;
                default:
                    break;
                }
            }
        }
    }
}

void MainWindow::viewLog()
{
    int n = ui->log_num->text().toInt();
    if(n <= 0)
        n = 1;
    else if(n > LOG_MAX_PAGE)
        n = LOG_MAX_PAGE;

    QByteArray ba = logType?log2Text[n-1]:logText[n-1];
    ui->log->clear();
    ui->log->appendPlainText(QString(ba));
}

void MainWindow::viewSysPara()
{
    ui->comboBoxModuleNum->setCurrentIndex(config_others->moduleNum - 1);
    ui->canRate->setCurrentIndex(config_others->canRate.toInt());
    ui->dataSaveInterval->setText(config_others->dataSaveInterval);
    ui->dateTime->setDateTime(QDateTime::currentDateTime());
    ui->checkBoxRealSoc->setChecked(config_others->isRealSocEnabled);

    hmi->getNetWork(ETH0, &net.isDhcp,net.ip,net.mask,net.gateway,net.dns,net.macAddr);
    ui->ip->setText(QString(net.ip));
    ui->mask->setText(QString(net.mask));
    ui->gateway->setText(QString(net.gateway));
    ui->dns->setText(QString(net.dns));
    ui->isDHCP->setCurrentIndex(net.isDhcp?1:0);

    hmi->getNetWork(ETH1, &net_2.isDhcp,net_2.ip,net_2.mask,net_2.gateway,net_2.dns,net_2.macAddr);
    ui->ip_2->setText(QString(net_2.ip));
    ui->mask_2->setText(QString(net_2.mask));
    ui->gateway_2->setText(QString(net_2.gateway));
    ui->dns_2->setText(QString(net_2.dns));
    ui->isDHCP_2->setCurrentIndex(net_2.isDhcp?1:0);

    ui->systemType->setNum(config_others->systemType);

    int level = 0;
    int timeout = 0;
    bool autoClose = false;

#if defined(WEIQIAN)
    level = hmi->getBacklightLevel();
#elif defined(YCTEK)
    hmi->getBacklightState(&level,&autoClose,&timeout);
#endif

    QRadioButton *btn = ui->backlight->findChild<QRadioButton *>(QString("d%1").arg(level));
    if(btn != NULL)btn->setChecked(true);

    ui->autoClose->setChecked(autoClose);
    ui->timeout->setText(QString::number(timeout));

    config_others->pcsDevComPort = config_others->value("pcsDevComPort", 3).toInt();
    config_others->periphComPort = config_others->value("periphComPort", 1).toInt();
    qDebug() << __func__ << config_others->pcsDevComPort << config_others->periphComPort;
    ui->labelPcsDevPort->setText(QString("COM%1").arg(config_others->pcsDevComPort));
    ui->labelPeriphPort->setText(QString("COM%1").arg(config_others->periphComPort));

#ifdef FONT_POINT_SIZE_TEST
    QWidget *w = NULL;
    QGroupBox *b = NULL;
    QList<QObject *> objList = ui->centralWidget->findChildren<QObject *>();

    qDebug() << "font point size increment:" << --font_point_size_inc;
    foreach (QObject *obj, objList) {
        if(obj->inherits("QWidget"))
        {
            w = qobject_cast<QWidget *>(obj);
            QFont font("DejavuSans", w->font().pointSize() - 1);

            w->setFont(font);

            if(0 == qstrcmp(obj->metaObject()->className(), "QGroupBox"))
            {
                b = qobject_cast<QGroupBox *>(obj);

                QList<QObject *> childrenOfBox = b->findChildren<QObject *>();

                foreach (QObject *child, childrenOfBox) {
                    if(child->inherits("QWidget"))
                    {
                        QFont font("DejavuSans", w->font().pointSize() + 1);
                        w->setFont(font);
                    }
                }
            }
        }
    }

    update();
#endif
}


QString MainWindow::getFaultInfo(int code, int flag)
{
    switch (code) {
    case OVER_VOLTAGE:
        return (flag?tr("Cell over voltage"):QString("Cell over voltage"));

    case UNDER_VOLTAGE:
        return (flag?tr("Cell under voltage"):QString("Cell under voltage"));

    case CHARGE_OVER_TEMP:
        return (flag?tr("Cell charging over temperature"):QString("Cell charging over temperature"));

    case CHARGE_UNDER_TEMP:
        return (flag?tr("Cell charging under temperature"):QString("Cell under charging temperature"));

    case MODULE_OVER_CURRENT:
        return (flag?tr("Module over current"):QString("Module over current"));

    case MODULE_INSULATION:
        return (flag?tr("Module insulation"):QString("Module insulation"));

    case MODULE_OVER_VOLTAGE:
        return (flag?tr("Module over voltage"):QString("Module over voltage"));

    case MODULE_UNDER_VOLTAGE:
        return (flag?tr("Module under voltage"):QString("Module under voltage"));

    case RELAY:
        return (flag?tr("Relay fault"):QString("Relay fault"));

    case BMU_ACQUISITION:
        return (flag?tr("BMU acquisiton fault"):QString("BMU acquisiton fault"));

    case BMU_COMM:
        return (flag?tr("BMU communication fault"):QString("BMU communication fault"));

    case PCS_IO:
        return (flag?tr("PCS I/O fault"):QString("PCS I/O fault"));

    case BMU_BALANCE:
        return flag?tr("BMU balance fault"):QString("BMU balance fault");

    case DISCHARGE_OVER_TEMP:
        return flag?tr("Cell discharging over temperature"):QString("Cell discharging over temperature");

    case DISCHARGE_UNDER_TEMP:
        return flag?tr("Cell discharging under temperature"):QString("Cell discharging under temperature");

    default:
        return QString("code error");
    }

}

/*
 * Caller should lock mutex, if there exist fault 'code' in system
 * return 1, else return 0.
*/
int MainWindow::getModuleFaultValueByCode(int module, FAULT_CODE code)
{
    int ret = 0;
    int moduleId = 0;
    int faultCode = 0;
    QMap<int, int>::const_iterator i = fault_bak.lowerBound(module << 16 | code);
    QMap<int, int>::const_iterator upperBound = fault_bak.upperBound(module << 16 | 0xFF << 8 | code);

    while (i != upperBound) {
        moduleId = (i.key() >> 16) & 0xFF;
        faultCode = i.key() & 0xFF;

        if (module == moduleId && faultCode == code) {
            ret = 1;
            break;
        }
        ++i;
    }

    return ret;
}

unsigned char MainWindow::getMdFtByF(unsigned module, unsigned flag)
{
//    QMutexLocker locker(&MapMutex);
    MapMutex.lock();

    bool ret = false;
    QList<int> ids;

    if (0 == (flag & W::Module) && 0 == (flag & W::Cell)) {
        flag |= W::Module;
    }

    if (0 != (flag & W::Charging) && 0 != (flag & W::Discharging)) {
        MapMutex.unlock();
        ret = 0 < (getMdFtByF(module, flag & (~W::Charging)) + getMdFtByF(module, flag & (~W::Discharging))) ? 1 : 0;
        return ret;
    }

    ids.clear();
    ids = paramLoader->getIdListByFlag(flag);
    if (ids.isEmpty()) {
        ret = false;
    } else if (1 < ids.size()) {
        qSort(ids.begin(), ids.end());
        QMap<int, int>::const_iterator i = wnMarks.lowerBound(ids.first());
        QMap<int, int>::const_iterator upperBound = wnMarks.upperBound(ids.last());
        while (i != upperBound) {
//            cout << i.value() << endl;
            if ((int)module == i.value()) {
                ret = true;
                break;
            }
            ++i;
        }
    } else {
        ret = wnMarks.values(ids.first()).contains(module);
    }

    MapMutex.unlock();
    return ret ? 0x01 : 0x00;
}

bool MainWindow::isMdFt(unsigned module)
{
    QMutexLocker locker(&MapMutex);

    bool ret = false;
    QList<int> values = wnMarks.keys(module);

    foreach (int val, values) {
        if (1 == paramLoader->getWnLevelById(val)) {
            ret = true;
            break;
        }
    }

    return ret;
}

bool MainWindow::isMdFt(unsigned module, QList<int> filter)
{
    return isMdFtFilter(module, filter, true);
}

bool MainWindow::isMdFtExcept(unsigned module, QList<int> filter)
{
    return isMdFtFilter(module, filter, false);
}

bool MainWindow::isMdFtFilter(unsigned module, QList<int> filter, bool containFilter)
{
    QMutexLocker locker(&MapMutex);

    bool ret = false;
    QList<int> idFilter;
    QList<int> values = wnMarks.keys(module);

    idFilter.clear();
    foreach (int i, filter) {
        idFilter << paramLoader->getIdListByFlag(i);
    }

    foreach (int val, values) {
        if (containFilter == idFilter.contains(val)
                && 1 == paramLoader->getWnLevelById(val)) {
            ret = true;
            break;
        }
    }

    return ret;
}

bool MainWindow::isMdMainRelayOff(unsigned module)
{
    return 0 == this->module[module].mainRelay;
}

QString MainWindow::getWarningInfo(int code,int flag)
{

    switch(code)
    {
    case OVER_VOLTAGE_2:
        return (flag?tr("Cell over voltage level 2"):QString("Cell over voltage level 2"));

    case OVER_VOLTAGE_3:
        return (flag?tr("Cell over voltage level 3"):QString("Cell over voltage level 3"));

    case UNDER_VOLTAGE_2:
        return (flag?tr("Cell under voltage level 2"):QString("Cell under voltage level 2"));

    case UNDER_VOLTAGE_3:
        return (flag?tr("Cell under voltage level 3"):QString("Cell under voltage level 3"));

    case OVER_TEMP_2:
        return (flag?tr("Cell over temperature level 2"):QString("Cell over temperature level 2"));

    case OVER_TEMP_3:
        return (flag?tr("Cell over temperature level 3"):QString("Cell over temperature level 3"));

    case UNDER_TEMP_2:
        return (flag?tr("Cell under temperature level 2"):QString("Cell under temperature level 2"));

    case UNDER_TEMP_3:
        return (flag?tr("Cell under temperature level 3"):QString("Cell under temperature level 3"));

    case MODULE_OVER_CURRENT_2:
        return (flag?tr("Module over current level 2"):QString("Module over current level 2"));

    case MODULE_OVER_CURRENT_3:
        return (flag?tr("Module over current level 3"):QString("Module over current level 3"));

    case MODULE_INSULATION_2:
        return (flag?tr("Module insulation level 2"):QString("Module insulation level 2"));

    case MODULE_INSULATION_3:
        return (flag?tr("Module insulation level 3"):QString("Module insulation level 3"));

    case MODULE_OVER_VOLTAGE_2:
        return (flag?tr("Module over voltage level 2"):QString("Module over voltage level 2"));

    case MODULE_OVER_VOLTAGE_3:
        return (flag?tr("Module over voltage level 3"):QString("Module over voltage level 3"));

    case MODULE_UNDER_VOLTAGE_2:
        return (flag?tr("Module under voltage level 2"):QString("Module under voltage level 2"));

    case MODULE_UNDER_VOLTAGE_3:
        return (flag?tr("Module under voltage level 3"):QString("Module under voltage level 3"));

    case VOLTAGE_CONSISTENCY_2:
        return (flag?tr("voltage consistency warning level 2"):QString("voltage consistency warning level 2"));

    case VOLTAGE_CONSISTENCY_3:
        return (flag?tr("voltage consistency warning level 3"):QString("voltage consistency warning level 3"));

    case TEMP_CONSISTENCY_2:
        return (flag?tr("temp consistency warning level 2"):QString("temp consistency warning level 2"));

    case TEMP_CONSISTENCY_3:
        return (flag?tr("temp consistency warning level 3"):QString("temp consistency warning level 3"));

    case SOH:
        return (flag?tr("SOH warning"):QString("SOH warning"));

    case SOC_HIGH_1:
        return (flag?tr("SOC high warning level 1"):QString("SOC high warning level 1"));

    case SOC_HIGH_2:
        return (flag?tr("SOC high warning level 2"):QString("SOC high warning level 2"));

    case SOC_HIGH_3:
        return (flag?tr("SOC high warning level 3"):QString("SOC high warning level 3"));

    case SOC_LOW_1:
        return (flag?tr("SOC low warning level 1"):QString("SOC low warning level 1"));

    case SOC_LOW_2:
        return (flag?tr("SOC low warning level 2"):QString("SOC low warning level 2"));

    case SOC_LOW_3:
        return (flag?tr("SOC low warning level 3"):QString("SOC low warning level 3"));

    default:
        return QString("code error");
    }

}

void MainWindow::faultReport()
{
    QMutexLocker locker(&MapMutex);

    uint faultModule = 0;
    uint faultLevel = 0;
    uint faultCode = 0;
    uint row = 0;
    QString faultInfo;

    warningBit = 0;
    faultBit = 0;
    row = ui->table->rowCount();
    while (row--) {
        ui->table->removeRow(0);
    }

    if (0 < wnMarks.size()) {
//        qDebug() << "***" << wnMarks;
        for (QMap<int, int>::iterator i = wnMarks.begin(); i != wnMarks.end(); ++i) {
            row = ui->table->rowCount();
            faultModule = i.value();
            faultLevel = paramLoader->getWnLevelById(i.key());
            faultCode = (faultModule << 16) | (i.key() << 8) | faultLevel;
            faultInfo = getWarningDesc(i.key());

            if (faultInfo.contains("undefined id")) {
                continue;
            }

            if (0 < faultModule && MODULE_NUM > faultModule - 1) {
                if (1 == faultLevel) {
                    faultBit |= 0x01 << (faultModule - 1);
                } else {
                    warningBit |= 0x01 << (faultModule - 1);
                }
            }

            ui->table->insertRow(row);
            ui->table->setItem(row, 0, new QTableWidgetItem(1 == faultLevel
                                                            ? QIcon(QPixmap(":/image/error.png"))
                                                            : QIcon(QPixmap(":/image/warning.png")),
                                                            QString("%1").arg(faultCode, 6, 16, QChar('0')).toUpper().prepend("0x")));
            ui->table->setItem(row,1,new QTableWidgetItem(QString("%1").arg(i.value())));
            ui->table->setItem(row,2,new QTableWidgetItem(faultInfo));
        }
    }
    ui->table->sortItems(0);

    /*if(fault.size() > 0)
    {
        for (i = fault.begin(); i != fault.end(); ++i)
        {
            //insert table
            row = ui->table->rowCount();
            ui->table->insertRow(row);
            ui->table->setItem(row,0,new QTableWidgetItem(QIcon(QPixmap(":/image/error.png")),
                                                          QString("0x") + QString("%1").arg(i.key(),6,16,QChar('0')).toUpper()));

            ui->table->setItem(row,1,new QTableWidgetItem(QString("%1(%2-%3)").arg((i.key() >> 16) & 0xFF).
                                                          arg((i.key() >> 8) & 0xFF).arg(i.value())));

            ui->table->setItem(row,2,new QTableWidgetItem(getFaultInfo(i.key() & 0xFF,1)));

            //log
            if(fault_bak.find(i.key()) == fault_bak.end() || fault_bak.find(i.key()).value() != i.value())
            {
                info = getFaultInfo(i.key() & 0xFF);
                qWarning("Fault: 0x%06X, Value:%d, Info: %s",i.key(),i.value(),info.toLatin1().data());
            }

            //write bit
            faultBit |= (0x01 << ((i.key() >> 16)-1));
        }
    }*/

    fault_bak = fault;
    fault.clear();

    /*if(warning.size() > 0)
    {
        for (i = warning.begin(); i != warning.end(); ++i)
        {
            //insert table
            row = ui->table->rowCount();
            ui->table->insertRow(row);
            ui->table->setItem(row,0,new QTableWidgetItem(QIcon(QPixmap(":/image/warning.png")),
                                                          QString("0x") + QString("%1").arg(i.key(),6,16,QChar('0')).toUpper()));

            ui->table->setItem(row,1,new QTableWidgetItem(QString("%1(%2-%3)").arg((i.key() >> 16) & 0xFF).
                                                          arg((i.key() >> 8) & 0xFF).arg(i.value())));

            ui->table->setItem(row,2,new QTableWidgetItem(getWarningInfo(i.key() & 0xFF,1)));

            //log
            if(warning_bak.find(i.key()) == warning_bak.end() || warning_bak.find(i.key()).value() != i.value())
            {
                info = getWarningInfo(i.key() & 0xFF);
                qWarning("Warning: 0x%06X, Value: %d, Info: %s",i.key(),i.value(),info.toLatin1().data());
            }

            //write bit
            warningBit |= (0x01 << ((i.key() >> 16)-1));
        }
    }

    warning_bak = warning;*/
}

void MainWindow::muteClicked()
{
    mute = !mute;
    ui->mute->setIcon(mute ? QIcon(":/image/mute.png") : QIcon(":/image/speaker.png"));
    wnMarksMute = wnMarks;
}

void MainWindow::sendRelay()
{
    int i = 0;
    int dest = ui->global_relay->isChecked()?0:m_page;
    can_frame frame;
    memset(frame.data,0,sizeof(frame.data));

    frame.data[0] = 0xA0;

    QMessageBox *box = new QMessageBox(QMessageBox::Question," "," ");
    QAbstractButton *openBtn = box->addButton(tr("Open"),QMessageBox::ActionRole);
    QAbstractButton *closeBtn = box->addButton(tr("Close"),QMessageBox::ActionRole);
    box->addButton(tr("Cancel"),QMessageBox::RejectRole);

    if(sender()->objectName() == "m_mRelay_btn")
    {
        i = 2;
        frame.data[1] = module[m_page-1].forceMode;
        frame.data[3] = module[m_page-1].totalNegativeRelay;
        frame.data[4] = module[m_page-1].preChargeRelay;
        box->setText(tr("Operate Total Positive Relay?"));
    }
    else if(sender()->objectName() == "m_nRelay_btn")
    {
        i = 3;
        frame.data[1] = module[m_page-1].forceMode;
        frame.data[2] = module[m_page-1].mainRelay;
        frame.data[4] = module[m_page-1].preChargeRelay;
        box->setText(tr("Operate Total Negative Relay?"));
    }
    else if(sender()->objectName() == "m_pRelay_btn")
    {
        i = 4;
        frame.data[1] = module[m_page-1].forceMode;
        frame.data[2] = module[m_page-1].mainRelay;
        frame.data[3] = module[m_page-1].totalNegativeRelay;
        box->setText(tr("Operate Precharge Relay?"));
    }
    else if(sender()->objectName() == "m_fMode_btn")
    {
        i = 1;
        frame.data[2] = module[m_page-1].mainRelay;
        frame.data[3] = module[m_page-1].totalNegativeRelay;
        frame.data[4] = module[m_page-1].preChargeRelay;
        box->setText(tr("Change forceMode?"));
    }

    box->exec();

    if(box->clickedButton() == openBtn)
    {
        frame.data[i] = 0x01;
    }
    else if(box->clickedButton() == closeBtn)
    {
        frame.data[i] = 0x00;
    }
    else {
        delete box;
        return;
    }

    frame.can_id = (CAN::BAMS_RELAY_INSTRUCTION << 4) | dest;
    frame.can_dlc = 8;

    hmi->WriteCan(&frame);

    showMsg("Send success!");
    delete box;
}

void MainWindow::sendCommand()
{
    bool ok = false;
    int dest = ui->global->isChecked() ? 0 : para_page;
    uint index = 0;
    uint offset = 0;
    QString objName;
    QStringList stringList;
    can_frame cmdFrame;

    if (NULL == sender()) {
        return;
    }

    objName = sender()->objectName();
    stringList = objName.remove("pushButton").remove("Clbr").split('_');
    if (1 < stringList.size()) {
        index = stringList.at(0).toUInt(&ok);
        if (ok) {
            offset = stringList.at(1).toUInt(&ok);
            if (ok && CHAR_BIT > offset) {
                qMemSet(&cmdFrame, 0x00, sizeof(cmdFrame));
                cmdFrame.can_id = (CAN::BAMS_SEND_CALIBRATION_PARA << 4) | dest;
                cmdFrame.can_dlc = 8;
                cmdFrame.data[0] = 0x70;
                cmdFrame.data[1] = (quint8)index;
                cmdFrame.data[5] = 0x01 << offset;
                hmi->WriteCan(&cmdFrame);
            }
        }
    }
}

void MainWindow::startBalance()
{
    can_frame frame;
    memset(frame.data,0,sizeof(frame.data));

    bool ok;
    int16_t time = (int16_t)ui->balanceTime->text().toInt(&ok, 10);
    if(!ok)
    {
        ui->balanceTime->setFocus();
        ui->balanceTime->setStyleSheet("background-color:rgb(255, 72, 17)");
        QMessageBox::information(this,QString("Info"),QString("Incorrect input!"));
        return;
    }
    else
    {
        ui->balanceTime->setStyleSheet("");
    }

    p_page = ui->balancePackID->currentIndex() + 1;

    frame.data[0] = 0x80;
    frame.data[1] = p_page;
    frame.data[2] = ui->balanceMode->currentIndex();
    frame.data[3] = ui->balanceID->currentIndex() + 1;
    frame.data[4] = ui->balanceDir->currentIndex() ? 0x02 : 0x01;
    frame.data[5] = time >> 8;
    frame.data[6] = time;

    frame.can_id = (CAN::BAMS_BALANCE_INSTRUCTION_ << 4) | m_page;
    frame.can_dlc = 8;

    hmi->WriteCan(&frame);

    showMsg("Send success!");
}

void MainWindow::sendPara()
{
    bool ok = false;
    int num = 0;
    int index = 0;
    int dest = ui->global->isChecked() ? 0 : para_page;
    int precision = 0;
    int disp = 1;
    qreal value = 0;
    ParamLoader::ParamType pType = ParamLoader::NoType;
    can_frame canFrames[MAX_PARAM_NUM_PER_MODULE];
    QList<QCheckBox*> list;
    QLineEdit *lineEdit = NULL;
    QWidget *container = NULL;
    SwitchButton *switchButton = NULL;

    memset(canFrames, 0x00, MAX_PARAM_NUM_PER_MODULE * sizeof(can_frame));
    if (sender()->objectName().contains(ParamLoader::paramType2Abbr(ParamLoader::Operating))) {
        pType = ParamLoader::Operating;
        container = qobject_cast<QWidget *>(ui->stackedWidgetOperationPara);
    } else if (sender()->objectName().contains(ParamLoader::paramType2Abbr(ParamLoader::Warning))) {
        pType = ParamLoader::Warning;
        container = qobject_cast<QWidget *>(ui->stackedWidgetWarningPara);
    } else if (sender()->objectName().contains(ParamLoader::paramType2Abbr(ParamLoader::Calibration))) {
        pType = ParamLoader::Calibration;
        container = qobject_cast<QWidget *>(ui->stackedWidgetCalibrationPara);
    } else if (sender()->objectName().contains("c_send1")) {
        pType = ParamLoader::Calibration;
        container = qobject_cast<QWidget *>(ui->calibration);
    }

    if (NULL == container) {
        return;
    }

    list = container->findChildren<QCheckBox *>();
    foreach(QCheckBox *cb,list)
    {
        if(cb->isChecked())
        {
            QString objName = cb->objectName();
            QString id = objName.remove("checkBox").remove(ParamLoader::paramType2Abbr(pType));
            int i = id.toInt(&ok);
            if (!ok) {
                continue;
            }

            precision = paramLoader->getPrecisionByType(i, pType);
            disp = paramLoader->getDispByType(i, pType);
            switch (disp) {
            case ParamLoader::NoDisplay:
            case ParamLoader::Decimal:
            case ParamLoader::Hex:
                objName = cb->objectName();
                lineEdit = container->findChild<QLineEdit *>(objName.replace("checkBox", "lineEdit"));
                if (lineEdit != NULL) {
                    if (ParamLoader::Hex > disp) {
                        value = lineEdit->text().toDouble(&ok);
                    } else {
                        value = lineEdit->text().toUInt(&ok, 16);
                    }

                    if (!ok) {
                        lineEdit->setStyleSheet("background-color:rgb(255, 72, 17)");
                        continue;
                    } else {
                        lineEdit->setFocus();
                        lineEdit->setStyleSheet("");
                    }

                    qDebug() << value << QString::number(qRound(value), 16) << dest << i << precision << pType;
                    COMM::Encode(canFrames[index++], value, dest, i, precision, pType);
                }
                break;
            case ParamLoader::ZeroAndOne:
            case ParamLoader::MinusOneAndOne:
                objName = cb->objectName();
                switchButton = container->findChild<SwitchButton *>(objName.replace("checkBox", "switchButton"));
                if (switchButton != NULL) {
                    if (switchButton->getChecked()) {
                        value = 1;
                    } else if (ParamLoader::ZeroAndOne == disp) {
                        value = 0;
                    } else if (ParamLoader::MinusOneAndOne == disp) {
                        value = -1;
                    }

                    qDebug() << value << dest << i << precision << pType;
                    COMM::Encode(canFrames[index++], value, dest, i, precision, pType);
                }
                break;
            case ParamLoader::OnlyOne:
                value = 1;
                COMM::Encode(canFrames[index++], value, dest, i, precision, pType);
                break;
            default:
                break;
            }
        } else {
            num++;
        }
    }

    if (num == list.size())
    {
        QMessageBox::information(this,QString("info"),QString("No selected parameter!"));
        return;
    }

    if (index) {
        paraACK(0,1);
        sendFlag = paramLoader->paramType2Char(pType);
        for (int i = 0; i < index; ++i) {
            hmi->WriteCan(canFrames+i);
            usleep(1000);
        }

        showMsg("Send success!");
    }
}

/*void MainWindow::savePara()
{
    QList<QCheckBox*> list;
    QGroupBox *box = NULL;
    QStackedWidget *stkWidget = ui->stackedWidgetWarningPara;
    QPushButton *btn = qobject_cast<QPushButton *>(sender());

    if(0 == stkWidget->currentIndex())
    {
        box = ui->w_para_1;
    }
    else if(1 == stkWidget->currentIndex()) {
        box = ui->w_para_2;
    }

    if(NULL == box)
    {
        return;
    }

    if(btn->objectName().contains("w_save"))
    {
        list = box->findChildren<QCheckBox*>();

        foreach(QCheckBox *cb,list)
        {
            QStringList name = cb->objectName().split("_");
            if(name.size() < 2)continue;

            QString flag = name.at(0);
            int id = (int)name.at(1).toInt();

            modifyPara(flag,id,box);
        }
        config->sync();
    }

    QMessageBox::information(this,"info","Save done!");
}*/

void MainWindow::readPara(QString flag,int dest)
{
    can_frame frame;
    memset(frame.data,0,sizeof(frame.data));

    if(flag == "m"){
        frame.data[0] = 0x80;
    }
    else if(flag == "f"){
        frame.data[0] = 0x90;
    }
    else if(flag == "c"){
       frame.data[0] = 0x70;
    }
    else
        return;

    frame.can_id = (CAN::BAMS_READ_PARA << 4) | dest;
    frame.can_dlc = 1;

    hmi->WriteCan(&frame);
}

/*QString MainWindow::modifyPara(QString flag,int id,QGroupBox *box)
{
    QString key = config->getStrkey(flag,id);
    QLineEdit *edit = box->findChild<QLineEdit *>(key);

    if(edit != NULL){
        //validate
        if(!dataValidate(edit))return "null";

        //global set
        if(ui->global->isChecked()){
            //modify memory
            for(int i = 0;i < MODULE_NUM;i++)
            {
                QString *p = getPtr(flag,id,i);
                if(p != NULL)*p = edit->text();
            }
            //modify config
            config->beginWriteArray("module");
            for(int i = 0;i < MODULE_NUM;i++)
            {
                config->setArrayIndex(i);
                config->setValue(key,edit->text());
            }
            config->endArray();
        }else{
            //modify memory
            QString *p = getPtr(flag,id,para_page-1);
            if(p != NULL)*p = edit->text();
            //modify config
            config->beginWriteArray("module");
            config->setArrayIndex(para_page-1);
            config->setValue(key,edit->text());
            config->endArray();
        }

        return edit->text();
    }else
        return "null";
}*/

void MainWindow::checkPara()
{
    QMutexLocker locker(&HeartMutex);

    QString flag;
    QStackedWidget *stackedWidget = NULL;

    if (0 == ui->stackedWidgetBodyPara->currentIndex()) {
        flag = "m";
        stackedWidget = ui->stackedWidgetOperationPara;
    } else if (1 == ui->stackedWidgetBodyPara->currentIndex()) {
        flag = "f";
        stackedWidget = ui->stackedWidgetWarningPara;
    } else if (2 == ui->stackedWidgetBodyPara->currentIndex()) {
        flag = "c";
        stackedWidget = ui->stackedWidgetCalibrationPara;
    }

    if (stackedWidget == NULL) {
        return;
    }

    if (module[para_page-1].heartbeat_bak) {
        initPara(flag, para_page);
    }
//    QList<QLineEdit*> lineEdits = stackedWidget->findChildren<QLineEdit *>();
//    foreach (QLineEdit *lineEdit, lineEdits) {
//    break;
//    }
}

void MainWindow::paraACK(int id, int flag)
{
    if (0 < id && 1 > id - MODULE_NUM) {        //response
        QLabel *obj = labelAck[id - 1];
        if (obj != NULL) {
            obj->setPixmap(pixmapChecked);
        }

        initPara(sendFlag,sendPage);
    } else {
        if (flag == 0) {                        //check
            bool sendDone = true;
            if (sendGlobal) {
                for (uint i = 0; i < MODULE_NUM; ++i) {
                    if (labelAck[i]->pixmap()->cacheKey() != pixmapChecked.cacheKey()) {
                        sendDone = false;
                    }
                }
            }
            else {
                if(labelAck[sendPage - 1]->pixmap()->cacheKey() != pixmapChecked.cacheKey()) {
                    sendDone = false;
                }
            }
            if (sendDone) {
                sendTime = 0;
            }
        } else if (flag == 1) {                 //send
            sendTime = 1;

            sendGlobal = ui->global->isChecked();
            sendPage = para_page;

            for (uint i = 0; i < MODULE_NUM; ++i) {
                if (ui->global->isChecked()) {
                    labelAck[i]->setPixmap(pixmapDownload);
                } else {
                    labelAck[i]->setPixmap(i == para_page - 1 ? pixmapDownload : pixmapUnchecked);
                }
            }
        } else if (flag == 2) {                 //timeout
            sendTime = 0;

            for (uint i = 0; i < MODULE_NUM; ++i) {
                if (labelAck[i]->pixmap()->cacheKey() == pixmapDownload.cacheKey()) {
                    labelAck[i]->setPixmap(pixmapError);
                }
            }
        } else if(flag == -1) {                 //revert
            sendTime = 0;

            for (uint i = 0; i < MODULE_NUM; ++i) {
                labelAck[i]->setPixmap(pixmapUnchecked);
            }
        }
    }
}

void MainWindow::saveSysPara()
{
    QPushButton *btn = static_cast<QPushButton *>(sender());

    if (btn->objectName() == "apply1") {
        config_others->setValue("moduleNum", ui->comboBoxModuleNum->currentIndex() + 1);
        config_others->setValue("canRate",ui->canRate->currentIndex());
        config_others->setValue("dataSaveInterval",ui->dataSaveInterval->text());
        config_others->setValue("isRealSocEnabled", ui->checkBoxRealSoc->isChecked());
        config_others->sync();

        const char* cmd = qPrintable(QString("date -s \"%1\"").arg(ui->dateTime->dateTime().toString("yyyy-MM-dd hh:mm:ss")));
        system(cmd);
        system("hwclock -w");
    } else if (btn->objectName() == "apply2") {
        memcpy(net.ip,ui->ip->text().toLatin1().data(),20);
        memcpy(net.mask,ui->mask->text().toLatin1().data(),20);
        memcpy(net.gateway,ui->gateway->text().toLatin1().data(),20);
        memcpy(net.dns,ui->dns->text().toLatin1().data(),20);

        net.isDhcp = (ui->isDHCP->currentIndex() == 0)?false:true;
        hmi->setNetWork(ETH0, net.isDhcp,net.ip,net.mask,net.gateway,net.dns);
    } else if (btn->objectName() == "apply2_2") {
        memcpy(net_2.ip,ui->ip_2->text().toLatin1().data(),20);
        memcpy(net_2.mask,ui->mask_2->text().toLatin1().data(),20);
        memcpy(net_2.gateway,ui->gateway_2->text().toLatin1().data(),20);
        memcpy(net_2.dns,ui->dns_2->text().toLatin1().data(),20);

        net_2.isDhcp = (ui->isDHCP_2->currentIndex() == 0)?false:true;
        hmi->setNetWork(ETH1, net_2.isDhcp,net_2.ip,net_2.mask,net_2.gateway,net_2.dns);
    } else if (btn->objectName() == "apply3") {
        hmi->setBacklightAutoClose(ui->autoClose->isChecked(),ui->timeout->text().toInt());
    } else if (btn->objectName() == "apply4") {
        config_others->setValue("systemType", ui->systemType->text().toInt());
        config_others->sync();
    }

    showMsg(QString("Apply success!"),3000);
}

void MainWindow::changeBacklight()
{
    int level = 0;

    if (ui->d0->isChecked()) {
        level = 1;
    } else if (ui->d1->isChecked()) {
        level = 2;
    } else if (ui->d2->isChecked()) {
        level = 3;
    } else {
        level = 4;
    }

    hmi->setBacklightLevel(level);
    config_others->setValue("backlightLevel", level);
    config_others->sync();
}

quint64 MainWindow::checkDisk()
{
    qreal diskUsage = 0.0;                      // range: [0, 1]
    quint64 blockSize = 0;
    quint64 freeSpace = -1;                     // uint: bytes
    quint64 usedSpace = 0;
    quint64 totalSpace = 0;
    struct statfs info;
    bool detected = externalStorageDetect();

    if (!extDiskUsingFlag && detected) {
        if (0 == statfs(storagePath.toLatin1().data(),&info)) {
            blockSize = info.f_bsize;
            totalSpace = info.f_blocks * blockSize;
            freeSpace = info.f_bfree * blockSize;
            usedSpace = totalSpace - freeSpace;
            if (0 != totalSpace) {
                diskUsage = usedSpace / (qreal)totalSpace;
            }

            ui->diskInfo->setText(QString("Available"));
            ui->progressBar1->setValue(qRound(diskUsage * ui->progressBar1->maximum()));
            ui->labelUsage->setText(improveReadability(usedSpace) + " / " + improveReadability(totalSpace));

            if (0.85 < diskUsage) {
                ui->progressBar1->setStyleSheet(QString("background-color: rgb(255, 68, 11)"));
            } else {
                ui->progressBar1->setStyleSheet(QString("background-color: rgb(74, 255, 42)"));
            }
        } else {
            ui->labelUsage->setText("statfs error");
        }
    }

    ui->progressBar1->setVisible(detected);
    ui->labelUsage->setVisible(detected);

    return freeSpace;
}

void MainWindow::updateCopyProgress(double percent)
{
    ui->progressBarData->setValue(percent * 100);
}

void MainWindow::copyFinishSlot()
{
    ui->progressBarData->setValue(100);
}

void MainWindow::errorHandleSlot()
{
    qDebug() << __func__;
    QMessageBox::information(this, "Error", tr("Abort."));
}

void MainWindow::processData()
{
#if 0
    struct statfs data;
    QString cmd;

    int free = checkDisk();
    if(free == -1)
    {
        QMessageBox::information(this,"Info",QString("No storage medium!"));
        return;
    }

    if(storagePath == USB_PATH)
    {
        statfs(SD_PATH,&data);
        if((uint)free < ((data.f_bfree*data.f_bsize)>>20))
        {
            ui->diskInfo->setText(QString("Not enough storage!"));
            return;
        }

        if(QMessageBox::Ok == QMessageBox::question(this,"Copy","Copy data?",QMessageBox::Ok|QMessageBox::Cancel))
        {
            QDir d(storagePath + QString("/data"));
            if(!d.exists())d.mkpath(d.absolutePath());

            cmd = QString("\\cp -rf ") + QString(SD_PATH) + QString("/* ") + d.absolutePath();
            ui->diskInfo->setText(QString("Copying...!"));
            emit copyData_Signal(cmd);
        }
    }
    else if(storagePath == SD_PATH)
    {
        if(QMessageBox::Ok == QMessageBox::question(this,"Delete","Delete data?",QMessageBox::Ok|QMessageBox::Cancel))
        {
            cmd = QString("rm -rf ") + QString(SD_PATH) + QString("/*");
            ui->diskInfo->setText(QString("Deleting...!"));
            emit copyData_Signal(cmd);
        }
    }
#else
//    storagePath = "/mnt/userdata1";                 // only for debug
    // TO-DO: check if there is enough space, check folder name, disk is about full during saving, show these msg in ui
    quint64 freeBytes = checkDisk();
    quint64 dataBytes = getFilesInDirSize(APP_DATA_DIRPATH, QStringList("*.csv"));
    QDir dir(APP_DATA_DIRPATH);

    if (!externalStorageDetect()) {
        QMessageBox::information(this, "Info", tr("No selected storage medium found."));
        return;
    }

    qDebug() << "data:" << dataBytes << "disk free:" << freeBytes;
    if (dataBytes > freeBytes) {
        QMessageBox::information(this, "Not enough sapce", tr("Please reserve at least %1 on your disk.")
                                 .arg(improveReadability(dataBytes)));
        return;
    }

    nCopied = 0;
    nTotal = getDataFileNum(APP_DATA_DIRPATH);
    dirQueue.clear();
    foreach (QFileInfo info, dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        qDebug() << __func__ << "111" << info.fileName() << isDataDirNameValid(info.fileName());
        if (isDataDirNameValid(info.fileName())) {
            dirQueue.enqueue(info);
        }
    }

    if (dirQueue.isEmpty()) {
        qDebug() << __func__ << "nothing to copy.";
        return;
    }

    qDebug() << __func__ << "before cd" << dir.absolutePath() << dirQueue.head().fileName();
    if (!dir.cd(dirQueue.head().fileName())) {
        qDebug() << __func__ << "cd failed:" << dir.absolutePath() << dirQueue.head().fileName();
        return;
    }

    fileQueue.clear();
    foreach (QFileInfo info, dir.entryInfoList(QDir::Files)) {
        fileQueue.enqueue(info);
        qDebug() << __func__ << "222" << info.fileName();
    }

    // TO-DO: calculate how many files recursively and update total task progress.
//    doDataDirCopy();
    QtConcurrent::run(this, &MainWindow::doDataDirCopy);
#endif
}

void MainWindow::processDone()
{
    ui->diskInfo->setText(QString("Process done!"));
}

void MainWindow::openFile()
{
    QString path;
    QString filter;

    if (0 == checkDisk() + 1) {
        QMessageBox::information(this, "Info", tr("No storage medium found."));
        return;
    }

    switch (ui->device->currentIndex()) {
    case 0:
        filter = "hmi (*BMS *.qm *.cfg)";
        break;
    case 1:
        filter = "mcu (*.bin)";
        break;
    case 2:
        filter = "slave (*.bin)";
        break;
    default:
        filter = "unknown (*)";
        break;
    }

    updateFiles.clear();
    updateFiles = QFileDialog::getOpenFileNames(this, "File", storagePath, filter);
    if(updateFiles.size() == 0)return;

    for (int i = 0; i < updateFiles.size(); ++i)
    {
        path += updateFiles.at(i);
        path += ";";
    }

    ui->filepath->setText(path);
}

void MainWindow::do_update()
{
    QFileInfo file;

    if(updateFiles.size() == 0)
    {
        QMessageBox::information(this,"Info",QString("No files!"));
        return;
    }

    progressChanged(0);
    ui->device->setEnabled(false);
    ui->update->setEnabled(false);

    if(ui->device->currentIndex() == 0)
    {
        for (int i = 0; i < updateFiles.size(); ++i)
        {
            file = QFileInfo(updateFiles.at(i));

            if(appDir.exists(file.fileName()))
            {
                appDir.remove(file.fileName());
                //appDir.rename(file.fileName(),file.fileName()+"_"+QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
            }

            if (appDir.exists(file.fileName() + ".upd")) {
                QFile::remove(qApp->applicationDirPath() + "/" + file.fileName() + ".upd");
            }

#ifdef WEIQIAN
            if (QFile::copy(updateFiles.at(i), qApp->applicationDirPath() + "/" + file.fileName() + ".upd"))
#else
            if (QFile::copy(updateFiles.at(i), appDir.absolutePath() + "/" + file.fileName()))
#endif
            {
                progressChanged((i+1)*100/updateFiles.size());
            }
            else
            {
                progressChanged(-1);
                return;
            }
        }

#ifndef WEIQIAN
        system(qPrintable(QString("chmod +x %1").arg(qApp->applicationFilePath())));
#else
//        system(qPrintable(QString("%1/%2/update.sh %3").arg(qApp->applicationDirPath()).arg(SCRIPT_DIR_NAME).arg(qAppName())));
#endif

        this->close();
        qApp->exit();
    }
    else if(ui->device->currentIndex() == 1)
    {
        emit updateMCU_Signal(updateFiles.at(0), 0);
    }
    else if(ui->device->currentIndex() == 2)
    {
        emit updateMCU_Signal(updateFiles.at(0), 1);
    }

}

void MainWindow::updatingMCU(int progress)
{
    if(progress == 0)
        mcu_updating = true;
    else if((progress == -1) || (progress == 100))
        mcu_updating = false;

    progressChanged(progress);
}

void MainWindow::progressChanged(int progress)
{
    if(progress < 0)
    {
        ui->progressBar2->setValue(0);
        QMessageBox::information(this,"Info",tr("Update failed!"));
        ui->device->setEnabled(true);
        ui->update->setEnabled(true);

        return;
    }
    else if(progress <= ui->progressBar2->maximum())
    {
        ui->progressBar2->setValue(progress);

        if(progress == ui->progressBar2->maximum())
        {
            QMessageBox::information(this,"Info",tr("Update success!"));
            ui->device->setEnabled(true);
            ui->update->setEnabled(true);

            qCritical()<<"update "<< ui->device->currentText()<<" success!";
        }
    }

}

void MainWindow::reboot()
{
#ifdef FONT_POINT_SIZE_TEST
    QWidget *w = NULL;
    QGroupBox *b = NULL;
    QList<QObject *> objList = ui->centralWidget->findChildren<QObject *>();

    qDebug() << "font point size increment:" << ++font_point_size_inc;
    foreach (QObject *obj, objList) {
        if(obj->inherits("QWidget"))
        {
            w = qobject_cast<QWidget *>(obj);
            QFont font("DejavuSans", w->font().pointSize() + 1);

            w->setFont(font);

            if(0 == qstrcmp(obj->metaObject()->className(), "QGroupBox"))
            {
                b = qobject_cast<QGroupBox *>(obj);

                QList<QObject *> childrenOfBox = b->findChildren<QObject *>();

                foreach (QObject *child, childrenOfBox) {
                    if(child->inherits("QWidget"))
                    {
                        QFont font("DejavuSans", w->font().pointSize() + 1);
                        w->setFont(font);
                    }
                }
            }
        }
    }

    update();
#else
    if (QMessageBox::Ok == QMessageBox::question(this,"Reboot","Reboot now?",QMessageBox::Ok|QMessageBox::Cancel)) {
        this->close();
        qApp->exit();
//        qDebug() << QString("%1/%2/restart.sh %3").arg(qApp->applicationDirPath()).arg(SCRIPT_DIR_NAME).arg(qAppName());
//        system(qPrintable(QString("%1/%2/restart.sh %3").arg(qApp->applicationDirPath()).arg(SCRIPT_DIR_NAME).arg(qAppName())));
    }
#endif
}

void MainWindow::selectAllPara()
{
    QList<QCheckBox *> checkBoxes;
    QCheckBox *senderCheckBox = qobject_cast<QCheckBox *>(sender());

    if (senderCheckBox->objectName().contains(paramLoader->paramType2Abbr(ParamLoader::Operating))) {
        checkBoxes = ui->stackedWidgetOperationPara->currentWidget()->findChildren<QCheckBox *>();
    } else if (senderCheckBox->objectName().contains(paramLoader->paramType2Abbr(ParamLoader::Warning))) {
        checkBoxes = ui->stackedWidgetWarningPara->currentWidget()->findChildren<QCheckBox *>();
    } else if (senderCheckBox->objectName().contains(paramLoader->paramType2Abbr(ParamLoader::Calibration))) {
        checkBoxes = ui->stackedWidgetCalibrationPara->currentWidget()->findChildren<QCheckBox *>();

    }

    foreach (QCheckBox *checkBox, checkBoxes) {
        if (checkBox->isVisible()) {
            checkBox->setChecked(senderCheckBox->isChecked());
        }
    }
}


void MainWindow::hideWidget(bool hide)
{
    // history
    ui->pushButton0_5->setHidden(hide);

    //para
    ui->pushButton1_2->setHidden(hide);
    ui->line_9->setHidden(hide);
//    ui->verticalSpacer->changeSize(20,90);

	//force mode
    ui->label_44->setHidden(hide);
    ui->m_fMode->setHidden(hide);
    ui->m_fMode_btn->setHidden(hide);

    ui->m_capacity->setHidden(hide);
    ui->startBalance->setDisabled(hide);

}

/**
 * calculate the quantity of electricty
 * every 2 seconds
 **/
//void MainWindow::countQuantity()
//{
//    for (int i = 0;i < MODULE_NUM; ++i) {
//        if ((module[i].moduleCurrent == 0) || (module[i].moduleVoltage == 0) ) {
//            continue;
//        }

//        // W = U*I*t
//        float singleQ = (float)(module[i].moduleVoltage * module[i].moduleCurrent * 2) / 360000000.0;
//        if (module[i].chargeState == 0x01) {
//            if(module[i].chargeState_bak == 0x02) {                 // trun into charging from discharging
//                quantity[i].singleChargeQuantity = 0;
//            }

//            quantity[i].singleChargeQuantity += singleQ;
//            quantity[i].totalCharge += singleQ;
//            module[i].chargeState_bak = module[i].chargeState;
//        } else if (module[i].chargeState == 0x02) {
//            if(module[i].chargeState_bak == 0x01) {                 // turn into discharging from charging
//                quantity[i].singleDischargeQuantity = 0;
//            }

//            quantity[i].singleDischargeQuantity += singleQ;
//            quantity[i].totalDischarge += singleQ;
//            module[i].chargeState_bak = module[i].chargeState;
//        }
//    }
//}

void MainWindow::countQuantity()
{
    quint8 state = 0;                       // 1:charging 2:discharging
    int U = 0;
    int I = 0;
    int soc = 0;
    qreal once = 0.0;                       // quantity of electricity(KWh)
    const qreal t = 2.0 / 60 / 60;          // time(h)

    for (uint i = 0; i < MODULE_NUM; ++i) {
        U = module[i].moduleVoltage;
        I = module[i].moduleCurrent;
        soc = module[i].moduleSoc;
        state = module[i].chargeState;

        if (0 == U || 0 == I || 0 == soc || 1 > state || 2 < state) {
            continue;
        }

        once = (U * 0.1) * (I * 0.1) * t * 0.001;
        if (1 == state) {
            if(2 == module[i].chargeState_bak) {
                quantity[i].singleChargeQuantity = 0;
            }

            quantity[i].singleChargeQuantity += once;
            quantity[i].totalCharge += once;
        }

        if (2 == state) {
            if(1 == module[i].chargeState_bak) {
                quantity[i].singleDischargeQuantity = 0;
            }

            quantity[i].singleDischargeQuantity += once;
            quantity[i].totalDischarge += once;
        }

        module[i].chargeState_bak = state;
    }
}

void MainWindow::saveQuantity()
{
    config_others->beginWriteArray("module");

    for(uint i = 0;i < MODULE_NUM;i++)
    {
        config_others->setArrayIndex(i);

        config_others->setValue("totalCharge",quantity[i].totalCharge);
        config_others->setValue("totalDischarge",quantity[i].totalDischarge);
    }

    config_others->endArray();
    config_others->sync();

//	callSystemSync();
}

void MainWindow::clearModuleData(unsigned n)
{
    qDebug() << __func__ << "111" << n << sizeof(fictitiousSoc);
    qMemSet(module + n, 0x00, sizeof(*module));
    fictitiousSoc[n].soc = 0;
    qDebug() << __func__ << "222";
}

bool MainWindow::clearModuleWarning(unsigned n)
{
    QMutexLocker locker(&MapMutex);

    bool ret = false;
    QMap<int, int>::iterator i = wnMarks.begin();
    while (i != wnMarks.end()) {
        if (0 == i.value() - n) {
            i = wnMarks.erase(i);
            ret = true;
        } else {
            i++;
        }
    }

    return ret;
}

/*QString* MainWindow::getPtr(QString flag,int id,int page)
{
    if(flag == "m"){
        switch(id){
        case 1:
            return &para[page].charge_cutoff_voltage;
        case 2:
            return &para[page].charge_limit_voltage_1;
        case 3:
            return &para[page].charge_limit_voltage_2;
        case 4:
            return &para[page].discharge_cutoff_voltage;
        case 5:
            return &para[page].discharge_limit_voltage_1;
        case 6:
            return &para[page].discharge_limit_voltage_2;
        case 7:
            return &para[page].fan_start_temp;
        case 8:
            return &para[page].fan_stop_temp;
        case 9:
            return &para[page].balance_start;
        case 10:
            return &para[page].nominal_capcity;
        case 11:
            return &para[page].temp_high_limit;
        case 12:
            return &para[page].temp_low_limit;
        case 13:
            return &para[page].temp_high_normal;
        case 14:
            return &para[page].temp_low_normal;
        case 15:
            return &para[page].soc_high_limit;
        case 16:
            return &para[page].soc_low_limit;
        case 17:
            return &para[page].voltage_consistency;
        case 18:
            return &para[page].temp_consistency;
        case 19:
            return &para[page].charge_current_limit;
        case 20:
            return &para[page].discharge_current_limit;
        case 21:
            return &para[page].charge_current_limit_2;
        case 22:
            return &para[page].discharge_current_limit_2 ;
        case 23:
            return &para[page].charge_current_limit_1;
        case 24:
            return &para[page].discharge_current_limit_1;
        case 25:
            return &para[page].voltage_refresh_period;
        case 26:
            return &para[page].temp_refresh_period;
        case 27:
            return &para[page].insulation_test_enable;
        case 28:
            return &para[page].voltage_consistency_restore;
        case 29:
            return &para[page].temp_consistency_restore;
        case 30:
            return &para[page].jumper_copper_resistance;
        case 31:
            return &para[page].equilibrium_enable;
        default:
            break;
        }
    }else if(flag == "f"){
        switch(id){
        case 1:
            return &para[page].over_voltage;
        case 2:
            return &para[page].under_voltage;
        case 3:
            return &para[page].charge_over_temp;
        case 4:
            return &para[page].charge_under_temp;
        case 5:
            return &para[page].charge_over_current;
        case 6:
            return &para[page].discharge_over_current;
        case 7:
            return &para[page].insulation_limit;
        case 8:
            return &para[page].module_over_voltage;
        case 9:
            return &para[page].module_under_voltage;
        case 10:
            return &para[page].pack_comm_time;
        case 11:
            return &para[page].module_comm_time;
        case 12:
            return &para[page].discharge_over_temp;
        case 13:
            return &para[page].discharge_under_temp;
        default:
            break;
        }
    }else if(flag == "c"){
        switch(id){
        case 1:
            return &para[page].current_K;
        case 2:
            return &para[page].current_B;
        case 3:
            return &para[page].precharge_voltage_K;
        case 4:
            return &para[page].precharge_voltage_B;
        case 17:
            return &para[page].voltage_K;
        case 18:
            return &para[page].voltage_B;
        default:
            break;
        }
    }else if(flag == "w"){
        switch(id)
        {
        case 1:
            return &para[page].over_temp_2;
        case 2:
            return &para[page].over_temp_3;
        case 3:
            return &para[page].under_temp_2;
        case 4:
            return &para[page].under_temp_3;
        case 5:
            return &para[page].over_voltage_2;
        case 6:
            return &para[page].over_voltage_3;
        case 7:
            return &para[page].under_voltage_2;
        case 8:
            return &para[page].under_voltage_3;
        case 9:
            return &para[page].module_over_voltage_2;
        case 10:
            return &para[page].module_over_voltage_3;
        case 11:
            return &para[page].module_under_voltage_2;
        case 12:
            return &para[page].module_under_voltage_3;
        case 13:
            return &para[page].module_over_current_2;
        case 14:
            return &para[page].module_over_current_3;
        case 15:
            return &para[page].temp_consistency_2;
        case 16:
            return &para[page].temp_consistency_3;
        case 17:
            return &para[page].voltage_consistency_2;
        case 18:
            return &para[page].voltage_consistency_3;
        case 19:
            return &para[page].insulation_limit_2;
        case 20:
            return &para[page].insulation_limit_3;
        case 21:
            return &para[page].SOH_limit;
        case 22:
            return &para[page].SOC_high_limit_1;
        case 23:
            return &para[page].SOC_high_limit_2;
        case 24:
            return &para[page].SOC_high_limit_3;
        case 25:
            return &para[page].SOC_low_limit_1;
        case 26:
            return &para[page].SOC_low_limit_2;
        case 27:
            return &para[page].SOC_low_limit_3;
        default:
            break;
        }
    }

    return NULL;
}*/


int MainWindow::get_m_page()
{
    return m_page-1;
}

MainWindow::~MainWindow()
{
    delete ui;
    delete fs;
    delete factoryDialog;

    Logger::destroyInstance();
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::setBuzzer(bool isOn)
{
    if (0 == systemType) {
        return;
    }

    hmi->setIO(0, isOn);
}

void MainWindow::checkBuzzer()
{
    bool isOn = false;
    bool toggleMute = false;

    if (0 == systemType) {
        return;
    }

    for (unsigned i = 0; i < MODULE_NUM; ++i) {
        if (isMdMainRelayOff(i) && isMdFt(i + 1)) {
            isOn = true;
            break;
        }
    }

    if (!isOn) {
        setBuzzer(isOn);
        return;
    }

    if (!mute) {
        setBuzzer(isOn);
        return;
    }

    QMap<int, int>::ConstIterator it = wnMarks.constBegin();
    do {
        if (1 != paramLoader->getWnLevelById(it.key())) {
            continue;
        }

        if (!wnMarksMute.contains(it.key())) {
            toggleMute = true;
            break;
        }

        if (!hasKeyValue(wnMarksMute, it.key(), it.value())) {
            toggleMute = true;
            break;
        }
    } while (++it != wnMarks.constEnd());

    if (toggleMute) {
        muteClicked();
    }

    setBuzzer(!mute);
}

void MainWindow::timerEvent(QTimerEvent *e)
{
    if(e->timerId() == timerID_0){
        //precharge relay
        viewPreChargeRelay();
    }
    else if(e->timerId() == timerID_1) {

        date->setText(QString("| Date: ")+QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss dddd "));

        // fault
        showFault();

        // buzzer
        checkBuzzer();

        emit feedBack2BMS_Signal();
        emit send2PCS_Signal();//pcs

        if(!mcu_updating) emit send2BMS_Signal();//bms
    } else if (e->timerId() == timerID_1_8) {

    }
    else if(e->timerId() == timerID_2) {

        checkDisk();
        // obsoleted
//        countQuantity();
    }
    else if(e->timerId() ==  timerID_5) {

        checkPara();
        hmi->FeedWDog();
        emit modbusDataMap_Signal();
        emit send2EMS_Signal();
        emit checkWarning_Signal();
        freshUI();

        /****** TEST LH-IO606 ******/
//        hmi->getIO(true, 4, 10);
//        hmi->getIO(false, 4, 8);
    } else if (e->timerId() == timerID_10) {
        checkCOMM();
    }
    else if(e->timerId() ==  timerID_15) {

        ui->dateTime->setDateTime(QDateTime::currentDateTime());
        emit readLog_Signal();
    }
    else if(e->timerId() ==  timerID_60) {

        saveQuantity();
    }
    else if(e->timerId() ==  timerID_X) {

       emit saveData_Signal();
    }
    else if(e->timerId() ==  timerID_XX) {
        //
    }

}

void MainWindow::showEvent(QShowEvent *e)
{
    Q_UNUSED(e)

    //hide widget
    if (userID == 0) {
        hideWidget(true);
        ui->userinfo->setText(QString("Welcome,user."));
        qCritical() << QString("user login success!");
    } else if(userID == 1) {
        hideWidget(false);
        ui->userinfo->setText(QString("Welcome,admin."));
        qCritical() << QString("admin login success!");
    }

    //read log
    emit readLog_Signal();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    Q_UNUSED(e)

    config->sync();
    config_others->sync();

    g_log.flush();
    g_log.close();
    g_logBak.flush();
    g_logBak.close();

    modbus_free(modbus);
    modbus_free(modbus_tcp);

    for(uint i = 0; i < MODULE_NUM + 1; i++)
    {
        modbus_mapping_free(mb_mapping[i]);
    }

    for(int i = 0; i < 8; i++)
    {
        bgWorker[i]->stopThread();
        delete bgWorker[i];
    }

    delete myModel_v;
    delete myModel_t;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    static int press_y   = 0;
    static int move_y    = -1;
    static int release_y = 0;
    static QDateTime pressDateTime;
    static QPropertyAnimation *animation = new QPropertyAnimation;
    QScrollBar *m_scrollBarV = NULL;

    if("qt_scrollarea_viewport" != obj->objectName())
    {
        return false;
    }

//    QObject *parent = obj->parent();
//    qDebug() << parent->objectName() << parent->metaObject()->className();
    if (0 == QString("QTableWidget").compare(obj->parent()->metaObject()->className())
            || 0 == QString("QTableView").compare(obj->parent()->metaObject()->className())) {
        m_scrollBarV = qobject_cast<QTableView *>(obj->parent())->horizontalScrollBar();
    } else {
        return false;
    }
//    QScrollBar *m_scrollBarV = ui->tableWidgetOverview->horizontalScrollBar();
    int scrollV_max = m_scrollBarV->maximum ();
    int scrollV_min = m_scrollBarV->minimum ();

    //根据鼠标的动作——按下、放开、拖动，执行相应的操作
    if(event->type() == QEvent::MouseButtonPress)
    {
        //记录按下的时间、坐标

        pressDateTime = QDateTime::currentDateTime();
        move_y  = QCursor::pos().x();
        press_y = move_y;

        animation->stop();
    }
    else if(event->type() == QEvent::MouseButtonRelease)
    {
        //鼠标放开，根据鼠标拖动的垂直距离和持续时间，设置窗口滚动快慢程度和距离

        if(animation->targetObject() != m_scrollBarV)
        {
            animation->setTargetObject(m_scrollBarV);
            animation->setPropertyName("value");
        }

        move_y = -1;
        release_y = QCursor::pos().x();
//        qDebug()<<"MouseButtonRelease QCursor::pos().y()="<<QCursor::pos().y();
        QObject *parent_obj = obj->parent();
        if(parent_obj != 0 || parent_obj->inherits("QAbstractItemView"))
        {
            QTimer::singleShot(150, (QAbstractItemView *)parent_obj
                               , SLOT(clearSelection()));
        }

        int endValue = 0;
        int pageStep;
        if(release_y - press_y != 0 && qAbs(release_y - press_y) > 45)
        {
            //qDebug()<<"obj->objectName()="<<obj->objectName();
            int mseconds = pressDateTime.msecsTo(QDateTime::currentDateTime());
//            qDebug()<<"mseconds="<<mseconds;

            int limit = 440;
            pageStep = 240;//scrollBarV->pageStep();
//            qDebug()<<"pageStep="<<pageStep;
            if(mseconds > limit)//滑动的时间大于某个值的时候，不再滚动(通过增加分母)
            {
                mseconds = mseconds + (mseconds - limit) * 20;
            }

            if(release_y - press_y > 0)
            {
                endValue = m_scrollBarV->value()
                        - pageStep * (200.0 / mseconds);//.0避免避免强制转换为整形
                if(scrollV_min > endValue)
                {
                    endValue = scrollV_min;
                }
            }
            else if(release_y - press_y < 0)
            {
                    endValue = m_scrollBarV->value() + pageStep * (200.0 / mseconds);
                    if(endValue > scrollV_max)
                    {
                        endValue = scrollV_max;
                    }
            }
            if(mseconds > limit)
            {
                mseconds = 0;//滑动的时间大于某个值的时候，滚动距离变小，减小滑动的时间
            }
            animation->setDuration(mseconds + 550);
            animation->setEndValue(endValue);
            animation->setEasingCurve(QEasingCurve::OutQuad);
            animation->start();
            return true;
        }
    }
    else if(event->type() == QEvent::MouseMove && move_y >= 0)
    {
        //窗口跟着鼠标移动

        int move_distance = QCursor::pos().x() - move_y;
        int endValue = m_scrollBarV->value() - move_distance;
        if(scrollV_min > endValue)
        {
            endValue = scrollV_min;
        }

        if(endValue > scrollV_max)
        {
            endValue = scrollV_max;
        }
        m_scrollBarV->setValue(endValue);
        //qDebug()<<"endValue="<<endValue;
        //qDebug()<<"move_distance="<<move_distance;
        move_y = QCursor::pos().x();
    }
    if (QEvent::TouchBegin == event->type()) {
        qDebug() << __func__ << "TouchBegin";
    }
    return false;
}


void MainWindow::on_balancePackID_currentIndexChanged(int index)
{
    p_page = index + 1;

    ui->packID->setText(QString::number(p_page));
}

void MainWindow::showFault()
{
    if(sendTime){
        if(!ui->sending->isVisible())ui->sending->setVisible(true);
        ui->sendTime->setText(QString("%1 S").arg(sendTime));

        if(sendTime++ > (sendGlobal ? CAN_TIMEOUT : CAN_TIMEOUT_SIGNEL)){

            paraACK(0,2);
        }

        paraACK(0,0);
    }
    else{
        if(ui->sending->isVisible())ui->sending->setVisible(false);
        if(ui->sendTime->text() != "")ui->sendTime->setText("");
    }

    if(tableRow < ui->table->rowCount()){
        showMsg(tr("FaultCode: %1,Value: %2, Info: %3").arg(ui->table->item(tableRow,0)->text())
                                                .arg(ui->table->item(tableRow,1)->text())
                                                .arg(ui->table->item(tableRow,2)->text()),2000);
        tableRow++;
    }
    else{

        tableRow = 0;
    }
}

void MainWindow::viewPreChargeRelay()
{
    static int transparency = 0;

    if (module[m_page-1].preChargeRelay) {
        if (!ui->m_preRelay->isVisible())
            ui->m_preRelay->show();

        transparency = (transparency + 50) > 0xFF ? 0 : (transparency + 50);
        ui->m_preRelay->setStyleSheet(QString("color:rgb(229, 30, 95, %1)")
                                      .arg(transparency));
    } else {
        if (ui->m_preRelay->isVisible())
            ui->m_preRelay->hide();
    }

}

void MainWindow::on_easterEggLabelMaintenance_triggered()
{
    factoryDialog->showPage(Maintenance);
}

void MainWindow::on_easterEggLabel_triggered()
{
    const quint8 networkAddrGate = 1;
    const quint8 networkAddrCorp = 209;
    const quint8 networkAddrHome = 116;
    const QString networkSegCorp = "10.2.100.";
    const QString networkSegHome = "192.168.1.";

    QString ip = ui->ip->text();
    QString gateway = ui->gateway->text();

    if(QMessageBox::Ok == QMessageBox::question(this, "Engineering Mode", "Are you sure to switch network configuration?"))
    {
        if(ip.contains(networkSegCorp) && gateway.contains(networkSegCorp))
        {
            ui->ip->setText(networkSegHome + QString::number(networkAddrHome));
            ui->gateway->setText(networkSegHome + QString::number(networkAddrGate));
        }
        else if(ip.contains(networkSegHome) && gateway.contains(networkSegHome)) {
            ui->ip->setText(networkSegCorp + QString::number(networkAddrCorp));
            ui->gateway->setText(networkSegCorp + QString::number(networkAddrGate));
        }
    }
}

void MainWindow::on_easterEggLabel4RealSoc_triggered()
{
    ui->checkBoxRealSoc->setVisible(!ui->checkBoxRealSoc->isVisible());
}

void MainWindow::on_labelVersion_clicked()
{
    // do MD5 checksum
    QFile appFile(qApp->applicationFilePath());
    QByteArray hash;

    if (appFile.open(QFile::ReadOnly)) {
        hash = QCryptographicHash::hash(appFile.readAll(), QCryptographicHash::Md5);
        appFile.close();
        showMsg(hash.toHex().toUpper());
    }
}

void MainWindow::on_pushButtonLock_clicked()
{
    hide();
    emit logout();
}

void MainWindow::on_pushButtonDisplayOff_clicked()
{
    if (NULL == fs) {
        return;
    }

    fs->showFullScreen();
    connect(fs, SIGNAL(clicked()), this, SLOT(displayRestore()));
    hmi->setBacklightLevel(0);
}

void MainWindow::displayRestore()
{
    if (NULL == fs) {
        return;
    }

    fs->hide();
    disconnect(fs, SIGNAL(clicked()), this, SLOT(displayRestore()));
    usleep(500 * 1000);
    changeBacklight();
}

void MainWindow::loginByUser(int type)
{
    userID = type;
    showFullScreen();
    raise();
}

void MainWindow::readDigitalInput(quint8 addr, quint8 len, QBitArray bitArray)
{
    Q_UNUSED(addr);

    int yxaddr = 0x08 - 1;
    static bool flip = false;

#ifndef XINLONG
    if (13 < bitArray.size()) {
        dcSideSwitch[0] = (0 != bitArray.at(7));
        dcSideSwitch[1] = (0 != bitArray.at(13));
    }

    if (21 < bitArray.size()) {
        if (bitArray.at(1) || bitArray.at(2) || bitArray.at(3)
                || bitArray.at(11)/* || bitArray.at(22)*/) {
            qDebug() << "*** gang control ***";
            hmi->setIO(7, 0, 6, true);
            isEmergency = true;
        } else {
//            isEmergency = false;
        }
    }
#else
    if (2 < bitArray.size()) {
        if (bitArray.at(1) || bitArray.at(2)) {
            can_frame cmdFrame;

            qMemSet(&cmdFrame, 0x00, sizeof(cmdFrame));
            cmdFrame.can_id = CAN::BAMS_SEND_CALIBRATION_PARA << 4;
            cmdFrame.can_dlc = 8;
            cmdFrame.data[0] = 0x70;
            cmdFrame.data[1] = 25;
            cmdFrame.data[5] = 0x01;
            hmi->WriteCan(&cmdFrame);
            qDebug() << __func__ << "broadcast external error.";

            for (unsigned i = 0; i < MODULE_NUM; ++i) {
                hmi->setIO(i, true);
            }
        } else if (bitArray.at(0)) {
            qWarning() << "*** fire control failure ***";
        }
    }
#endif

    /****** remote signaling part1 ******/
    for (int i = 0; i < len; ++i, ++yxaddr) {
        setyx(yxaddr, bitArray.at(i) ? 1 : 0);
    }

    /****** set fire alarm ******/
    if (bitArray.at(0)) {
        isFireAlarm = true;
    }
}

void MainWindow::readDigitalOutput(quint8 addr, quint8 len, QBitArray bitArray)
{
    Q_UNUSED(addr)
    Q_UNUSED(len)
    Q_UNUSED(bitArray)
}

void MainWindow::readPeripheral(int type, int index, int result)
{
    switch (type) {
    case 1:
        if (0 > result) {
            ui->tableWidgetTempHumidity->setItem(index, 0, new QTableWidgetItem("FAILED"));
        } else {
            ui->tableWidgetTempHumidity->setItem(index, 0, new QTableWidgetItem("OK"));
            ui->tableWidgetTempHumidity->setItem(index, 1, new QTableWidgetItem(QString::number(hmi->peripheral.tempHumidityMeter[index].humidity)));
            ui->tableWidgetTempHumidity->setItem(index, 2, new QTableWidgetItem(QString::number(hmi->peripheral.tempHumidityMeter[index].temperature)));
        }
        break;
    case 2:
        if (0 > result) {
            ui->tableWidgetDcInsulation->setItem(index, 0, new QTableWidgetItem("FAILED"));
        } else {
            ui->tableWidgetDcInsulation->setItem(index, 0, new QTableWidgetItem("OK"));
            ui->tableWidgetDcInsulation->setItem(index, 1, new QTableWidgetItem(QString::number(hmi->peripheral.dcInsulationMonitoringDevice[index].bus_voltage)));
            ui->tableWidgetDcInsulation->setItem(index, 2, new QTableWidgetItem(QString::number(hmi->peripheral.dcInsulationMonitoringDevice[index].total_bus_resistance)));
            ui->tableWidgetDcInsulation->setItem(index, 3, new QTableWidgetItem(QString::number(hmi->peripheral.dcInsulationMonitoringDevice[index].positive_bus_resistance_to_ground)));
            ui->tableWidgetDcInsulation->setItem(index, 4, new QTableWidgetItem(QString::number(hmi->peripheral.dcInsulationMonitoringDevice[index].negative_bus_resistance_to_ground)));
         }
        break;
    case 3:
        if (0 > result) {
            ui->tableWidgetAirConditioner->setItem(index, 0, new QTableWidgetItem("FAILED"));
        } else {
            ui->tableWidgetAirConditioner->setItem(index, 0, new QTableWidgetItem("OK"));
            ui->tableWidgetAirConditioner->setItem(index, 1, new QTableWidgetItem(QString::number(hmi->peripheral.airConditioner[index].working_status)));
            ui->tableWidgetAirConditioner->setItem(index, 2, new QTableWidgetItem(QString::number(hmi->peripheral.airConditioner[index].inside_fan_status)));
            ui->tableWidgetAirConditioner->setItem(index, 3, new QTableWidgetItem(QString::number(hmi->peripheral.airConditioner[index].outside_fan_status)));
            ui->tableWidgetAirConditioner->setItem(index, 4, new QTableWidgetItem(QString::number(hmi->peripheral.airConditioner[index].compressor_status)));
            ui->tableWidgetAirConditioner->setItem(index, 5, new QTableWidgetItem(QString::number(hmi->peripheral.airConditioner[index].cabinet_air_temp)));
            ui->tableWidgetAirConditioner->setItem(index, 6, new QTableWidgetItem(QString::number(hmi->peripheral.airConditioner[index].water_pump_temp)));
            ui->tableWidgetAirConditioner->setItem(index, 7, new QTableWidgetItem(QString::number(hmi->peripheral.airConditioner[index].out_cabinet_temp)));
            ui->tableWidgetAirConditioner->setItem(index, 8, new QTableWidgetItem(QString::number(hmi->peripheral.airConditioner[index].condenser_temp)));
            ui->tableWidgetAirConditioner->setItem(index, 9, new QTableWidgetItem(QString::number(hmi->peripheral.airConditioner[index].evaporator_temp)));
        }
        break;
    case 4:
        if (0 > result) {
            ui->tableWidgetElectricityMeter->setItem(index, 0, new QTableWidgetItem("FAILED"));
        } else {
            ui->tableWidgetElectricityMeter->setItem(index, 0, new QTableWidgetItem("OK"));
            ui->tableWidgetElectricityMeter->setItem(index, 1, new QTableWidgetItem(QString::number(hmi->peripheral.airConditioner[index].working_status)));
            ui->tableWidgetElectricityMeter->setItem(index, 2, new QTableWidgetItem(QString::number(hmi->peripheral.airConditioner[index].inside_fan_status)));
            ui->tableWidgetElectricityMeter->setItem(index, 3, new QTableWidgetItem(QString::number(hmi->peripheral.airConditioner[index].outside_fan_status)));
            ui->tableWidgetElectricityMeter->setItem(index, 4, new QTableWidgetItem(QString::number(hmi->peripheral.airConditioner[index].compressor_status)));
            ui->tableWidgetElectricityMeter->setItem(index, 5, new QTableWidgetItem(QString::number(hmi->peripheral.airConditioner[index].cabinet_air_temp)));
            ui->tableWidgetElectricityMeter->setItem(index, 6, new QTableWidgetItem(QString::number(hmi->peripheral.airConditioner[index].water_pump_temp)));
            ui->tableWidgetElectricityMeter->setItem(index, 7, new QTableWidgetItem(QString::number(hmi->peripheral.airConditioner[index].out_cabinet_temp)));
            ui->tableWidgetElectricityMeter->setItem(index, 8, new QTableWidgetItem(QString::number(hmi->peripheral.airConditioner[index].condenser_temp)));
            ui->tableWidgetElectricityMeter->setItem(index, 9, new QTableWidgetItem(QString::number(hmi->peripheral.airConditioner[index].evaporator_temp)));
        }
        break;
    default:
        break;
    }
}

void MainWindow::doSendFactoryCalib(int n, int value)
{
    can_frame frame;

    qDebug() << __func__ << n << value;
    frame.can_id = 0x3A5;
    frame.can_dlc = 3;
    frame.data[0] = (quint8)n;
    frame.data[1] = (quint8)(value >> 8);
    frame.data[2] = (quint8)value;

    paraACK(0, 1);
    sendFlag = "c";
    hmi->WriteCan(&frame);

    if (0 == n % 2) {
        initPara("c", para_page);
    }
}
#if 0
/**
 * @brief   copy all the data file stored in the folder named with year and month one by one.
 */
void MainWindow::doDataDirCopy()
{
    QString destFilePath = QString("%1/data/%2/%3");
    QDir dir(APP_DATA_DIRPATH);

//    storagePath = "/mnt/userdata1";                 // only for debug
    extDiskUsingFlag = true;    // using this flag to avoid diskCheck() while data copying.
    if (dirQueue.isEmpty()) {
        qDebug() << __func__ << "all done. before sync" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:zzz");
        QtConcurrent::run(this, &MainWindow::callSystemSync);
        qDebug() << __func__ << "sync finished" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:zzz");
        extDiskUsingFlag = false;
        return;
    }

    if (fileQueue.isEmpty()) {
        qDebug() << __func__ << dirQueue.dequeue().absoluteFilePath() << __func__ << "dir copied";
        if (dirQueue.isEmpty()) {
            qDebug() << __func__ << "all done. before sync" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:zzz");
            QtConcurrent::run(this, &MainWindow::callSystemSync);
            qDebug() << __func__ << "sync finished" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:zzz");
            extDiskUsingFlag = false;
            return;
        }

        if (!dir.cd(dirQueue.head().fileName())) {
            qDebug() << __func__ << "cd failed:" << dirQueue.head().fileName();
            extDiskUsingFlag = false;
            return;
        }

        foreach (QFileInfo info, dir.entryInfoList()) {
            if (info.isFile()) {
                fileQueue.enqueue(info);
                qDebug() << __func__ << "333" << info.fileName();
            }
        }

        if (!fileQueue.isEmpty() && !dir.mkpath(storagePath + "/data/" + dirQueue.head().fileName())) {
            qDebug() << __func__ << "mkpath failed.";
            extDiskUsingFlag = false;
            return;
        }

        doDataDirCopy();
    } else {
        dir.setPath(storagePath + "/data/" + dirQueue.head().fileName());
        if (!dir.exists()) {
            if (!dir.mkpath(dir.absolutePath())) {
                qDebug() << __func__ << "mkpath failed:" << dir.absolutePath();
                extDiskUsingFlag = false;
                return;
            }
        }

        destFilePath = destFilePath.arg(storagePath).arg(dirQueue.head().fileName()).arg(fileQueue.head().fileName());
        copier->setSrcFileName(fileQueue.head().absoluteFilePath());
        copier->setDestFileName(destFilePath);
        qDebug() << __func__ << fileQueue.dequeue().absoluteFilePath() << "file copying to"
                 << destFilePath;
        ui->labelTotalProgress->setText(QString("%1 / %2").arg(++nCopied).arg(nTotal));
        emit startCopyRsquested();
    }
}
#else
// no recursion + slot
void MainWindow::doDataDirCopy()
{
    QElapsedTimer timer;
    timer.start();
    qDebug() << __func__ << "****** Start Time Count" << QTime::currentTime().toString("hh:mm:ss:zzz");
    QString destFilePath = QString("%1/data/%2/%3");
    QDir dir(APP_DATA_DIRPATH);

//    storagePath = "/mnt/userdata1";                 // only for debug
    do {
        if (!fileQueue.isEmpty()) {
            break;
        }

        qDebug() << __func__ << dirQueue.dequeue().absoluteFilePath() << __func__ << "dir copied";
        if (dirQueue.isEmpty()) {
            qDebug() << __func__ << "all done. before sync" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:zzz");
//            QtConcurrent::run(this, &MainWindow::callSystemSync);
//            callSystemSync();
            QProcess process;
            process.start("sync");
            process.waitForFinished();
            qDebug() << __func__ << "sync finished" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:zzz");
            return;
        }

        if (!dir.cd(dirQueue.head().fileName())) {
            qDebug() << __func__ << "cd failed:" << dirQueue.head().fileName();
            return;
        }

        foreach (QFileInfo info, dir.entryInfoList()) {
            if (info.isFile()) {
                fileQueue.enqueue(info);
                qDebug() << __func__ << "333" << info.fileName();
            }
        }

        if (!fileQueue.isEmpty() && !dir.mkpath(storagePath + "/data/" + dirQueue.head().fileName())) {
            qDebug() << __func__ << "mkpath failed.";
            return;
        }
    } while (!dirQueue.isEmpty() && fileQueue.isEmpty());

    dir.setPath(storagePath + "/data/" + dirQueue.head().fileName());
    if (!dir.exists()) {
        if (!dir.mkpath(dir.absolutePath())) {
            qDebug() << __func__ << "mkpath failed:" << dir.absolutePath();
            return;
        }
    }

    destFilePath = destFilePath.arg(storagePath).arg(dirQueue.head().fileName()).arg(fileQueue.head().fileName());
    copier->setSrcFileName(fileQueue.head().absoluteFilePath());
    copier->setDestFileName(destFilePath);
    qDebug() << __func__ << fileQueue.dequeue().absoluteFilePath() << "file copying to"
             << destFilePath;
    ui->labelTotalProgress->setText(QString("%1 / %2").arg(++nCopied).arg(nTotal));
    emit startCopyRsquested();
    qDebug() << __func__ << "****** End Time Count"
             << QTime::currentTime().toString("hh:mm:ss:zzz")
             << "elapsed:" << timer.elapsed() << "ms";
}
#endif

void MainWindow::dataDirCpyWrapper()
{
    QtConcurrent::run(this, &MainWindow::doDataDirCopy);
}
/**
 * @brief validate date dir name.
 * @param name data dir name.
 * @return true data dir name is valid, otherwise return false.
 */
bool MainWindow::isDataDirNameValid(const QString &name)
{
    int year = 0;
    int month = 0;
    bool ok = false;
    bool ret = false;
    QStringList list = name.split("-");

    if (2 == list.size()) {
        year = list.at(0).toInt(&ok);
        if (ok) {
            month = list.at(1).toInt(&ok);
            if (ok) {
                ret = QDate::isValid(year, month, 1);
            }
        }
    }

    return ret;
}

bool MainWindow::isDataFileNameValid(const QString &name)
{
    int year = 0;
    int month = 0;
    int day = 0;
    bool ok = false;
    bool ret = false;
    QString _name = name;
    QStringList list;

    _name.remove(".csv");
    list = _name.split("-");
    if (3 == list.size()) {
        year = list.at(0).toInt(&ok);
        if (ok) {
            month = list.at(1).toInt(&ok);
            if (ok) {
                day = list.at(2).toInt(&ok);
                if (ok) {
                    ret = QDate::isValid(year, month, day);
                }
            }
        }
    }

    return ret;
}

int MainWindow::getDataFileNum(const QString &path)
{
    quint64 count = 0;
    QDir dir(path);

    foreach (QFileInfo fileInfo, dir.entryInfoList(QDir::Files)) {
        if (0 == fileInfo.suffix().compare("csv") && isDataFileNameValid(fileInfo.fileName())) {
            qDebug() << __func__ << fileInfo.absoluteFilePath();
            ++count;
        }
    }

    foreach(QString subDir, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (isDataDirNameValid(subDir)) {
            count += getDataFileNum(path + QDir::separator() + subDir);         // recursion
        }
    }

    return count;
}

quint64 MainWindow::getDirSize(const QString &path)
{
    quint64 size = 0;
    QDir dir(path);

    foreach (QFileInfo fileInfo, dir.entryInfoList(QDir::Files)) {
        size += fileInfo.size();
    }

    foreach(QString subDir, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        size += getDirSize(path + QDir::separator() + subDir);         // recursion
    }

    return size;
}

quint64 MainWindow::getFilesInDirSize(const QString &path, const QStringList &nameFilters)
{
    quint64 size = 0;
    QDir dir(path);

    foreach (QFileInfo fileInfo, dir.entryInfoList(nameFilters, QDir::Files)) {
        size += fileInfo.size();
    }

    foreach(QString subDir, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        size += getFilesInDirSize(path + QDir::separator() + subDir, nameFilters);         // recursion
    }

    return size;
}

QString MainWindow::improveReadability(const quint64 &size)
{
    int integer = 0;
    int decimal = 0;
    char unit ='B';
    qint64 standardSize = size;
    qint64 curSize = size;
    QString dec = "0";

    if (standardSize > 1024) {       // larger then 1024B
        curSize = standardSize * 1000;
        curSize /= 1024;
        integer = curSize / 1000;
        decimal = curSize % 1000;
        standardSize /= 1024;
        unit = 'K';

        if (standardSize > 1024) {
            curSize = standardSize * 1000;
            curSize /= 1024;
            integer = curSize / 1000;
            decimal = curSize % 1000;
            standardSize /= 1024;
            unit = 'M';

            if (standardSize > 1024) {
                curSize = standardSize * 1000;
                curSize /= 1024;
                integer = curSize / 1000;
                decimal = curSize % 1000;
                unit = 'G';
            }
        }
    }

    if (0 <= decimal && decimal <= 9) {
        dec = dec + dec + QString::number(decimal);
    }

    if (10 <= decimal && decimal <= 99) {
        dec = "0" + QString::number(decimal);
    }

    if (100 <= decimal && decimal <= 999) {
        dec = QString::number(decimal);
    }

    return QString::number(integer) + "." + dec + unit;
}

bool MainWindow::externalStorageDetect()
{
    int ret = true;

    if(ui->usb->isChecked())
    {
        storagePath = USB_PATH;
        if(0 != access(storagePath.toLatin1().data(),F_OK))
        {
            ui->diskInfo->setText(QString("No Udisk!"));
            ret = false;
        }
    }
    else if(ui->sd->isChecked())
    {
        storagePath = SD_PATH;
        if(0 != access(storagePath.toLatin1().data(),F_OK))
        {
            ui->diskInfo->setText(QString("No SDcard!"));
            ret = false;
        }
    }

    return ret;
}

void MainWindow::doParallelOperation()
{
    int reqIndex = getParallelReqIndex(sender());
    can_frame frame;
    QMessageBox *msgBox = new QMessageBox(QMessageBox::Question
                                          , "Parallel Operation"
                                          , "Do Parallel Operation?"
                                          , QMessageBox::NoButton
                                          , this);
    QAbstractButton *btnStart = msgBox->addButton(tr("Start"), QMessageBox::ActionRole);
    QAbstractButton *btnClear = msgBox->addButton(tr("Clear"), QMessageBox::ActionRole);

    frame.can_id = (CAN::BAMS_PARALLEL_OPERATION << 4) | (reqIndex + 1);
    memset(frame.data, 0x00, sizeof(frame.data));
    msgBox->addButton(tr("Cancel"), QMessageBox::RejectRole);
    msgBox->exec();

    if (btnStart == msgBox->clickedButton()) {
        frame.can_dlc = 1;
        frame.data[0] = 0x01;
    } else if (btnClear == msgBox->clickedButton()) {
        frame.can_dlc = 2;
        frame.data[1] = 0x01;

#ifdef XINLONG
        hmi->setIO(reqIndex, false);
#endif

    } else {
        return;
    }

    hmi->WriteCan(&frame);
    showMsg("Send success.");
}

void MainWindow::callSystemSync()
{
    system("sync");
}

void MainWindow::logCanFrame(const can_frame & frame, const QString & prefix)
{
    QChar space(' ');
    QString hexStr;

    for (unsigned i = 0; i < 8; ++i) {
        hexStr += space;
        hexStr += QString("%1").arg(frame.data[i], 2, 16, QChar('0')).toUpper();
    }

    QLOG_TRACE() << prefix << space
                 << QString("%1").arg(frame.can_id & CAN_SFF_MASK, 3, 16, QChar('0')).toUpper().prepend("0x")
                 << hexStr;
}

void MainWindow::logWarningItem(int coordinate, bool isTrigger)
{
    // TO-DO: take a snapshot of runtime
    QChar space(' ');
    Coordiante _coordinate = {coordinate};

    QLOG_INFO() << (isTrigger ? "trigger" : "clear") << space
                << _coordinate.data << space
                << QString("#%1").arg(_coordinate.n) << space
                << paramLoader->getWnNameById(_coordinate.key);
}

void MainWindow::handleBitInWarningSet(int n, int key, bool isExist)
{
    QMutexLocker locker(&MapMutex);

    Coordiante coordinate;
    coordinate.key = key;
    coordinate.n = n;
    coordinate.level = paramLoader->getWnLevelById(key);
    if (isExist) {
        if (!wnMarks.contains(key)) {
            logWarningItem(coordinate.data, true);
            hisRecordModel->addRecord(coordinate.data, true);
            wnMarks.insert(key, n);
            return;
        }

        if (!hasKeyValue(wnMarks, key, n)) {
            logWarningItem(coordinate.data, false);
            hisRecordModel->addRecord(coordinate.data, true);
            wnMarks.insertMulti(key, n);
        }

        return;
    }

    QMap<int, int>::iterator i = wnMarks.lowerBound(key);
    while (i != wnMarks.upperBound(key)) {
        if (n == i.value()) {
            logWarningItem(coordinate.data, false);
            hisRecordModel->addRecord(coordinate.data, false);
            wnMarks.erase(i);
            break;
        }

        ++i;
    }
}

unsigned MainWindow::getN() const
{
    return MODULE_NUM;
}

int MainWindow::getChargeCurrentMax(unsigned n) const
{
    if (MODULE_NUM - 1 < n) {
        return -1;
    }

    return module[n].chargeCurrentMax;
}

int MainWindow::getDischargeCurrentMax(unsigned n) const
{
    if (MODULE_NUM - 1 < n) {
        return -1;
    }

    return module[n].dischargeCurrentMax;
}

QString MainWindow::getWarningDesc(int id) const
{
    unsigned level = paramLoader->getWnLevelById(id);
    QString description = paramLoader->getWnNameById(id);

    switch (level) {
    case 1:
        description += description.endsWith(QString::fromUtf8("故障")) ? "" : QString::fromUtf8("故障");
        break;
    case 2:
        description += description.endsWith(QString::fromUtf8("告警")) ? "" : QString::fromUtf8("告警");
        break;
    case 3:
        description += description.endsWith(QString::fromUtf8("预警")) ? "" : QString::fromUtf8("预警");
        break;
    default:
        break;
    }

    return description;
}

QList<unsigned> MainWindow::getCheckList() const
{
    return checkList;
}

void MainWindow::setCheckList(QList<unsigned> checkList)
{
    this->checkList.clear();
    this->checkList << checkList;
    qDebug() << __func__ << checkList << "this->checkList" << this->checkList;
}

bool MainWindow::loadParamPage(const QString &configPath)
{
    QFile xmlFile(configPath);

    if (!xmlFile.exists() || (QFile::NoError != xmlFile.error())) {
        qDebug() << "ERROR: Unable to open config file " << configPath;
        return false;
    }

    xmlFile.open(QFile::ReadOnly);
    QXmlStreamReader reader(&xmlFile);
    while (!reader.atEnd()) {
        reader.readNext();

        if (reader.isStartElement()) {
            if ("calibration" == reader.name()) {
                parseCalibrationParams(reader);
            } else if ("warning" == reader.name()) {
                parseWarningParams(reader);
            } else if ("running" == reader.name()) {
                parseRunningParams(reader);
            }
        }
    }

    if (reader.hasError()) {
        qDebug() << QString("Error parsing %1 on line %2 column %3: \n%4")
                 .arg(configPath)
                 .arg(reader.lineNumber())
                 .arg(reader.columnNumber())
                 .arg(reader.errorString());
    }

    return true;
}

void MainWindow::parseRunningParams(QXmlStreamReader &reader)
{
    int i = 0;
    QCheckBox *checkBox = NULL;
    QLineEdit *lineEdit = NULL;
    QGroupBox *groupBox = NULL;
    QGridLayout *gridLayout = NULL;
    QHBoxLayout *hBoxLayout = NULL;
    QStackedWidget *stackedWidget = ui->stackedWidgetOperationPara;

    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && "param" == reader.name()) {
            QXmlStreamAttributes attrs = reader.attributes();
            if (0 == i % MAX_PARAM_ROW) {
                if (NULL != gridLayout && NULL != hBoxLayout) {
                    hBoxLayout->addLayout(gridLayout);
                }

                gridLayout = new QGridLayout(groupBox);
            }

            if (0 == i % MAX_PARAM_IN_PAGE) {
                if (NULL != groupBox && NULL != hBoxLayout) {
                    // add a vertical line
                }

                groupBox = groupBoxRunningParam[i / MAX_PARAM_IN_PAGE];
                groupBox = new QGroupBox(stackedWidget);
                hBoxLayout = new QHBoxLayout(groupBox);
                stackedWidget->addWidget(groupBox);
            }

            if (NULL == groupBox) {
                return;
            }

            QStringRef id = attrs.value("id");
            QStringRef name = attrs.value("name");
            QStringRef type = attrs.value("type");
            QStringRef precision = attrs.value("precision");
            QStringRef disp = attrs.value("disp");

            if ("false" == disp) {
                continue;
            }

            checkBox = new QCheckBox(groupBox);
            checkBox->setObjectName("checkBoxR" + id.toString());
            checkBox->setText(name.toString());
            gridLayout->addWidget(checkBox, i % MAX_PARAM_ROW, 0);

            lineEdit = new QLineEdit(groupBox);
            lineEdit->setObjectName("lineEditR" + id.toString());
            gridLayout->addWidget(lineEdit, i % MAX_PARAM_ROW, 1);

            ++i;
        } else if (reader.isEndElement() && "running" == reader.name()) {
            return;
        }
    }
}

void MainWindow::parseWarningParams(QXmlStreamReader &reader)
{
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && "param" == reader.name()) {
            QXmlStreamAttributes attrs = reader.attributes();
            // TO-DO: add pairs
        } else if (reader.isEndElement() && "warning" == reader.name()) {
            return;
        }
    }
}

void MainWindow::parseCalibrationParams(QXmlStreamReader &reader)
{
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && "param" == reader.name()) {
            QXmlStreamAttributes attrs = reader.attributes();
            // TO-DO: add pairs
        } else if (reader.isEndElement() && "calibration" == reader.name()) {
            return;
        }
    }
}

void MainWindow::installKeyPad(QLineEdit *lineEdit)
{
    if (NULL == lineEdit || NULL == myKeyboard) {
        return;
    }

    lineEdit->installEventFilter(myKeyboard);
}

void MainWindow::installKeyPad(QWidget *container)
{
    if (NULL == container) {
        return;
    }

    foreach (QLineEdit *child, container->findChildren<QLineEdit *>()) {
        installKeyPad(child);
    }
}

void MainWindow::installTouchScroll(QTableView *tableView)
{
    if (NULL == tableView) {
        return;
    }

    foreach (QObject *obj, tableView->children()) {
        if (obj->objectName() == "qt_scrollarea_viewport") {
            obj->installEventFilter(this);
        }
    }
}

const QDateTime MainWindow::buildDateTime()
{
    QString dateTime;

    dateTime.clear();
    dateTime += __DATE__;
    dateTime += __TIME__;
    dateTime.replace("  ", " 0");
    return QLocale(QLocale::English).toDateTime(dateTime, "MMM dd yyyyhh:mm:ss");
}

/**
 * @brief   to get whether a map container has a key-value pair,
 *          we used to use 'map.values(key).contains(value)'.
 *          clang will warn us 'allocating an unneeded temporary
 *          container [clazy-container-anti-pattern]'.
 *          now use this functions to correct this.
 * @date    2023/04/28
 * @author  Davy Jones
 */
template <typename Key, typename T>
bool MainWindow::hasKeyValue(const QMap<Key, T> & map, const Key & key, const T & value)
{
    bool ret = false;
    typename QMap<Key, T>::ConstIterator it = map.constFind(key);

    while (it != map.constEnd() && it.key() == key) {
        if (it.value() == value) {
            ret = true;
            break;
        }

        ++it;
    }

    return ret;
}
