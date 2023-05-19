#include "paramloader.h"

ParamLoader::ParamLoader(QObject *parent) : QObject(parent)
{
    myKeyboard = new Mykeyboard();
    bitCalculator = new BitCalculator();
}

ParamLoader::ParamLoader(const QString &configPath, QObject *parent)
    : QObject(parent)
{
    myKeyboard = new Mykeyboard();
    bitCalculator = new BitCalculator();
    xmlFile.setFileName(configPath);
}

ParamLoader::~ParamLoader()
{
    delete myKeyboard;
    delete bitCalculator;
}

void ParamLoader::setXmlFilePath(const QString &configPath)
{
    xmlFile.setFileName(configPath);
}

QString ParamLoader::getXmlFilePath() const {
    return xmlFile.fileName();
}

bool ParamLoader::load()
{
    if (!xmlFile.exists() || (QFile::NoError != xmlFile.error())) {
        qDebug() << tr("ERROR: Unable to open config file") << xmlFile.fileName();
        return false;
    }

    xmlFile.open(QFile::ReadOnly);
    QXmlStreamReader reader(&xmlFile);
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            if ("warninglist" == reader.name()) {
                parseWarnings(reader);
            }
        }
    }

    if (reader.hasError()) {
        qDebug() << QString("Error parsing %1 on line %2 column %3: \n%4")
                 .arg(xmlFile.fileName())
                 .arg(reader.lineNumber())
                 .arg(reader.columnNumber())
                 .arg(reader.errorString());
    }

    qDebug() << wnSet << wnLevel;
    xmlFile.close();
    return true;
}

bool ParamLoader::load(QStackedWidget *stackedWidget, ParamType pType)
{
    if (NULL == stackedWidget) {
        return false;
    }

    if (!xmlFile.exists() || (QFile::NoError != xmlFile.error())) {
        qDebug() << "ERROR: Unable to open config file" << xmlFile.fileName();
        return false;
    }

    xmlFile.open(QFile::ReadOnly);
    QXmlStreamReader reader(&xmlFile);
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            if ("paramlist" == reader.name()) {
                QXmlStreamAttributes attrs = reader.attributes();
                QStringRef type = attrs.value("type");
                QStringRef pair = attrs.value("pair");
                if (!type.isEmpty() && 0 == type.compare(paramType2String(pType))) {
                    qDebug() << type;
                    if (pair.isEmpty() || (!pair.isEmpty() && 0 == pair.compare("false"))) {
                        parseParams(reader, stackedWidget, pType);
                    } else if (!pair.isEmpty() && 0 == pair.compare("true")) {
                        parseParamPairs(reader, stackedWidget, pType);
                    }
                }
            }
        }
    }

    if (reader.hasError()) {
        qDebug() << QString("Error parsing %1 on line %2 column %3: \n%4")
                 .arg(xmlFile.fileName())
                 .arg(reader.lineNumber())
                 .arg(reader.columnNumber())
                 .arg(reader.errorString());
    }

    xmlFile.close();
    return true;
}

