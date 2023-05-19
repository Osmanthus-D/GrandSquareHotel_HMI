#include "systemtypedialog.h"
#include "ui_systemtypedialog.h"

SystemTypeDialog::SystemTypeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SystemTypeDialog)
{
    ui->setupUi(this);

    systemTypeList << "Industrial Energy Storage" << "UPS_A" << "UPS_B";
    tumblerSystemType = new Tumbler(this);
    tumblerSystemType->setListValue(systemTypeList);
    tumblerSystemType->setBackground(Qt::transparent);

    ui->verticalLayout->addWidget(tumblerSystemType);
}

SystemTypeDialog::~SystemTypeDialog()
{
    delete ui;
}

void SystemTypeDialog::on_buttonBox_accepted()
{
    done(tumblerSystemType->getCurrentIndex());
}
