#ifndef SYSTEMTYPEDIALOG_H
#define SYSTEMTYPEDIALOG_H

#include <QDialog>
#include "tumbler.h"

namespace Ui {
class SystemTypeDialog;
}

class SystemTypeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SystemTypeDialog(QWidget *parent = NULL);
    ~SystemTypeDialog();

private:
    Ui::SystemTypeDialog *ui;

    QStringList systemTypeList;
    Tumbler *tumblerSystemType;

private slots:
    void on_buttonBox_accepted();
};

#endif // SYSTEMTYPEDIALOG_H
