#ifndef PORTMGRDIALOG_H
#define PORTMGRDIALOG_H

#include <QDialog>
#include "animationwidget.h"
#include "tumbler.h"
#include "config.h"
#include "bms.h"

class PortMgrDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PortMgrDialog(QWidget *parent = NULL);
    ~PortMgrDialog();

signals:

private:
    AnimationWidget *slideWidget;
    QPushButton *pushButtonPrev;
    QPushButton *pushButtonNext;

    QStringList comPortList;
    int currentPage;
    const int pageNum = 2;
    Config *conf;

    QString getPortFunc(int page);
    int getPortNum(int page);

private slots:
    void pageUp();
    void pageDown();
    void animUnlock();
};

#endif // PORTMGRDIALOG_H
