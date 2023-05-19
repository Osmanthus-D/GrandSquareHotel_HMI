#ifndef PARAMLOADER_H
#define PARAMLOADER_H

#include <QObject>
#include <QDebug>
#include <QFile>
#include <QMap>
#include <QFrame>
#include <QLineEdit>
#include <QCheckBox>
#include <QGridLayout>
#include <QBoxLayout>
#include <QGroupBox>
#include <QXmlStreamReader>
#include <QStackedWidget>
#include "switchbutton.h"
#include "mykeyboard.h"
#include "bitcalculator.h"
#include "para.h"
#include "bms.h"

class ParamLoader : public QObject
{
    Q_OBJECT

public:
    enum ParamTypeFlag {
        NoType,
        Operating,
        Warning,
        Calibration
    };
    Q_DECLARE_FLAGS(ParamType, ParamTypeFlag)

    enum ParamDisplayFlag {
        NoDisplay,
        Decimal,
        Hex,
        ZeroAndOne,
        MinusOneAndOne,
        OnlyOne
    };
    Q_DECLARE_FLAGS(ParamDisplay, ParamDisplayFlag)

    explicit ParamLoader(QObject *parent = NULL);
    explicit ParamLoader(const QString &configPath, QObject *parent = NULL);
    ~ParamLoader();

    QString getWnNameById(int id);
    int getWnLevelById(int id);
    bool getTypeByType(int id, ParamType type);
    int getPrecisionByType(int id, ParamType type);
    int getDispByType(int id, ParamType type);
    QList<int> getIdListByFlag(unsigned flag);
    static QString paramType2Char(ParamType type);
    static QString paramType2String(ParamType type);
    static QString paramType2Abbr(ParamType type);
    void setXmlFilePath(const QString &configPath);
    QString getXmlFilePath() const;

    bool load();
    bool load(QStackedWidget *stackedWidget, ParamType pType);

signals:

public slots:

private:
    QMap<int, QString> wnSet;
    QMap<int, int> wnLevel;
    QMap<unsigned, int> wnFlag;
    QMap<int, int> oprtPrcsMap;
    QMap<int, int> oprtTypeMap;
    QMap<int, int> oprtDispMap;
    QMap<int, int> warnPrcsMap;
    QMap<int, int> warnTypeMap;
    QMap<int, int> warnDispMap;
    QMap<int, int> clbrPrcsMap;
    QMap<int, int> clbrTypeMap;
    QMap<int, int> clbrDispMap;
    QFile xmlFile;
    QGroupBox *groupBoxRunningParam[8];
    Mykeyboard *myKeyboard;
    BitCalculator *bitCalculator;

    void parseWarningPage(QXmlStreamReader &reader);
    void parseWarningGroup(QXmlStreamReader &reader, uint pageId);
    void parseWarnings(QXmlStreamReader &reader, uint byteStart, uint byteOnce, uint groupLevel);
//    void parseWarnings(QXmlStreamReader &reader);
    void parseParams(QXmlStreamReader &reader, QStackedWidget *stackedWidget, ParamType pType);
    void parseParamPairs(QXmlStreamReader &reader, QStackedWidget *stackedWidget, ParamType pType);
    int saveParamAttrById(QStringRef id, QStringRef type, QStringRef precision, QStringRef disp, ParamType pType);
    int bindWarningTypeAndId(const QString &name, uint level);
};

#endif // PARAMLOADER_H
