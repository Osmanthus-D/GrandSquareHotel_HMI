#ifndef BITCALCULATOR_H
#define BITCALCULATOR_H

#include <QDialog>
#include <QtGui>

class BitCalculator : public QDialog
{
    Q_OBJECT

public:
    explicit BitCalculator(QWidget *parent = NULL);
    static QString hex2Str(quint32 val);

private slots:
    bool eventFilter(QObject *obj, QEvent *e);
    void bit2Byte(bool isChecked);
    void numDisplay();
    void accept();

private:
    quint32 mValue;
    QLineEdit *focusedLineEdit;
    QGridLayout *gridLayoutBody;
    QLineEdit *lineEditValue;
    QComboBox *comboBox;

    void init();
    bool byte2Bit(const QString &text);
    int getBitIndexByItem(QCheckBox *item);
    int getBitIndexInGrid(uint i, uint j);
};

#endif // BITCALCULATOR_H
