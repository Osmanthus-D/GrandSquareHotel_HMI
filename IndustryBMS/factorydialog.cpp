#include "factorydialog.h"
#include "ui_factorydialog.h"
#include "MCU801A.h"
#include "mainwindow.h"

FactoryDialog::FactoryDialog(Callback *callback, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FactoryDialog)
    , cb(callback)
{
    ui->setupUi(this);

    init();
}

FactoryDialog::~FactoryDialog()
{
    delete ui;
    delete hmi;
}

void FactoryDialog::init()
{
    myKeyboard = new Mykeyboard(this);

    mapMsgTypeName.insert(EmsMsg, tr("EMS Device"));
    mapMsgTypeName.insert(PcsMsg, tr("PCS Device"));
    mapMsgTypeName.insert(PerMsg, tr("Peripheral"));

    initPageCoefCali();
    initPageDigitalIO();
    initPageRmtSigl();
    initPageDbgMsgType();
    initPageMaintenance();
}

void FactoryDialog::initPageCoefCali()
{
    foreach (QPushButton *button, ui->groupBoxCalibration->findChildren<QPushButton *>()) {
        connect(button, SIGNAL(clicked()), this, SLOT(factoryCalib()));
    }
}

void FactoryDialog::initPageDigitalIO()
{
    int n = 30;
    QGridLayout *gridLayout = NULL;
    QGroupBox *groupBox = ui->groupBoxDigitalOutput;

    hmi = new HMI;
    timer = new QTimer(this);
    if (MAX_INPUT_NUM < n) {
        qDebug("num [%d] is larger than maximum module num %d", n, MAX_INPUT_NUM);
        return;
    }

    if (NULL != groupBox && NULL != groupBox->layout()) {
        qDebug("widget [%s] had have a layout.", qPrintable(groupBox->objectName()));
        return;
    }

    gridLayout = new QGridLayout(groupBox);
    gridLayout->setSpacing(2);
    gridLayout->setContentsMargins(2, 2, 2, 2);

    for (int i = 0; i < MAX_OUTPUT_NUM; ++i) {
        checkBoxOut[i] = new QCheckBox(groupBox);
        checkBoxOut[i]->setObjectName(QString("checkBoxOut_%1").arg(i + 1));
        checkBoxOut[i]->setMaximumSize(QSize(20, 20));
        checkBoxOut[i]->setEnabled(i < n);

        gridLayout->addWidget(checkBoxOut[i], i / MAX_CHECKBOX_NUM_IN_ROW, i % MAX_CHECKBOX_NUM_IN_ROW);
    }

    groupBox = ui->groupBoxDigitalInput;
    if (NULL != groupBox && NULL != groupBox->layout()) {
        qDebug("widget [%s] had have a layout.", qPrintable(groupBox->objectName()));
        return;
    }

    gridLayout = new QGridLayout(groupBox);
    gridLayout->setSpacing(2);
    gridLayout->setContentsMargins(2, 2, 2, 2);

    for (int i = 0; i < MAX_INPUT_NUM; ++i) {
        checkBoxIn[i] = new QCheckBox(groupBox);
        checkBoxIn[i]->setObjectName(QString("checkBoxIn_%1").arg(i + 1));
        checkBoxIn[i]->setMaximumSize(QSize(20, 20));
        checkBoxIn[i]->setEnabled(i < n);

        gridLayout->addWidget(checkBoxIn[i], i / MAX_CHECKBOX_NUM_IN_ROW, i % MAX_CHECKBOX_NUM_IN_ROW);
    }

    ui->stackedWidget->setCurrentIndex(1);

//    connect(hmi, SIGNAL(readyReadDigiInput(quint8, quint8, quint8)), this, SLOT(readDigitalInput(quint8, quint8, quint8)));
//    connect(hmi, SIGNAL(readyReadDigiOutput(quint8, quint8, quint8)), this, SLOT(readDigitalOutput(quint8, quint8, quint8)));
//    connect(timer, SIGNAL(timeout()), this, SLOT(doQuery()));

//    timer->start(5000);
}

