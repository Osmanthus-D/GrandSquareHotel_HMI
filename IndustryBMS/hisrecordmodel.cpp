#include "hisrecordmodel.h"

HisRecordModel::HisRecordModel(Callback *callback, QObject *parent) : QAbstractTableModel(parent), cb(callback)
{
    initWarningMap();
}

int HisRecordModel::rowCount(const QModelIndex &) const
{
    return queueCoordinate.size();
}

int HisRecordModel::columnCount(const QModelIndex &) const
{
    return 4;
}

QVariant HisRecordModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        if (0 == index.column()) {
            int p = queueCoordinate.at(index.row());
            QString value = getAlarmStr(p, true);
            return (value.isEmpty() ? tr("unregistered coordinate:(%1,%2,%3)").arg(ONES(p)).arg(TENS(p)).arg(HUNDREDS(p)) : value);
        } else if (1 == index.column()) {
            Coordiante coordinate = {queueCoordinate.at(index.row())};
            return coordinate.n;
        } else if (2 == index.column()){
            return queueTimePair.at(index.row())[0];
        } else if (3 == index.column()) {
            return queueTimePair.at(index.row())[1];
        }
    }

    return QVariant();
}

QVariant HisRecordModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if(Qt::Horizontal == orientation)
    {
        switch (section) {
        case 0:
            return tr("Alarm Item");
        case 1:
            return tr("Module");
        case 2:
            return tr("Trigger Time");
        case 3:
            return tr("Clear Time");
        default:
            break;
        }
    }
    else if(Qt::Vertical == orientation)
    {
        return QString::number(section + 1);
    }

    return QVariant();
}

void HisRecordModel::initWarningMap()
{
    wMap.insert(10, "over voltage fault");
    wMapTr.insert(10, tr("over voltage fault"));
    wMap.insert(11, "charge over current fault");
    wMapTr.insert(11, tr("charge over current fault"));
    wMap.insert(14, "battery over voltage fault");
    wMapTr.insert(14, tr("battery over voltage fault"));
    wMap.insert(15, "battery charge over temp fault");
    wMapTr.insert(15, tr("battery charge over temp fault"));
    wMap.insert(16, "battery discharge over temp fault");
    wMapTr.insert(16, tr("battery discharge over temp fault"));
    wMap.insert(20, "low voltage fault");
    wMapTr.insert(20, tr("low voltage fault"));
    wMap.insert(21, "discharge over current fault");
    wMapTr.insert(21, tr("discharge over current fault"));
    wMap.insert(22, "low SOC fault");
    wMapTr.insert(22, tr("low SOC fault"));
    wMap.insert(23, "low insulation resistance fault");
    wMapTr.insert(23, tr("low insulation resistance fault"));
    wMap.insert(24, "battery low voltage fault");
    wMapTr.insert(24, tr("battery low voltage fault"));
    wMap.insert(25, "battery charge low temp fault");
    wMapTr.insert(25, tr("battery charge low temp fault"));
    wMap.insert(26, "battery discharge low temp fault");
    wMapTr.insert(26, tr("battery discharge low temp fault"));
    wMap.insert(30, "battery over voltage diff warning");
    wMapTr.insert(30, tr("battery over voltage diff warning"));
    wMap.insert(31, "battery over voltage diff fault");
    wMapTr.insert(31, tr("battery over voltage diff fault"));
    wMap.insert(32, "battery over temp diff warning");
    wMapTr.insert(32, tr("battery over temp diff warning"));
    wMap.insert(33, "battery over temp diff fault");
    wMapTr.insert(33, tr("battery over temp diff fault"));

    for(int i = 0; i < 8; i++)
    {
        wMap.insert(40 + i, QString("slave#%1 battery sampling fault").arg(i + 1 + 8));
        wMapTr.insert(40 + i, tr("slave#%1 battery sampling fault").arg(i + 1 + 8));
        wMap.insert(50 + i, QString("slave#%1 battery sampling fault").arg(i + 1));
        wMapTr.insert(50 + i, tr("slave#%1 battery sampling fault").arg(i + 1));
        wMap.insert(60 + i, QString("slave#%1 communication loss fault").arg(i + 1 + 8));
        wMapTr.insert(60 + i, tr("slave#%1 communication loss fault").arg(i + i + 8));
        wMap.insert(70 + i, QString("slave#%1 communication loss fault").arg(i + 1));
        wMapTr.insert(70 + i, tr("slave#%1 communication loss fault").arg(i + 1));
    }

    wMap.insert(110, "self-check slave communication loss fault");
    wMapTr.insert(110, tr("self-check slave communication fault"));
    wMap.insert(111, "self-check slave sampling fault");
    wMapTr.insert(111, tr("self-check slave sampling fault"));
    wMap.insert(120, "self-check over voltage fault");
    wMapTr.insert(120, tr("self-check over voltage fault"));
    wMap.insert(121, "self-check low voltage fault");
    wMapTr.insert(121, tr("self-check low voltage fault"));
    wMap.insert(122, "self-check charge over current fault");
    wMapTr.insert(122, tr("self-check charge over current fault"));
    wMap.insert(123, "self-check discharge over current fault");
    wMapTr.insert(123, tr("self-check discharge over current fault"));
    wMap.insert(124, "self-check battery over voltage");
    wMapTr.insert(124, tr("self-check battery over voltage"));
    wMap.insert(125, "self-check battery low voltage");
    wMapTr.insert(125, tr("self-check battery low voltage"));
    wMap.insert(126, "self-check battery over temp");
    wMapTr.insert(126, tr("self-check battery over temp"));
    wMap.insert(127, "self-check battery low temp");
    wMapTr.insert(127, tr("self-check battery low temp"));
    wMap.insert(130, "charge relay fault");
    wMapTr.insert(130, tr("charge relay fault"));
    wMap.insert(140, "fuse fault");
    wMapTr.insert(140, tr("fuse fault"));
    wMap.insert(150, "main circuit breaker fault");
    wMapTr.insert(150, tr("main circuit breaker fault"));
    wMap.insert(160, "over voltage warning");
    wMapTr.insert(160, tr("over voltage warning"));
    wMap.insert(161, "charge over current warning");
    wMapTr.insert(160, tr("over voltage warning"));
    wMap.insert(164, "battery over voltage warning");
    wMapTr.insert(164, tr("battery over voltage warning"));
    wMap.insert(165, "battery charge over temp warning");
    wMapTr.insert(165, tr("battery charge over temp warning"));
    wMap.insert(166, "battery discharge over temp warning");
    wMapTr.insert(166, tr("battery discharge over temp warning"));
    wMap.insert(170, "low voltage warning");
    wMapTr.insert(170, tr("low voltage warning"));
    wMap.insert(171, "discharge over current warning");
    wMapTr.insert(171, tr("discharge over current warning"));
    wMap.insert(172, "low SOC warning");
    wMapTr.insert(172, tr("low SOC warning"));
    wMap.insert(173, "low insulation resistance warning");
    wMapTr.insert(173, tr("low insulation resistance warning"));
    wMap.insert(174, "battery low voltage warning");
    wMapTr.insert(174, tr("battery low voltage warning"));
    wMap.insert(175, "battery low temp warning");
    wMapTr.insert(175, tr("battery low temp warning"));
    wMap.insert(176, "battery discharge low temp warning");
    wMapTr.insert(176, tr("battery discharge low temp warning"));
}

