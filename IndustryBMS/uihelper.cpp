#include "uihelper.h"

UiHelper::UiHelper()
    : pixmapChecked(QPixmap(":/image/checked.png"))
    , pixmapDownload(QPixmap(":/image/down.png"))
    , pixmapUnchecked(QPixmap(":/image/unchecked.png"))
    , pixmapError(QPixmap(":/image/error.png"))
{
    t = new TopoGraphElement();
}

UiHelper::~UiHelper()
{
    delete t;
}

void UiHelper::genTopoGraph(QWidget *widget, int n)
{
    QHBoxLayout *hBoxLayout = NULL;

    if (NULL != widget && NULL != widget->layout()) {
        qDebug("widget [%s] had have a layout.", qPrintable(widget->objectName()));
        return;
    }

    if (MAX_MODULE_NUM < n) {
        qDebug("num [%d] is larger than maximum module num %d", n, MAX_MODULE_NUM);
        return;
    }

    hBoxLayout = new QHBoxLayout(widget);
    hBoxLayout->setSpacing(0);
    hBoxLayout->setMargin(0);

    for (int i = 0; i < n; ++i) {
        QVBoxLayout *verticalLayoutModule;
        QVBoxLayout *verticalLayoutHead;
        QVBoxLayout *verticalLayoutBody;
        QVBoxLayout *verticalLayoutM;
        QHBoxLayout *horizontalLayout_17;
        QVBoxLayout *verticalLayout_35;
        QHBoxLayout *horizontalLayoutMiddle;
        QVBoxLayout *verticalLayoutWarnAndCanComm;

        verticalLayoutModule = new QVBoxLayout;
        verticalLayoutModule->setContentsMargins(0, 0, 0, 0);
        verticalLayoutModule->setSpacing(6);
        verticalLayoutHead = new QVBoxLayout;
        verticalLayoutHead->setSpacing(6);
        t->labelVoltage[i] = new QLabel(widget);
    //    labelVoltage->setStyleSheet(QString::fromUtf8("background-color: rgb(170, 170, 0);"));
        t->labelVoltage[i]->setAlignment(Qt::AlignCenter);

        verticalLayoutHead->addWidget(t->labelVoltage[i]);

        t->labelCurrent[i] = new QLabel(widget);
        t->labelCurrent[i]->setAlignment(Qt::AlignCenter);

        verticalLayoutHead->addWidget(t->labelCurrent[i]);

        t->labelModule[i] = new QLabel(widget);
        t->labelModule[i]->setAlignment(Qt::AlignCenter);

        verticalLayoutHead->addWidget(t->labelModule[i]);


        verticalLayoutModule->addLayout(verticalLayoutHead);

        verticalLayoutBody = new QVBoxLayout;
        verticalLayoutBody->setSpacing(0);
        verticalLayoutM = new QVBoxLayout;
        verticalLayoutM->setSpacing(0);
        t->lineSplitRed[i] = new QFrame(widget);
        t->lineSplitRed[i]->setMinimumWidth(120);
        t->lineSplitRed[i]->setStyleSheet(QString::fromUtf8("border:3px solid rgb(249, 159, 159)"));
        t->lineSplitRed[i]->setFrameShape(QFrame::HLine);
        t->lineSplitRed[i]->setFrameShadow(QFrame::Sunken);
        t->lineSplitRed[i]->setMinimumHeight(6);

        verticalLayoutM->addWidget(t->lineSplitRed[i]);

        horizontalLayout_17 = new QHBoxLayout;
        horizontalLayout_17->addStretch();

        t->labelM[i] = new CustomLabel(widget);
        t->labelM[i]->setMaximumSize(QSize(50, 50));
        t->labelM[i]->setPixmap(QPixmap(QString::fromUtf8(":/image/disconnect.png")));
        t->labelM[i]->setScaledContents(true);

        horizontalLayout_17->addWidget(t->labelM[i]);
        horizontalLayout_17->addStretch();


        verticalLayoutM->addLayout(horizontalLayout_17);


        verticalLayoutBody->addLayout(verticalLayoutM);


        t->widgetBattery[i] = new QWidget(widget);
//        widgetBattery[i]->setMinimumSize(QSize(60, 128));
//        widgetBattery[i]->setMaximumSize(QSize(80, 128));
        t->widgetBattery[i]->setMinimumHeight(128);
        t->widgetBattery[i]->setStyleSheet(QString::fromUtf8("image: url(:/image/empty.png);"));
        verticalLayout_35 = new QVBoxLayout(t->widgetBattery[i]);

        verticalLayout_35->addStretch(2);

        horizontalLayoutMiddle = new QHBoxLayout;

        horizontalLayoutMiddle->addStretch();

        verticalLayoutWarnAndCanComm = new QVBoxLayout;
        t->labelWarn[i] = new QLabel(t->widgetBattery[i]);
        t->labelWarn[i]->setMaximumSize(QSize(24, 24));
        t->labelWarn[i]->setPixmap(QPixmap(QString::fromUtf8(":/image/warning.png")));
        t->labelWarn[i]->setScaledContents(true);

        verticalLayoutWarnAndCanComm->addWidget(t->labelWarn[i]);

        t->labelCanComm[i] = new QLabel(t->widgetBattery[i]);
        t->labelCanComm[i]->setMaximumSize(QSize(24, 24));
        t->labelCanComm[i]->setPixmap(QPixmap(QString::fromUtf8(":/image/power_disconnected_32px.png")));
        t->labelCanComm[i]->setScaledContents(true);

        verticalLayoutWarnAndCanComm->addWidget(t->labelCanComm[i]);


        horizontalLayoutMiddle->addLayout(verticalLayoutWarnAndCanComm);

        horizontalLayoutMiddle->addStretch();


        verticalLayout_35->addLayout(horizontalLayoutMiddle);

        verticalLayout_35->addStretch(1);

        t->labelSOC[i] = new QLabel(t->widgetBattery[i]);
        t->labelSOC[i]->setAlignment(Qt::AlignCenter);

        verticalLayout_35->addWidget(t->labelSOC[i]);

        verticalLayoutBody->addWidget(t->widgetBattery[i]);

        t->labelQuantity[i] = new QLabel(widget);
        t->labelQuantity[i]->setAlignment(Qt::AlignCenter);

        verticalLayoutBody->addWidget(t->labelQuantity[i]);


        verticalLayoutModule->addLayout(verticalLayoutBody);
        verticalLayoutModule->addStretch();

        t->labelVoltage[i]->setText("000.0 V");
        t->labelCurrent[i]->setText("000.0 A");
        t->labelModule[i]->setText(QObject::tr("Module %1").arg(i + 1));
        t->labelM[i]->setText(QString());
        t->labelWarn[i]->setText(QString());
        t->labelCanComm[i]->setText(QString());
        t->labelSOC[i]->setText("000.0 %");
        t->labelQuantity[i]->setText("000.0 kWh");

        hBoxLayout->addLayout(verticalLayoutModule);
    }
}