void FactoryDialog::initPageRmtSigl()
{
    ui->lineEditRmtSiglAddr->installEventFilter(myKeyboard);
}

void FactoryDialog::initPageDbgMsgType()
{
    QMapIterator<ExtMsgType, QString> i(mapMsgTypeName);
    QVBoxLayout *vBoxLayout = new QVBoxLayout(ui->groupBoxDbgMsgTyp);

    while (i.hasNext()) {
        i.next();
        QCheckBox *checkBox = new QCheckBox(i.value(), ui->groupBoxDbgMsgTyp);
        checkBox->setChecked(ExtMsgFlag::bit(i.key()));
        vBoxLayout->addWidget(checkBox);
        connect(checkBox, SIGNAL(toggled(bool)), this, SLOT(dbgMsgTypeToggled(bool)));
    }
    vBoxLayout->addStretch();
}

void FactoryDialog::initPageMaintenance()
{
    unsigned N = 0;
    unsigned M = PCS_NUM;
    QWidget *widget = NULL;
    QGridLayout *gridLayout = NULL;

    if (NULL == cb) {
        return;
    }

    N = cb->getN();
//    M = cb->getM();

    if (0 == N || 0 == M) {
        return;
    }

    widget = new QWidget();
    gridLayout = new QGridLayout(widget);
    for (unsigned i = 0; i < cb->getN(); ++i) {
        QLabel *n = new QLabel(widget);
        QLabel *m = new QLabel(widget);
        b[i] = new QCheckBox(widget);
        c[i] = new QLabel(widget);
        d[i] = new QLabel(widget);

        n->setNum((int)(i + 1));
        if (0 != N / M) m->setNum((int)(i / (N / M) + 1));
        c[i]->setNum(0);
        d[i]->setNum(0);

        gridLayout->addWidget(n, i, 0);
        gridLayout->addWidget(m, i, 1);
        gridLayout->addWidget(c[i], i, 2);
        gridLayout->addWidget(d[i], i, 3);
        gridLayout->addWidget(b[i], i, 4);
    }

    widget->setLayout(gridLayout);
    ui->scrollAreaMaintenance->setWidget(widget);
}

void FactoryDialog::factoryCalib()
{
    bool ok = false;
    int n = 0;
    QStringList list;
    QLineEdit *edit = NULL;
    QPushButton *button = qobject_cast<QPushButton *>(sender());
    QGroupBox *box = ui->groupBoxCalibration;

    if (NULL == button) {
        return;
    }

    list = button->objectName().split('_');
    n = list.at(1).toInt(&ok);

    if (ok) {
        if (0 < n && 7 > n) {
            edit = box->findChild<QLineEdit *>(QString("lineEditVoltage_%1").arg(n));
        } else if (6 < n && 9 > n) {
            edit = box->findChild<QLineEdit *>(QString("lineEditCurrent_%1").arg(n));
        }

        if (NULL == edit) {
            return;
        }

        int value = edit->text().toInt(&ok);
        if (ok) {
            emit sendFactoryCalib(n, value);
        }
    }
}

void FactoryDialog::doQuery()
{
    hmi->getIO(false, 0, 18);
    hmi->getIO(true, 0, 18);
}

void FactoryDialog::readDigitalInput(quint8 addr, quint8 len, QBitArray bitArray)
{
    ui->groupBoxDigitalInput->setTitle(QString("Digital Input (%1/%2)")
                                       .arg(bitArray.size()).arg(len));
    if (0 == bitArray.size() || MAX_INPUT_NUM < addr + bitArray.size()) {
        return;
    }

    for (int i = 0; i < bitArray.size(); ++i) {
        checkBoxIn[addr + i]->setChecked(bitArray.at(i));
    }
}