void ParamLoader::parseParams(QXmlStreamReader &reader, QStackedWidget *stackedWidget, ParamType pType)
{
    bool isGridLayoutNotEmpty = false;
    int existingPages = stackedWidget->count();
    int i = 0;
    int _id = 0;
    int _disp = 0;
    QFont font;
    QFrame *vLine = NULL;
    QWidget *page = NULL;
    QCheckBox *checkBox = NULL;
    QLineEdit *lineEdit = NULL;
    QGroupBox *groupBox = NULL;
    QGridLayout *gridLayout = NULL;
    QHBoxLayout *hBoxLayout = NULL;
    QHBoxLayout *hBoxLayout4Page = NULL;
    SwitchButton *switchButton = NULL;

    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && "param" == reader.name()) {
            QXmlStreamAttributes attrs = reader.attributes();
            if (0 == i % MAX_PARAM_ROW) {
                if (NULL == gridLayout) {
                    gridLayout = new QGridLayout();
                } else if (!gridLayout->isEmpty()) {
                    isGridLayoutNotEmpty = true;
                    hBoxLayout->addLayout(gridLayout, 1);
                    gridLayout = new QGridLayout();
                } else {
                    isGridLayoutNotEmpty = false;
                }
            }

            if (0 == i % MAX_PARAM_IN_PAGE) {
                if (NULL == page || (NULL != page && isGridLayoutNotEmpty)) {
                    if (NULL != groupBox && NULL != hBoxLayout) {
                        vLine = new QFrame(groupBox);
                        vLine->setFrameShape(QFrame::VLine);
                        vLine->setFrameShadow(QFrame::Sunken);
                        hBoxLayout->insertWidget(1, vLine);
                    }

                    page = new QWidget();
                    hBoxLayout4Page = new QHBoxLayout(page);
                    groupBox = groupBoxRunningParam[i / MAX_PARAM_IN_PAGE];
                    groupBox = new QGroupBox(page);
                    hBoxLayout = new QHBoxLayout(groupBox);
                    hBoxLayout4Page->setMargin(0);
                    hBoxLayout4Page->addWidget(groupBox);
                    stackedWidget->insertWidget(stackedWidget->count() - existingPages, page);
                }
            }

            if (NULL == groupBox) {
                return;
            }

            QStringRef id = attrs.value("id");
            QStringRef name = attrs.value("name");
            QStringRef type = attrs.value("type");
            QStringRef precision = attrs.value("precision");
            QStringRef disp = attrs.value("disp");

            _id = saveParamAttrById(id, type, precision, disp, pType);
            if (0 > _id) {
                continue;
            }

            _disp = getDispByType(_id, pType);
            if (NoDisplay == _disp) {
                continue;
            }

            qDebug() << name.toUtf8() << id;
            checkBox = new QCheckBox(groupBox);
            checkBox->setObjectName("checkBox" + paramType2Abbr(pType) + id.toString());
            checkBox->setText(name.toString());
            font = checkBox->font();
            font.setPointSize(12);
            checkBox->setFont(font);
            gridLayout->addWidget(checkBox, i % MAX_PARAM_ROW, 0, Qt::AlignLeft);

            switch (_disp) {
            case Decimal:
            case Hex:
                lineEdit = new QLineEdit(groupBox);
                lineEdit->setObjectName("lineEdit" + paramType2Abbr(pType) + id.toString());
                font = lineEdit->font();
                if (0 < _id && Hex == getDispByType(_id, pType)) {
                    font.setPointSize(11);
                } else {
                    font.setPointSize(13);
                }

                lineEdit->setFont(font);
                lineEdit->setMaximumWidth(160);
                lineEdit->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
                if (Decimal == _disp) {
                    lineEdit->installEventFilter(myKeyboard);
                } else if (Hex == _disp) {
                    lineEdit->installEventFilter(bitCalculator);
                }

                gridLayout->addWidget(lineEdit, i % MAX_PARAM_ROW, 1, Qt::AlignCenter);
                gridLayout->setAlignment(lineEdit, Qt::AlignRight);
                break;
            case ZeroAndOne:
            case MinusOneAndOne:
                switchButton = new SwitchButton(groupBox);
                switchButton->setObjectName("switchButton" + paramType2Abbr(pType) + id.toString());
                switchButton->setMinimumWidth(75);
                switchButton->setMinimumHeight(29);
                switchButton->setTextOn("ON");
                switchButton->setTextOff("OFF");
                gridLayout->addWidget(switchButton, i % MAX_PARAM_ROW, 1, Qt::AlignCenter);
                break;
            case OnlyOne:
                break;
            default:
                break;
            }

            ++i;
        } else if (reader.isEndElement() && "paramlist" == reader.name()) {
            if (NULL != gridLayout && NULL != hBoxLayout) {
                hBoxLayout->addLayout(gridLayout, 1);
                if (0 != i % MAX_PARAM_ROW) {
                    QSpacerItem *spacerItem = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
                    gridLayout->addItem(spacerItem, i % MAX_PARAM_ROW, 0);
                }

                if (1 < hBoxLayout->count()) {
                    vLine = new QFrame(groupBox);
                    vLine->setFrameShape(QFrame::VLine);
                    vLine->setFrameShadow(QFrame::Sunken);
                    hBoxLayout->insertWidget(1, vLine);
                } else {
                    hBoxLayout->addStretch(1);
                }
            }

            if (0 != stackedWidget->count()) {
                stackedWidget->setCurrentIndex(0);
            }

            return;
        }
    }
}

