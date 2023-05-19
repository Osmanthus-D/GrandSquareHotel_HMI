#include "bitcalculator.h"

BitCalculator::BitCalculator(QWidget *parent) :
    QDialog(parent)
{
    init();
}

void BitCalculator::init()
{
    QVBoxLayout *vLayoutMain;
    QHBoxLayout *hLayoutTop;
    QSpacerItem *vSpacer;
    QDialogButtonBox *buttonBox;
    QLabel *label = NULL;
    QCheckBox *checkBox = NULL;
    QFont font;

    this->resize(400, 360);
    this->setWindowTitle("BitCalculator");
    vLayoutMain = new QVBoxLayout(this);
    hLayoutTop = new QHBoxLayout();
    lineEditValue = new QLineEdit(this);
    comboBox = new QComboBox(this);
    gridLayoutBody = new QGridLayout();
    buttonBox = new QDialogButtonBox(this);
    vSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    comboBox->addItem("Hex");
    comboBox->addItem("Decimal");
    lineEditValue->setReadOnly(true);
    hLayoutTop->addWidget(lineEditValue);
    hLayoutTop->addWidget(comboBox);
    vLayoutMain->addLayout(hLayoutTop);
    vLayoutMain->addItem(vSpacer);
    gridLayoutBody->setVerticalSpacing(20);
    vLayoutMain->addLayout(gridLayoutBody);
    font.setPointSize(6);

    vSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    vLayoutMain->addItem(vSpacer);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    vLayoutMain->addWidget(buttonBox);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(numDisplay()));
    QMetaObject::connectSlotsByName(this);

    mValue = 0xFFFFFFFF;
    for (uint i = 0; i < sizeof(mValue) + 1; ++i) {
        for (uint j = 0; j < CHAR_BIT + 1; ++j) {
            if (0 == i && 0 < j) {
                label = new QLabel(this);
                label->setFont(font);
                label->setAlignment(Qt::AlignCenter);
                label->setText(QString("b%1").arg(CHAR_BIT - j));
                label->setEnabled(false);
                gridLayoutBody->addWidget(label, i, j);
            } else if (0 < i && 0 == j) {
                label = new QLabel(this);
                label->setFont(font);
                label->setAlignment(Qt::AlignCenter);
                label->setText(QString("B%1").arg(4 - i));
                label->setEnabled(false);
                gridLayoutBody->addWidget(label, i, j);
            } else if (0 < i && 0 < j) {
                checkBox = new QCheckBox(this);
                checkBox->setChecked(true);
                gridLayoutBody->addWidget(checkBox, i, j);
                connect(checkBox, SIGNAL(toggled(bool)), this, SLOT(bit2Byte(bool)));
            }
        }
    }
    lineEditValue->setText(QString::number(mValue, 16).toUpper());
    this->setStyleSheet("QCheckBox::indicator{ width: 30px; height: 30px; } \n"
                        "QCheckBox::indicator:unchecked{ image:url(:/image/number_zero.png); } \n"
                        "QCheckBox::indicator:checked{ image:url(:/image/number_one.png); } \n"
                        "QDialog{ background-color: rgb(221, 219, 217); }");
    focusedLineEdit = NULL;
}

bool BitCalculator::byte2Bit(const QString &text)
{
    bool ok = false;

    mValue = text.toUInt(&ok, 16);
    if (ok) {
        numDisplay();
        for (uint i = 0; i < sizeof(mValue) + 1; ++i) {
            for (uint j = 0; j < CHAR_BIT + 1; ++j) {
                if (0 < i * j) {
                    QCheckBox *checkBox = qobject_cast<QCheckBox *>(gridLayoutBody->itemAtPosition(i, j)->widget());
                    if (NULL != checkBox) {
                        checkBox->setChecked((mValue >> getBitIndexInGrid(i, j)) & 0x01);
                    }
                }
            }
        }
    }

    return ok;
}

void BitCalculator::numDisplay()
{
    if (0 == comboBox->currentIndex()) {                        // hex display
        lineEditValue->setText(hex2Str(mValue));
    } else {                                                    // decimal display
        lineEditValue->setText(QString::number(mValue));
    }
}

void BitCalculator::bit2Byte(bool isChecked)
{
    int bitIndex = 0;
    QCheckBox *checkBox = qobject_cast<QCheckBox *>(sender());

    if (NULL == checkBox) {
        return;
    }

    bitIndex = getBitIndexByItem(checkBox);
    if(bitIndex < 0) {
        return;
    }

    if (isChecked) {
        mValue |= 0x01 << bitIndex;
    } else {
        mValue &= ~(0x01 << bitIndex);
    }

    numDisplay();
}

int BitCalculator::getBitIndexByItem(QCheckBox *item)
{
    int index = -1;
    QLayoutItem *currentItem = NULL;

    for (uint i = 0; i < sizeof(mValue) + 1; ++i) {
        for (uint j = 0; j < CHAR_BIT + 1; ++j) {
            currentItem = gridLayoutBody->itemAtPosition(i, j);
            if (NULL == currentItem) {
                continue;
            }

            if (qobject_cast<QWidget *>(item) == currentItem->widget()) {
                index = getBitIndexInGrid(i, j);
            }
        }
    }

    return index;
}

int BitCalculator::getBitIndexInGrid(uint i, uint j)
{
    int index = -1;

    if (0 < i && 0 < j) {
        index = CHAR_BIT * (sizeof(mValue) - i + 1) - j;
    }

    return index;
}

void BitCalculator::accept()
{
    if (NULL != focusedLineEdit) {
        focusedLineEdit->setText(hex2Str(mValue));
    }

    close();
}

QString BitCalculator::hex2Str(quint32 val)
{
    return QString("%1").arg(val, sizeof(val) * 2, 16, QChar('0')).toUpper().prepend("0x");
}

bool BitCalculator::eventFilter(QObject *obj, QEvent *e)
{
    QString className = obj->metaObject()->className();

    if (0 == className.compare("QLineEdit")) {
        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(obj);

        if (e->type() == QEvent::MouseButtonPress && lineEdit->isEnabled()) {
            focusedLineEdit = lineEdit;
            byte2Bit(focusedLineEdit->text());
            this->repaint();
            this->exec();
            return true;
        } else {
            return false;
        }
    } else {
        return QObject::eventFilter(obj, e);
    }
}
