#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFile>
#include <QDir>
#include <QMap>
#include <QPropertyAnimation>
#include <sys/statfs.h>
#include <QMutex>
#include <QTimer>
#include <QtGui>
#include <QString>
#include <QDebug>
#include <QTimer>
#include <QDir>
#include <QListView>
#include <QValidator>
#include <QDateTime>
#include <QList>
#include <QMovie>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <QsLog.h>
#include <global.h>
#include <base.h>
#include "module.h"
#include "hmi.h"
#include "config.h"
#include "comm.h"
#include "modbus.h"
#include "mykeyboard.h"
#include "bgworker.h"
#include "mymodel.h"
#include "uihelper.h"
#include "customlabel.h"
#include "login.h"
#include "systemtypedialog.h"
#include "factorydialog.h"
#include "portmgrdialog.h"
#include "filecopier.h"
#include "paramloader.h"
#include <ExampleKbdHandler.h>
#include "hisrecordmodel.h"
#include "callback.h"
#include "qcustomplot.h"

#define chartonum(x) (x-'0')

extern QFile g_log;
extern QFile g_logBak;
extern QFile g_log2;
extern QFile g_log2Bak;

class BgWorker;
class MyModel;
class Login;

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow, UiHelper, public Base, public Callback
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0, int moduleNum = DEFAULT_MODULE_NUM);
    ~MainWindow();

    int systemType;
    int userID;
    bool mcu_ack;
    bool mcu_updating;

    HMI *hmi;
    modbus_t *modbus;
    modbus_t *modbus_tcp;
    modbus_mapping_t **mb_mapping;
#ifdef XINLONG
    MODBUS::PCS *pcs;
#else
    MODBUS::PCS pcs[PCS_NUM];
#endif
    uint16_t *faultArray;

    Heap heap;
    Module *module;
    FictitiousSoc *fictitiousSoc;
    Quantity *quantity;
    Para *para;
    bool isFireAlarm;
    quint16 (*operatingParam)[MAX_PARAM_NUM_PER_MODULE];
    quint16 (*warningParam)[MAX_PARAM_NUM_PER_MODULE];
    quint32 (*calibParam)[MAX_PARAM_NUM_PER_MODULE];
    bool dcSideSwitch[PCS_NUM];
    QList<unsigned> checkList;
    bool isEmergency;

    QByteArray logText[LOG_MAX_PAGE];
    QByteArray log2Text[LOG_MAX_PAGE];

    QMutex MapMutex;
    QMutex HeartMutex;
    QMap<int, int> wnMarks;             // mark, module
    QMap<int, int> wnMarksMute;         // <wnMarks> stored when mute button clicked
    QMap<int,int> fault;
    QMap<int,int> fault_bak;

    QMap<int,int> warning;
    QMap<int,int> warning_bak;

    int get_m_page();
    /*QString* getPtr(QString flag,int id,int page);*/
    int getModuleFaultValueByCode(int module, FAULT_CODE code);
    unsigned char getMdFtByF(unsigned module, unsigned flag);
    bool isMdFt(unsigned module);
    bool isMdFt(unsigned module, QList<int> filter);
    bool isMdFtExcept(unsigned module, QList<int> filter);
    bool isMdMainRelayOff(unsigned module);
    void handleBitInWarningSet(int n, int key, bool isExist);
    void logCanFrame(const can_frame & frame, const QString & prefix = "");

    // implement virtual function
    unsigned getN() const;
    int getChargeCurrentMax(unsigned n) const;
    int getDischargeCurrentMax(unsigned n) const;
    QString getWarningDesc(int id) const;
    QList<unsigned> getCheckList() const;
    void setCheckList(QList<unsigned> checkList);

protected:
    void changeEvent(QEvent *e);
    void timerEvent(QTimerEvent *e);
    void showEvent(QShowEvent *e);
    void closeEvent(QCloseEvent *e);
    bool eventFilter(QObject *obj, QEvent *e);