void UiHelper::genGroupBoxAck(QGroupBox *groupBox, int n)
{
    QGridLayout *gridLayout = NULL;

    if (NULL != groupBox && NULL != groupBox->layout()) {
        qDebug("widget [%s] had have a layout.", qPrintable(groupBox->objectName()));
        return;
    }

    if (MAX_MODULE_NUM < n) {
        qDebug("num [%d] is larger than maximum module num %d", n, MAX_MODULE_NUM);
        return;
    }

    gridLayout = new QGridLayout(groupBox);
    gridLayout->setSpacing(2);
    gridLayout->setContentsMargins(2, 2, 2, 2);

    for (int i = 0; i < MAX_MODULE_NUM; ++i) {
        labelAck[i] = new QLabel(groupBox);
        labelAck[i]->setObjectName(QString("ack_%1").arg(i + 1));
        labelAck[i]->setMaximumSize(QSize(20, 20));
        labelAck[i]->setPixmap(QPixmap(":/image/unchecked.png"));
        labelAck[i]->setScaledContents(true);
        labelAck[i]->setEnabled(i < n);

        gridLayout->addWidget(labelAck[i], i / MAX_CHECKBOX_NUM_IN_ROW, i % MAX_CHECKBOX_NUM_IN_ROW);
    }
}

void UiHelper::updateBackgroundBySOC(int i, quint16 soc)
{
    QString state;

    if (MAX_MODULE_NUM > i && 0 <= i) {
        if (0 >= soc) {
            state = "empty";
        }
        else if (0 < soc && 100 > soc) {
            state = "limit";
        }
        else if (100 <= soc && 350 > soc) {
            state = "low";
        }
        else if (350 <= soc && 600 > soc) {
            state = "middle";
        }
        else if (600 <= soc && 950 > soc) {
            state = "high";
        }
        else {
            state = "full";
        }

        if (NULL != t->widgetBattery[i]) {
            t->widgetBattery[i]->setStyleSheet(QString("image: url(:/image/%1.png);").arg(state));
        }
    }
}

int UiHelper::getParallelReqIndex(QObject *obj)
{
    int index = -1;

    if (NULL == obj) {
        return -1;
    }

    for (int i = 0; i < MAX_MODULE_NUM; ++i) {
        if (t->labelM[i] == obj) {
            index = i;
            break;
        }
    }

    return index;
}