void ParamLoader::parseParamPairs(QXmlStreamReader &reader, QStackedWidget *stackedWidget, ParamType pType)
{
    int i = 0;
    int _id = 0;
    int _disp = 0;
    int existingPages = stackedWidget->count();
    QFont font;
    QFrame *vLine = NULL;
    QWidget *page = NULL;
    QCheckBox *checkBox = NULL;
    QLineEdit *lineEdit = NULL;
    QGroupBox *groupBox = NULL;
    QGridLayout *gridLayoutLeft = NULL;
    QGridLayout *gridLayoutRight = NULL;
    QHBoxLayout *hBoxLayout = NULL;
    QHBoxLayout *hBoxLayout4Page = NULL;

    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && "param" == reader.name()) {
            QXmlStreamAttributes attrs = reader.attributes();
            if (0 == i % MAX_PARAM_IN_PAGE) {
                if (NULL != gridLayoutLeft && NULL != gridLayoutRight && NULL != hBoxLayout) {
                    hBoxLayout->addLayout(gridLayoutLeft, 1);
                    hBoxLayout->addLayout(gridLayoutRight, 1);
                    vLine = new QFrame(groupBox);
                    vLine->setFrameShape(QFrame::VLine);
                    vLine->setFrameShadow(QFrame::Sunken);
                    hBoxLayout->insertWidget(1, vLine);
                }

                gridLayoutLeft = new QGridLayout();
                gridLayoutRight = new QGridLayout();
                groupBox = groupBoxRunningParam[i / MAX_PARAM_IN_PAGE];
                groupBox = new QGroupBox(page);
                hBoxLayout = new QHBoxLayout(groupBox);
                page = new QWidget();
                hBoxLayout4Page = new QHBoxLayout(page);
                hBoxLayout4Page->setMargin(0);
                hBoxLayout4Page->addWidget(groupBox);
                stackedWidget->insertWidget(stackedWidget->count() - existingPages, page);
            }

            if (NULL == groupBox) {
                return;
            }

            QStringRef id = attrs.value("id");
            QStringRef name = attrs.value("name");
            QStringRef type = attrs.value("type");
            QStringRef precision = attrs.value("precision");
            QStringRef disp = attrs.value("disp");

            _id = saveParamAttrById(id, type, precision, disp, pType);
            if (0 > _id) {
                continue;
            }

            _disp = getDispByType(_id, pType);
            if (NoDisplay == _disp) {
                continue;
            }

            qDebug() << name.toUtf8() << id;
            checkBox = new QCheckBox(groupBox);
            checkBox->setObjectName("checkBox" + paramType2Abbr(pType) + id.toString());
            checkBox->setText(name.toString());
            font = checkBox->font();
            font.setPointSize(12);
            checkBox->setFont(font);
            if (0 == i % 2) {
                gridLayoutLeft->addWidget(checkBox, (i % MAX_PARAM_IN_PAGE) / 2, 0, Qt::AlignLeft);
            } else {
                gridLayoutRight->addWidget(checkBox, (i % MAX_PARAM_IN_PAGE) / 2, 0, Qt::AlignLeft);
            }

            lineEdit = new QLineEdit(groupBox);
            lineEdit->setObjectName("lineEdit" + paramType2Abbr(pType) + id.toString());
            font = lineEdit->font();
            if (0 < _id && Hex == getDispByType(_id, pType)) {
                font.setPointSize(11);
            } else {
                font.setPointSize(13);
            }

            lineEdit->setFont(font);
            lineEdit->setMaximumWidth(160);
            lineEdit->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            if (Decimal == _disp) {
                lineEdit->installEventFilter(myKeyboard);
            } else if (Hex == _disp) {
                lineEdit->installEventFilter(bitCalculator);
            }

            if (0 == i % 2) {
                gridLayoutLeft->addWidget(lineEdit, (i % MAX_PARAM_IN_PAGE) / 2, 1, Qt::AlignCenter);
                gridLayoutLeft->setAlignment(lineEdit, Qt::AlignRight);
            } else {
                gridLayoutRight->addWidget(lineEdit, (i % MAX_PARAM_IN_PAGE) / 2, 1, Qt::AlignCenter);
                gridLayoutRight->setAlignment(lineEdit, Qt::AlignRight);
            }

            ++i;
        } else if (reader.isEndElement() && "paramlist" == reader.name()) {
            if (NULL != gridLayoutLeft && NULL != gridLayoutRight && NULL != hBoxLayout) {
                hBoxLayout->addLayout(gridLayoutLeft, 1);
                hBoxLayout->addLayout(gridLayoutRight, 1);
                vLine = new QFrame(groupBox);
                vLine->setFrameShape(QFrame::VLine);
                vLine->setFrameShadow(QFrame::Sunken);
                hBoxLayout->insertWidget(1, vLine);

                if (MAX_PARAM_ROW < gridLayoutLeft->rowCount()) {
                    QSpacerItem *spacerItem = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
                    gridLayoutLeft->addItem(spacerItem, gridLayoutLeft->rowCount(), 0);
                }

                if (MAX_PARAM_ROW < gridLayoutRight->rowCount()) {
                    QSpacerItem *spacerItem = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
                    gridLayoutRight->addItem(spacerItem, gridLayoutRight->rowCount(), 0);
                }
            }

            if (0 != stackedWidget->count()) {
                stackedWidget->setCurrentIndex(0);
            }

            return;
        }
    }
}

