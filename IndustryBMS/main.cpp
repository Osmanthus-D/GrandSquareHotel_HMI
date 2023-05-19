/* Copyright (C) 2019 Xinyi Power CO.,LTD
 *
 * This HMI program is used in Industry energy
 * storage BMS system.
 *
 * Author:  Zongpei Liu <liuz-zp@163.com>
 *
 * Create Date: 2019.04.30
 */

#include <QApplication>
#include <QTranslator>
#include <QString>
#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QProcess>
#include <QSharedMemory>
#include <stdio.h>
#include <stdlib.h>
#include <QTextCodec>
#include <QWSServer>
#include <QSplashScreen>
//#include "myinputpanelcontext.h"
#include "mainwindow.h"
#include "qembededplastiquestyle.h"
#include "ExtMsgType.h"

unsigned ExtMsgFlag::flag = UINT_MAX;

void writeLog(QFile& log,QFile& logBak,QString msg)
{
    if(log.size() > LOG_FILE_MAX_SIZE)
    {
        log.seek(0);

        logBak.resize(0);
        logBak.write(log.readAll());
        logBak.flush();

        log.resize(0);
    }

    log.seek(log.size()-1);
    log.write(msg.toLatin1().data());
    log.flush();

}

void message_handler(QtMsgType type, const char *msg)
{
    QString message = "";

    message += QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ");
    message += QString(msg);

    if(!message.endsWith("\n"))message += "\n";

    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "%s", qPrintable(message));
        fflush(stderr);
        return;
    case QtWarningMsg:
        writeLog(g_log,g_logBak,/*QString("0x%1").arg((int)QThread::currentThreadId(), 8, 16, QChar('0')) + */message);
        break;
    case QtCriticalMsg:
        writeLog(g_log2,g_log2Bak,message);
        break;
    case QtFatalMsg:
        fprintf(stderr, "%s", qPrintable(message));
        fflush(stderr);
        abort();
    default:
        if (0 > type ||  32 < type + 1 || !ExtMsgFlag::bit(type)) {
            break;
        }

        fprintf(stderr, "xDebug %s", qPrintable(message));
        fflush(stderr);
    }

}

int main(int argc, char *argv[])
{
    int n = 0;
    bool ok = false;
    MainWindow *m = NULL;

    QApplication a(argc, argv);
    qApp->setStyle(new QEmbededPlastiqueStyle);

//    a.setOverrideCursor(Qt::BlankCursor);
    QWSServer::setCursorVisible(true);
    a.setStyleSheet("*{outline:0px;}");
//    a.setFont(QFont("Utopia"));
//    {
//        int order = 1;
//        QFontDatabase db;
//        foreach (const QString &family, db.families(QFontDatabase::SimplifiedChinese)) {
//            qDebug() << order++ << family;
//        }
//    }
#ifdef WEIQIAN
    QFont f = a.font();
#if 0
    QString family = QString("%1/simhei.ttc").arg(qApp->applicationDirPath());
#else
    QString family = QString("%1/msyh.ttc").arg(qApp->applicationDirPath());
    int id = QFontDatabase::addApplicationFont(QString("%1/msyh.ttc").arg(qApp->applicationDirPath()));
    qDebug() << "fontid" << id;

    if (0 < id + 1) {
        family = QFontDatabase::applicationFontFamilies(id).at(0);
    }
#endif
    f.setPointSize(10);
    f.setFamily(family);
    a.setFont(f);
#endif

//    MyInputPanelContext *ic = new MyInputPanelContext;
//    a.setInputContext(ic);
#if defined(YCTEK) || defined(WEIQIAN)
    QSharedMemory tmp;
    tmp.setKey(QString("main_window"));
    if(tmp.attach())
    {
        tmp.detach();
    }
#endif

    QSharedMemory shared_memory;
    shared_memory.setKey(QString("main_window"));
    if(shared_memory.attach())
    {
        qDebug("Application is already running!");
        return 0;
    }
    else
    {
        // splash
        QSplashScreen *splash = new QSplashScreen;
        splash->setPixmap(QPixmap(":/image/splash.gif"));
        splash->showMessage(QString("Loading..."),Qt::AlignCenter);
        splash->setFont(QFont("Microsoft YaHei UI", 13));
//        splash->show();
//        a.processEvents();
//        qApp->processEvents();

        // create shared memory
        shared_memory.create(1);

        // log directory
        QDir l(LOG_PATH);
        if(!l.exists())l.mkpath(l.absolutePath());

        // log file
        bool ret1 = g_log.open(QIODevice::ReadWrite | QIODevice::Append);
        bool ret2 = g_logBak.open(QIODevice::ReadWrite | QIODevice::Append);
        qDebug()<<"Open log File "<<ret1<<",Open logBak File "<<ret2;
        ret1 = g_log2.open(QIODevice::ReadWrite | QIODevice::Append);
        ret2 = g_log2Bak.open(QIODevice::ReadWrite | QIODevice::Append);
        qDebug()<<"Open log2 File "<<ret1<<",Open log2Bak File "<<ret2;
        qInstallMsgHandler(message_handler);

        // init module num
        Config *conf = new Config(CONFIG_PATH_OTHERS);
        n = conf->value("moduleNum", DEFAULT_MODULE_NUM).toInt(&ok);
        delete conf;

        Login *login = new Login;

        if(ok && n > 0)
        {
            m = new MainWindow(0, n);
        }
        else {
            qDebug() << "module num invalid, user default value.";
            m = new MainWindow;
        }

//        if(login->exec() == QDialog::Accepted)
//        {
//            delete login;
//            m->userID = login->userID;
//            m->showFullScreen();

//            return a.exec();
//        }
//        else
//            return 0;
        login->showFullScreen();
        usleep(1000 * 1000);
//        splash->finish(login);
//        splash->hide();
        qDebug() << "splash screen finished.";
        QObject::connect(login, SIGNAL(finished(int)), m, SLOT(loginByUser(int)));
//        QObject::connect(m, SIGNAL(logout()), login, SLOT(raise()));
        QObject::connect(m, SIGNAL(logout()), splash, SLOT(close()));

        return a.exec();
    }

}
