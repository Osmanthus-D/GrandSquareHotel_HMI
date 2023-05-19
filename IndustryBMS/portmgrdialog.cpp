#include "portmgrdialog.h"

PortMgrDialog::PortMgrDialog(QWidget *parent) : QDialog(parent)
  , currentPage(0)
{
    QList<QWidget *> pageList;
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *buttomLayout = new QHBoxLayout();

    pageList.clear();
    comPortList.clear();

    conf = new Config(CONFIG_PATH_OTHERS);
    conf->load_others();

    comPortList << "COM1" << "COM2" << "COM3";
    for (int i = 0; i < pageNum; ++i) {
        QWidget *page = new QWidget(this);
        QVBoxLayout *pageLayout = new QVBoxLayout(page);
        QLabel *labelPortFunc = new QLabel(this);
        Tumbler *tumblerPort = new Tumbler(this);

        page->setFixedSize(320, 240);
        labelPortFunc->setText(getPortFunc(i));
        tumblerPort->setObjectName(QString("tumblerPort%1").arg(i));
        tumblerPort->setListValue(comPortList);
        pageLayout->setSpacing(0);
        pageLayout->addWidget(labelPortFunc);
        pageLayout->addWidget(tumblerPort);

        tumblerPort->setCurrentIndex(getPortNum(i) - 1);
        labelPortFunc->setStyleSheet(QString("background-color: rgb(%1, %2, %3)")
                            .arg(tumblerPort->getBackground().red())
                            .arg(tumblerPort->getBackground().green())
                            .arg(tumblerPort->getBackground().blue()));

        page->setStyleSheet(QString("color: rgb(%1, %2, %3)")
                            .arg(tumblerPort->getTextColor().red())
                            .arg(tumblerPort->getTextColor().green())
                            .arg(tumblerPort->getTextColor().blue()));

        pageList << page;
    }

    slideWidget = new AnimationWidget(pageList, "slide");
    slideWidget->raise(0);
    slideWidget->setFixedSize(320, 240);

    pushButtonPrev = new QPushButton(this);
    pushButtonNext = new QPushButton(this);
    pushButtonPrev->setText("Prev");
    pushButtonNext->setText("Next");
    buttomLayout->addStretch();
    buttomLayout->addWidget(pushButtonPrev);
    buttomLayout->addWidget(pushButtonNext);
    mainLayout->addWidget(slideWidget);
    mainLayout->addLayout(buttomLayout);

    connect(pushButtonPrev, SIGNAL(clicked()), this, SLOT(pageUp()));
    connect(pushButtonNext, SIGNAL(clicked()), this, SLOT(pageDown()));
    connect(slideWidget, SIGNAL(animationFinshed()), this, SLOT(animUnlock()));
}

PortMgrDialog::~PortMgrDialog()
{
    delete conf;
}

QString PortMgrDialog::getPortFunc(int page)
{
    QString portFunc = "undefind:";

    switch (page) {
    case 0:
        portFunc = "Comm with PCS device:";
        break;
    case 1:
        portFunc = "Comm with peripherals:";
        break;
    default:
        break;
    }

    return portFunc;
}

int PortMgrDialog::getPortNum(int page)
{
    int portNum = -1;

    switch (page) {
    case 0:
        portNum = conf->pcsDevComPort;
        break;
    case 1:
        portNum = conf->periphComPort;
        break;
    default:
        break;
    }

    return portNum;
}
void PortMgrDialog::pageUp()
{
    pushButtonNext->setText("Next");
    if (0 > currentPage - 1) {
        return;
    }

    if (slideWidget->getAnimation()->tryLock()) {
        slideWidget->animationShow(--currentPage, AnimationWidget::ANIMATION_LEFT);
    }
}

void PortMgrDialog::pageDown()
{
    if (0 == pushButtonNext->text().compare("Apply")) {
        Tumbler *tumbler = findChild<Tumbler *>("tumblerPort0");
        if (tumbler) {
            conf->setValue("pcsDevComPort", tumbler->getCurrentValue().remove("COM").toInt());
        }
        tumbler = findChild<Tumbler *>("tumblerPort1");
        if (tumbler) {
            conf->setValue("periphComPort", tumbler->getCurrentValue().remove("COM").toInt());
        }

        done(0);
        qDebug() << __func__ << "*** after done() called";
        slideWidget->animationShow(0, AnimationWidget::ANIMATION_LEFT);
        return;
    }

    if (2 > pageNum - currentPage) {
        return;
    }

    QStringList portList;
    Tumbler *tumbler = findChild<Tumbler *>(QString("tumblerPort%1").arg(currentPage));
    if (tumbler) {
        portList = tumbler->getListValue();
        portList.removeAt(tumbler->getCurrentIndex());
    }

    tumbler = findChild<Tumbler *>(QString("tumblerPort%1").arg(currentPage + 1));
    if (tumbler) {
        tumbler->setListValue(portList);
    }

    if (slideWidget->getAnimation()->tryLock()) {
        slideWidget->animationShow(++currentPage, AnimationWidget::ANIMATION_RIGHT);
    }

    if (1 == pageNum - currentPage) {
        pushButtonNext->setText("Apply");
    }
}

void PortMgrDialog::animUnlock()
{
    slideWidget->getAnimation()->unlock();
}
