#ifndef LOGIN_H
#define LOGIN_H

#include <QDialog>
#include <QDebug>
#include <QListView>
#include "mainwindow.h"
#include "bms.h"

#define LIMIT_LOGIN_TIMES 5;
#define LOGIN_PERIOD 30;

namespace Ui {
    class Login;
}

class Login : public QDialog {
    Q_OBJECT
public:
    Login(QWidget *parent = 0);
    ~Login();    
    int userID;

protected:
    void changeEvent(QEvent *e);
    void timerEvent(QTimerEvent *e);

private:
    Ui::Login *ui;
    QTranslator *tran;
    Mykeyboard *myKeyboard;
    int remainTimes;
    int timerID;
    int lockTimerID;
    int lockTime;

#if (defined(WEIQIAN) || defined(WZD)) && !defined(FONT_POINT_SIZE_TEST)
    void calibrateFontSize(int increment);
#endif

public slots:
    void LogIn();
    void LanguageChange(int);
    void clear();
};

#endif // LOGIN_H
