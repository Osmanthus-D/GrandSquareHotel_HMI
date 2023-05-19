#ifndef MYMODEL_H
#define MYMODEL_H

#include <QAbstractTableModel>
#include "mainwindow.h"

class MyModel : public QAbstractTableModel
{
    Q_OBJECT

private:
    int type;
    MainWindow *mw;

public:
    MyModel(int type, QObject *parent = NULL);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void refresh();
};

#endif // MYMODEL_H
