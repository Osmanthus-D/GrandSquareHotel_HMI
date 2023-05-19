#ifndef HISRECORDMODEL_H
#define HISRECORDMODEL_H

#include <QAbstractTableModel>
#include <QStringList>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include "callback.h"

#define HISTORY_RECORD_MAX_LINE             1024
#define HISTORY_RECORD_NEWEST_BUTTOM        0

#define COORDINATE_TO_NUMBER(i, j ,k)       ((i) * 100 + (j) * 10 + (k))
#define ONES(num)                           (num / 100)
#define TENS(num)                           ((num % 100) / 10)
#define HUNDREDS(num)                       (num % 10)

typedef union Coordinate_t {
    int data;

    struct {
        unsigned char n;
        unsigned char level;
        unsigned short key;
    };
} Coordiante;

class HisRecordModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit HisRecordModel(Callback * callback = NULL, QObject *parent = NULL);
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    int initRecordModelFromFile(QString fileName);
    void addRecord(int coordinate, bool isTriggerTime, const QString & dateTime = "");
    void refresh();
    QString getAlarmStr(int idx, bool isTr) const;

signals:
    
private:
    QMap<int, QString> wMap;
    QMap<int, QString> wMapTr;
    QList<int> queueCoordinate;
    QList<QStringList> queueTimePair;   // pair of trigger time and clear time
    Callback *cb;

    void initWarningMap();
    void removeRecord();
    void addNewLine(int coordinate, QString time, bool isTriggerTime);
    void closeTimePair(int index, QString clearTime);
};

#endif // HISRECORDMODEL_H
