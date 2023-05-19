#include "login.h"
#include "ui_login.h"

Login::Login(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Login)
{    
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);
    setWindowModality(Qt::WindowModal);

#if (defined(WEIQIAN) || defined(WZD)) && !defined(FONT_POINT_SIZE_TEST)
    calibrateFontSize(-5);
#endif

    ui->language->insertItem(0,QString("English"));
    ui->language->setCurrentIndex(0);
    ui->user->setView(new QListView);
    //load translator
    tran = new QTranslator(this);

    if(tran->load("zh_CN", ":/translation"))
    {
        qApp->installTranslator(tran);

        ui->language->insertItem(1,QString::fromUtf8("中文"));
        ui->language->setCurrentIndex(1);        
    }

    //password keyboard
    myKeyboard = new Mykeyboard(this, QLineEdit::Password);

    //hide password
    ui->password->setEchoMode(QLineEdit::Password);
    ui->password->installEventFilter(myKeyboard);

    //software version
    ui->version->setText(VERSION);

    //login init
    remainTimes = LIMIT_LOGIN_TIMES;
    lockTime = LOGIN_PERIOD;
    timerID = startTimer(30000);

    connect(ui->loginButton,SIGNAL(clicked()),this,SLOT(LogIn()));
    connect(ui->language,SIGNAL(currentIndexChanged(int)),this,SLOT(LanguageChange(int)));

}

Login::~Login()
{
    delete ui;
}

#if (defined(WEIQIAN) || defined(WZD)) && !defined(FONT_POINT_SIZE_TEST)
void Login::calibrateFontSize(int increment)
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

void Login::LogIn()
{

    if(ui->user->currentIndex() == 0 && ui->password->text() == "123456")
    {
        clear();
        finished(0);
    }
    else if(ui->user->currentIndex() == 1 && 0 == ui->password->text().compare(QDateTime::currentDateTime().toString("MMddhh")))
    {
        clear();
        finished(1);
    }
    else
    {        
        remainTimes--;
        ui->password->clear();

        if(remainTimes > 0)
        {
            ui->password->setFocus();
            ui->infoLable->setText(tr("Password incorrect!Remain %1 times!").arg(remainTimes));
        }
        else
        {          
            ui->loginButton->setEnabled(false);
            lockTimerID = startTimer(1000);
            ui->infoLable->setText(tr("Locked!Please retry later!"));
        }
    }

}

void Login::clear()
{
    ui->user->setCurrentIndex(0);
    ui->password->clear();

    remainTimes = LIMIT_LOGIN_TIMES;
    lockTime = LOGIN_PERIOD;
}

void Login::timerEvent(QTimerEvent *e)
{

    if(e->timerId() == timerID)       
    {
        remainTimes = LIMIT_LOGIN_TIMES;
        if(!(ui->infoLable->text() == tr("Locked!Please retry later!")))
        {
            ui->infoLable->setText("");
        }
    }
    else if(e->timerId() == lockTimerID)
    {
        lockTime--;
        ui->lockTime->setText(QString("%1 S").arg(lockTime));
        if(!lockTime)
        {
            killTimer(lockTimerID);
            lockTime = LOGIN_PERIOD;

            ui->lockTime->setText("");
            ui->infoLable->setText("");      
            ui->loginButton->setEnabled(true);
        }
    }

}

void Login::LanguageChange(int index)
{
    if(index == 0)
        qApp->removeTranslator(tran);
    else if(index == 1)
        qApp->installTranslator(tran);    
}

void Login::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
