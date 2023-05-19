#ifndef FACTORYDIALOG_H
#define FACTORYDIALOG_H

#include <QtGui>
#include <QDebug>
#include <QTimer>
#include "callback.h"
#include "mykeyboard.h"
#include "hmi.h"
#include "ExtMsgType.h"

enum FactoryDialogPage {
    CoefCali,
    DigitalIO,
    RmtSigl,
    DbgMsgType,
    Maintenance
};

namespace Ui {
class FactoryDialog;
}

class FactoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FactoryDialog(Callback *callback = NULL, QWidget *parent = NULL);
    ~FactoryDialog();
    void showPage(int i);

public slots:
    void readDigitalInput(quint8 addr, quint8 len, QBitArray bitArray);
    void readDigitalOutput(quint8 addr, quint8 len, QBitArray bitArray);

signals:
    void sendFactoryCalib(int n, int value);

private:
    Ui::FactoryDialog *ui;

    Callback *cb;
    HMI *hmi;
    QTimer *timer;
    QLabel *c[MAX_MODULE_NUM];
    QLabel *d[MAX_MODULE_NUM];
    QCheckBox *b[MAX_MODULE_NUM];
    QCheckBox *checkBoxIn[MAX_INPUT_NUM];
    QCheckBox *checkBoxOut[MAX_OUTPUT_NUM];
    Mykeyboard *myKeyboard;
    QMap<ExtMsgType, QString> mapMsgTypeName;

    void init();
    void initPageCoefCali();
    void initPageDigitalIO();
    void initPageRmtSigl();
    void initPageDbgMsgType();
    void initPageMaintenance();

private slots:
    void factoryCalib();
    void doQuery();
    void dbgMsgTypeToggled(bool);
    void on_pushButtonMinus_clicked();
    void on_pushButtonPlus_clicked();
    void on_pushButtonRmtSiglSet_clicked();
    void on_checkBoxAll_stateChanged(int state);
    void on_pushButtonLoad_clicked();
    void on_pushButtonCommit_clicked();
};

#endif // FACTORYDIALOG_H
