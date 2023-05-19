#include "mymodel.h"
#include "bms.h"

MyModel::MyModel(int type, QObject *parent) :
    QAbstractTableModel(parent), type(type)
{
    mw = qobject_cast<MainWindow *>(parent);
}

int MyModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    int row = type?TEMP_NUM:BAT_NUM;

    return row;
}

int MyModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return MAX_PACK_NUM;
}

QVariant MyModel::data(const QModelIndex &index, int role) const
{
    QString str;
    int id = index.column() * rowCount() + index.row();
    short value = 0;

    if (NULL == mw) {
        return QVariant();
    }

    value = type?mw->module[mw->get_m_page()].temp[id]:mw->module[mw->get_m_page()].voltage[id];
    switch(role) {
    case Qt::DisplayRole:
        str = type ? QString::fromUtf8("%1 ℃").arg((float)value / 10, 0, 'f', 1)
                 : QString("%1 V").arg((float)value / 1000, 0, 'f', 3);
        return str;
    case Qt::TextAlignmentRole:
        return Qt::AlignHCenter + Qt::AlignVCenter;
    default:
        return QVariant();
    }
}

QVariant MyModel::headerData(int section, Qt::Orientation orientation, int role) const
  {
      switch(role) {
      case Qt::DisplayRole:
          if (Qt::Horizontal == orientation) {
              return QString::number(section+1);
          } else if (Qt::Vertical == orientation) {
              return type ? tr("Temp %1").arg(section + 1) : tr("Cell %1").arg(section + 1);
          }
          break;
      case Qt::TextAlignmentRole:
          return Qt::AlignHCenter + Qt::AlignVCenter;
      default:
          return QVariant();
      }

      return QVariant();
  }

void MyModel::refresh()
{
    QModelIndex topleft = createIndex(0,0);
    QModelIndex bottomRight = createIndex(rowCount(), columnCount());

    emit dataChanged(topleft, bottomRight);
}