int HisRecordModel::initRecordModelFromFile(QString fileName)
{
    bool ok = false;
    int p = 0;              // coordinate
    QString ln;
    QStringList s;
    QFile f(fileName);

    if(!f.open(QFile::ReadOnly))
    {
        qDebug("unable to open file %s, skip loading history data.", qPrintable(fileName));
        return -1;
    }
    else
    {
        while(!f.atEnd())
        {
            ln = f.readLine();
            s = ln.split(QChar(' '));

            if(queueCoordinate.size() >= HISTORY_RECORD_MAX_LINE)
            {
                removeRecord();
            }

            if(s.size() > 4)
            {
//                qDebug() << s;
                p = s.at(4).toInt(&ok);
                if(ok)
                {
                    if(0 == s.at(3).compare("trigger"))
                    {
                        addRecord(p, true, s.at(2));
                    }
                    else if(0 == s.at(3).compare("clear"))
                    {
                        addRecord(p, false, s.at(2));
                    }
                }
            }
        }
    }

    f.close();
    return 0;
}

void HisRecordModel::removeRecord()
{
    if(queueCoordinate.isEmpty() || queueTimePair.isEmpty())
    {
        return;
    }

#if HISTORY_RECORD_NEWEST_BUTTOM
    queueCoordinate.pop_back();
    queueTimePair.pop_back();
#else
    queueCoordinate.pop_front();
    queueTimePair.pop_front();
#endif
}

void HisRecordModel::addRecord(int coordinate, bool isTriggerTime, const QString & dateTime)
{
    QString timeStamp = dateTime;

#if HISTORY_RECORD_NEWEST_BUTTOM
    int index = queueCoordinate.lastIndexOf(coordinate);
#else
    int index = queueCoordinate.indexOf(coordinate);
#endif

    if (timeStamp.isEmpty()) {
        timeStamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    }

    if(isTriggerTime)
    {
        addNewLine(coordinate, timeStamp.replace('T', ' '), isTriggerTime);
        return;
    }

    if(0 <= index)
    {
        if(queueTimePair[index][1].size())
        {
            addNewLine(coordinate, timeStamp.replace('T', ' '), isTriggerTime);
        }
        else
        {
            closeTimePair(index, timeStamp.replace('T', ' '));
        }
    }
    else
    {
        addNewLine(coordinate, timeStamp.replace('T', ' '), isTriggerTime);
    }

    refresh();
}

void HisRecordModel::addNewLine(int coordinate, QString time, bool isTriggerTime)
{
    QStringList timePair;

#if HISTORY_RECORD_NEWEST_BUTTOM
    beginInsertRows(QModelIndex(), rowCount(), rowCount());

    queueCoordinate.push_back(coordinate);
    if(isTriggerTime)
    {
        timePair << time << "";
    }
    else
    {
        timePair << "" << time;
    }
    queueTimePair.push_back(timePair);

    endInsertRows();
#else
    beginInsertRows(QModelIndex(), rowCount(), rowCount());

    queueCoordinate.push_front(coordinate);
    if(isTriggerTime)
    {
        timePair << time << "";
    }
    else
    {
        timePair << "" << time;
    }
    queueTimePair.push_front(timePair);

    endInsertRows();
#endif
}

void HisRecordModel::closeTimePair(int index, QString clearTime)
{
    QStringList pair;
    QString triggerTime;

    triggerTime = queueTimePair[index][0];
    pair << triggerTime << clearTime;
    queueTimePair[index] = pair;
}

void HisRecordModel::refresh()
{
    QModelIndex topLeft = createIndex(0, 0);
    QModelIndex bottomRight = createIndex(rowCount(), columnCount());

    emit dataChanged(topLeft, bottomRight);
}

QString HisRecordModel::getAlarmStr(int idx, bool isTr) const
{
    Q_UNUSED(isTr)

    Coordiante coordinate = {idx};

    if (NULL == cb) {
        return "";
    }

    return cb->getWarningDesc(coordinate.key);
}