void FactoryDialog::readDigitalOutput(quint8 addr, quint8 len, QBitArray bitArray)
{
    ui->groupBoxDigitalOutput->setTitle(QString("Digital Output (%1/%2)")
                                        .arg(bitArray.size()).arg(len));
    if (0 == bitArray.size() || MAX_OUTPUT_NUM < addr + bitArray.size()) {
        return;
    }

    for (int i = 0; i < bitArray.size(); ++i) {
        checkBoxOut[addr +i]->setChecked(bitArray.at(i));
    }
}

void FactoryDialog::showPage(int i)
{
    // set title
    ui->stackedWidget->setCurrentIndex(i);
    show();
}

void FactoryDialog::on_pushButtonMinus_clicked()
{
    bool ok = false;
    int addr = ui->lineEditRmtSiglAddr->text().toInt(&ok);

    if (ok) {
        if (0 < addr) {
            ui->lineEditRmtSiglAddr->setText(QString::number(--addr));
        } else {
            ui->lineEditRmtSiglAddr->setText(QString::number(1000));
        }
    }
}

void FactoryDialog::on_pushButtonPlus_clicked()
{
    bool ok = false;
    int addr = ui->lineEditRmtSiglAddr->text().toInt(&ok);

    if (ok) {
        if (999 < addr) {
            ui->lineEditRmtSiglAddr->setText(QString::number(0));
        } else {
            ui->lineEditRmtSiglAddr->setText(QString::number(++addr));
        }
    }
}

void FactoryDialog::on_pushButtonRmtSiglSet_clicked()
{
    bool ok;
    int addr = ui->lineEditRmtSiglAddr->text().toInt(&ok);

    if (ok) {
        qDebug() << __func__ << addr << ui->comboBoxSignalValue->currentIndex();
        setyx(addr, (unsigned char)ui->comboBoxSignalValue->currentIndex());
    }
}

void FactoryDialog::dbgMsgTypeToggled(bool toggled)
{
    QCheckBox *checkBox = NULL;

    if (NULL == sender()) {
        return;
    }

    checkBox = qobject_cast<QCheckBox *>(sender());
    ExtMsgType i = mapMsgTypeName.key(checkBox->text());
    if (toggled) {
        ExtMsgFlag::toggleBit(i);
    } else {
        ExtMsgFlag::clearBit(i);
    }
    qDebug() << __func__ << i << (mapMsgTypeName[i])
             << toggled << QString::number(ExtMsgFlag::getFlag(), 2) << ExtMsgFlag::bit(i);
}

void FactoryDialog::on_checkBoxAll_stateChanged(int state)
{
    QList<QCheckBox *> checkBoxes = ui->scrollAreaMaintenance->findChildren<QCheckBox *>();

    foreach (QCheckBox *checkBox, checkBoxes) {
        checkBox->setChecked((Qt::CheckState)state);
    }
}

void FactoryDialog::on_pushButtonLoad_clicked()
{
    unsigned N;

    if (NULL == cb) {
        return;
    }

    QList<unsigned> checkList = cb->getCheckList();
    N = cb->getN();
    for (unsigned i = 0; i < N; ++i) {
        b[i]->setChecked(checkList.contains(i));
        c[i]->setNum(cb->getChargeCurrentMax(i));
        d[i]->setNum(cb->getDischargeCurrentMax(i));
    }

    qDebug() << __func__;
}

void FactoryDialog::on_pushButtonCommit_clicked()
{
    unsigned N;
    QList<unsigned> checkList;

    if (NULL == cb) {
        return;
    }

    N = cb->getN();
    for (unsigned i = 0; i < N; ++i) {
        if (Qt::Checked == b[i]->checkState()) {
            checkList << i;
        }
    }

    if (1 != checkList.size() && 0 != cb->getN() - checkList.size()) {
        QMessageBox::information(this ,"Maintenance", "Only single selection support currently.");
        return;
    }

    cb->setCheckList(checkList);
}