private:
    Ui::MainWindow *ui;
    SystemTypeDialog *systemTypeDialog;
    FactoryDialog *factoryDialog;
    PortMgrDialog *portMgrDialog;
    TCPIP::network net;
    TCPIP::network net_2;
    Mykeyboard *myKeyboard;
    BgWorker *bgWorker[11];
    MyModel *myModel_v;
    MyModel *myModel_t;
    QGroupBox *groupBoxRunningParam[8];
    ExampleKbdHandler kbdHandler;
    HisRecordModel *hisRecordModel;

    bool mute;
    int logType;
    int transparency;

    bool sendGlobal;
    int sendTime;
    int sendPage;
    QString sendFlag;

    int tableRow;
    int faultBit;
    int warningBit;

    uint m_page;
    int p_page;
    uint para_page;

    int initOprtParamPage;
    int initWarnParamPage;
    int initClbrParamPage;

    int timerID_0;
    int timerID_1;
    int timerID_1_8;
    int timerID_2;
    int timerID_5;
    int timerID_10;
    int timerID_15;
	int timerID_60;
    int timerID_X;
    int timerID_XX;

    Config *config;
    Config *config_others;
    QLabel *date;
    QMovie *movie;
    QMovie *movie1;
    QMovie *movie2;
    QMovie *movie3;
    QMovie *movie4;
    CustomLabel *fs;                    // fulfill screen
    FileCopier *copier;
    QThread *copyThread;
    ParamLoader *paramLoader;

    QDir appDir;
    QString storagePath;
    QStringList updateFiles;

    int nCopied;
    int nTotal;
    QQueue<QFileInfo> dirQueue;
    QQueue<QFileInfo> fileQueue;
    bool extDiskUsingFlag;

    QString getFaultInfo(int code,int flag = 0);
    QString getWarningInfo(int code,int flag = 0);
    void progressChanged(int progress);

    void init();
    void initModbus();
    void initTablePeripheral();
    void createThread();
    void connectSignal();
    void hideWidget(bool hide);
    void countQuantity();
    void saveQuantity();
    void clearModuleData(unsigned n);
    bool clearModuleWarning(unsigned n);
    bool isMdFtFilter(unsigned module, QList<int> filter, bool containFilter);

    void checkPara();
    void faultReport();
    /*void viewPara(QGroupBox *box);*/
    void viewPara(QStackedWidget *stackedWidget, ParamLoader::ParamType pType);
    void showFault();
    void viewPreChargeRelay();
    /*QString modifyPara(QString flag,int id,QGroupBox *box);*/
    bool isDataDirNameValid(const QString &name);
    bool isDataFileNameValid(const QString &name);
    int getDataFileNum(const QString &path);
    quint64 getDirSize(const QString &path);
    quint64 getFilesInDirSize(const QString &path, const QStringList &nameFilters);
    QString improveReadability(const quint64 &size);
    bool externalStorageDetect();
    void callSystemSync();
    void logWarningItem(int coordinate, bool isTrigger);
    bool loadParamPage(const QString &configPath);
    void parseRunningParams(QXmlStreamReader &reader);
    void parseWarningParams(QXmlStreamReader &reader);
    void parseCalibrationParams(QXmlStreamReader &reader);
    void installKeyPad(QLineEdit *lineEdit);
    void installKeyPad(QWidget *container);
    void installTouchScroll(QTableView *tableView);
    const QDateTime buildDateTime();
    void setBuzzer(bool isOn);
    void checkBuzzer();

    template <typename Key, typename T>
    bool hasKeyValue(const QMap<Key, T> & map, const Key & key, const T & value);

#ifdef FONT_POINT_SIZE_TEST
    int font_point_size_inc;
#endif

#if (defined(WEIQIAN) || defined(WZD)) && !defined(FONT_POINT_SIZE_TEST)
    void calibrateFontSize(int increment);
#endif

signals:
    void send2BMS_Signal();
    void send2PCS_Signal();
    void send2EMS_Signal();
    void feedBack2BMS_Signal();
    void checkWarning_Signal();
    void modbusDataMap_Signal();

    void readLog_Signal();
    void saveData_Signal();

    void copyData_Signal(QString);
    void updateMCU_Signal(QString, int type);
    void logout();

    void startCopyRsquested();
    void cancelCopuRequested();

public slots:
    void switchPage();
    void showMsg(QString text, int timeout = 2000);
    void freshUI();
    void checkCOMM();
    void viewSOC(int id,uint16_t soc);
    void viewState(int id);
    void viewFault(int id);
    void viewVC(int id);
    void viewSystem();
    void viewModule();
    void viewPack();

    void viewLog();
    void viewSysPara();
    void readPara(QString flag,int dest);

    void paraACK(int id,int flag = 0);
    void saveSysPara();
    void changeBacklight();

    void processData();
    void processDone();
    void openFile();
    void do_update();
    void updatingMCU(int);
    void reboot();
    void selectAllPara();
    void loginByUser(int type);
    void readDigitalInput(quint8 addr, quint8 len, QBitArray bitArray);
    void readDigitalOutput(quint8 addr, quint8 len, QBitArray bitArray);
    void readPeripheral(int type, int index, int result);
    void doSendFactoryCalib(int n, int value);
    void updateCopyProgress(double percent);
    void copyFinishSlot();
    void errorHandleSlot();

private slots:
    void moduleChange();
    void packChange();
    void peripheralChange();
    void paramClusterChanged();
    void paramPageFlip();
    void paramPageChanged(int index);
    void logChange();
    void logNumChange(QString num);
    void showFactoryDialog();

    void muteClicked();
    void sendRelay();
    void sendCommand();
    void startBalance();

    void initPara(QString flag,int dest);
    void viewPara();
    void sendPara();
    /*void savePara();*/

    quint64 checkDisk();
    void on_balancePackID_currentIndexChanged(int index);
    void on_easterEggLabel_triggered();
    void on_easterEggLabel4RealSoc_triggered();
    void on_easterEggLabelMaintenance_triggered();
    void on_labelVersion_clicked();
    void on_pushButtonLock_clicked();
    void on_pushButtonDisplayOff_clicked();
    void displayRestore();
    void doDataDirCopy();
    void dataDirCpyWrapper();
    void doParallelOperation();
};


#endif // MAINWINDOW_H