void ParamLoader::parseWarnings(QXmlStreamReader &reader)
{
    bool ok = false;
    uint _id = 0;
    uint _level = 0;
    uint _start = 0;
    uint _num = 0;

    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && 0 == reader.name().compare("warning", Qt::CaseInsensitive)) {
            QXmlStreamAttributes attrs = reader.attributes();
            QStringRef id = attrs.value("id");
            QStringRef name = attrs.value("name");
            QStringRef level = attrs.value("level");
            QStringRef array = attrs.value("array");
            QStringRef start = attrs.value("start");
            QStringRef num = attrs.value("num");

            if (MAX_WARNING_NUM_PER_MODULE - 1 < wnSet.size()) {
                continue;
            }

            _id = id.toString().toUInt(&ok);
            if (!ok) {
                continue;
            }

            _level = level.toString().toUInt(&ok);
            if (!ok || name.isEmpty()) {
                continue;
            }

            if (0 == array.compare("1") && name.contains("%1")) {
                _num = num.toString().toUInt(&ok);
                if (!ok && 0 < _num) {
                    continue;
                }

                _start = start.toString().toUInt(&ok);
                if (!ok) {
                    continue;
                }

                for (uint i = 0; i < _num; ++i) {
                    wnSet.insert(_id + i, name.toString().arg(_start + i));
                    wnLevel.insert(_id + i, _level);
                }
            } else {
                wnSet.insert(_id, name.toString());
                wnLevel.insert(_id, _level);
            }
        }
    }
}

QString ParamLoader::paramType2Char(ParamType type)
{
    QString ret;

    switch (type) {
    case Operating:
        ret = "m";
        break;
    case Warning:
        ret = "f";
        break;
    case Calibration:
        ret = "c";
        break;
    default:
        break;
    }

    return ret;
}

