#ifndef UIHELPER_H
#define UIHELPER_H

#include <QtGui>
#include <QDebug>
#include "customlabel.h"
#include "bms.h"

typedef struct TopoGraphElement_t
{
    QLabel *labelVoltage[MAX_MODULE_NUM];
    QLabel *labelCurrent[MAX_MODULE_NUM];
    QLabel *labelModule[MAX_MODULE_NUM];
    QFrame *lineSplitRed[MAX_MODULE_NUM];
    CustomLabel *labelM[MAX_MODULE_NUM];
    QWidget *widgetBattery[MAX_MODULE_NUM];
    QLabel *labelWarn[MAX_MODULE_NUM];
    QLabel *labelCanComm[MAX_MODULE_NUM];
    QLabel *labelSOC[MAX_MODULE_NUM];
    QLabel *labelQuantity[MAX_MODULE_NUM];
} TopoGraphElement;

class UiHelper
{
public:
    UiHelper();
    ~UiHelper();

protected:
    const QPixmap pixmapChecked;
    const QPixmap pixmapDownload;
    const QPixmap pixmapUnchecked;
    const QPixmap pixmapError;

    QLabel *labelAck[MAX_MODULE_NUM];
    TopoGraphElement *t;

    void genTopoGraph(QWidget *widget, int n);
    void genGroupBoxAck(QGroupBox *groupBox, int n);
    void updateBackgroundBySOC(int i, quint16 soc);
    int getParallelReqIndex(QObject *obj);
};

#endif // UIHELPER_H
