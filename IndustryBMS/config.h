#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QSettings>
#include <stdint.h>
#include <base.h>
#include "para.h"
#include "module.h"

class Config : public QSettings, Base
{
public:
    Config(const QString & fileName, int moduleNum = DEFAULT_MODULE_NUM,
           QObject * parent = 0, Format format = QSettings::IniFormat);

    int moduleNum;
    QString backlightLevel;
    QString canRate;
    QString dataSaveInterval;
    bool isRealSocEnabled;
    int systemType;
    int pcsDevComPort;
    int periphComPort;

    void load(Para *para);
    void load_others();

    /*QString getStrkey(QString type,int id);*/

};

#endif // CONFIG_H