QString ParamLoader::paramType2String(ParamType type)
{
    QString ret;

    switch (type) {
    case Operating:
        ret = "operating";
        break;
    case Warning:
        ret = "warning";
        break;
    case Calibration:
        ret = "calibration";
        break;
    default:
        break;
    }

    return ret;
}

QString ParamLoader::paramType2Abbr(ParamType type)
{
    QString ret;

    switch (type) {
    case Operating:
        ret = "Oprt";
        break;
    case Warning:
        ret = "Warn";
        break;
    case Calibration:
        ret = "Clbr";
        break;
    default:
        break;
    }

    return ret;
}

int ParamLoader::saveParamAttrById(QStringRef id, QStringRef type, QStringRef precision, QStringRef disp, ParamType pType)
{
    bool ok = false;
    int _id = 0;
    int _type = 0;
    int _precision = 0;
    int _disp = 0;

    if (id.isEmpty()) {
        return -1;
    }

    _id = id.toString().toInt(&ok);
    if (!ok || 0 > _id) {
        return -1;
    }

    if (!type.isEmpty()) {
        _type = type.toString().toInt(&ok);
        if (ok) {
            switch (pType) {
            case Operating:
                oprtTypeMap.insert(_id, _type);
                break;
            case Warning:
                warnTypeMap.insert(_id, _type);
                break;
            case Calibration:
                clbrTypeMap.insert(_id, _type);
                break;
            default:
                break;
            }
        }
    }

    if (!precision.isEmpty()) {
        _precision = precision.toString().toInt(&ok);
        if (ok) {
            switch (pType) {
            case Operating:
                oprtPrcsMap.insert(_id, _precision);
                break;
            case Warning:
                warnPrcsMap.insert(_id, _precision);
                break;
            case Calibration:
                clbrPrcsMap.insert(_id, _precision);
                break;
            default:
                break;
            }
        }
    }

    if (!disp.isEmpty()) {
        _disp = disp.toString().toInt(&ok);
        if (ok) {
            switch (pType) {
            case Operating:
                oprtDispMap.insert(_id, _disp);
                break;
            case Warning:
                warnDispMap.insert(_id, _disp);
                break;
            case Calibration:
                clbrDispMap.insert(_id, _disp);
            default:
                break;
            }
        }
    }

    return _id;
}

QString ParamLoader::getWnNameById(int id)
{
    return wnSet.value(id, QString("undefined id: %1").arg(id));
}

int ParamLoader::getWnLevelById(int id)
{
    return wnLevel.value(id, 0);
}

/**
 * @brief ParamLoader::getTypeByType
 * @param id
 * @param type
 * @return false signed true unsigned
 */
bool ParamLoader::getTypeByType(int id, ParamType type)
{
    int _type = 0;

    switch (type) {
    case Operating:
        _type = oprtTypeMap.value(id, 0);
        break;
    case Warning:
        _type = warnTypeMap.value(id, 0);
        break;
    case Calibration:
        _type = clbrTypeMap.value(id, 0);
        break;
    default:
        break;
    }

    return (0 == _type);
}

/**
 * @brief ParamLoader::getPrecisionByType
 * @param id
 * @param type
 * @return 0 1 2 3
 */
int ParamLoader::getPrecisionByType(int id, ParamType type)
{
    int precision = 0;

    switch (type) {
    case Operating:
        precision = oprtPrcsMap.value(id, 0);
        break;
    case Warning:
        precision = warnPrcsMap.value(id, 0);
        break;
    case Calibration:
        precision = clbrPrcsMap.value(id, 0);
        break;
    default:
        break;
    }

    return precision;
}

/**
 * @brief ParamLoader::getDispByType
 * @param id
 * @param type
 * @return 0 1 2 3 4 5
 */
int ParamLoader::getDispByType(int id, ParamType type)
{
    int disp = 0;

    switch (type) {
    case Operating:
        disp = oprtDispMap.value(id, 1);
        break;
    case Warning:
        disp = warnDispMap.value(id, 1);
        break;
    case Calibration:
        disp = clbrDispMap.value(id, 1);
        break;
    default:
        break;
    }

    return disp;
}
