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
                parseWarningPage(reader);
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

    qDebug() /*<< wnSet */<< wnLevel << wnFlag;
    QMap<int, QString>::const_iterator i = wnSet.constBegin();
    QMap<int, QString>::const_iterator end = wnSet.constEnd();
    while (i != end) {
        qDebug() << "wnSet" << i.key() << i.value().toUtf8();
        ++i;
    }

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

void ParamLoader::parseWarningPage(QXmlStreamReader &reader)
{
    bool ok = false;
    uint pageId = 0;

    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && 0 == reader.name().compare("page", Qt::CaseInsensitive)) {
            QXmlStreamAttributes attrs = reader.attributes();
            QStringRef id = attrs.value("id");

            pageId = id.toString().toUInt(&ok);
            if (ok) {
                parseWarningGroup(reader, pageId);
            }
        }
    }
}

void ParamLoader::parseWarningGroup(QXmlStreamReader &reader, uint pageId)
{
    bool ok = false;
    uint byteOnce = 0;
    uint byteSum = 0;
    uint groupLevel = 0;

    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && 0 == reader.name().compare("group", Qt::CaseInsensitive)) {
            QXmlStreamAttributes attrs = reader.attributes();
            QStringRef byte = attrs.value("byte");
            QStringRef level = attrs.value("level");

            byteOnce = byte.toString().toUInt(&ok);
            if (!ok) {
                continue;
            }

            groupLevel = level.toString().toUInt(&ok);
            if (ok) {
                parseWarnings(reader, pageId * 8 + byteSum, byteOnce, groupLevel);
            }

            byteSum += byteOnce;
        } else if (reader.isEndElement() && 0 == reader.name().compare("page", Qt::CaseInsensitive)) {
            break;
        }
    }
}

void ParamLoader::parseWarnings(QXmlStreamReader &reader, uint byteStart, uint byteOnce, uint groupLevel)
{
    bool ok = false;
    int flag = 0;
    uint index = 0;
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
            QStringRef start = attrs.value("start");
            QStringRef num = attrs.value("num");

            if (MAX_WARNING_NUM_PER_MODULE - 1 < wnSet.size()) {
                continue;
            }

            _id = id.toString().toUInt(&ok);
            if (!ok) {
                continue;
            }

            _level = groupLevel;
            if (!level.isEmpty()) {
                _level = level.toString().toUInt(&ok);
                if (!ok) {
                    _level = groupLevel;
                }
            }

            if (name.isEmpty()) {
                continue;
            }

            if (1 < byteOnce) {
                _start = start.toString().toUInt(&ok);
                if (ok) {
                    _num = num.toString().toUInt(&ok);
                    if (ok && name.contains("%1")) {
                        for (uint i = 0; i < _num; ++i) {
                            index = (byteStart + byteOnce - (_id + i) / CHAR_BIT - 1)
                                * CHAR_BIT + (_id + i) % CHAR_BIT;
                            flag = bindWarningTypeAndId(name.toString(), _level);

                            wnSet.insert(index, name.toString().arg(_start + i));
                            wnLevel.insert(index, _level);
                            if (-1 < flag) {
                                if (0 < wnFlag.count(flag)) {
                                    wnFlag.insertMulti(flag, index);
                                } else {
                                    wnFlag.insert(flag, index);
                                }
                            }
                        }
                    }

                    continue;
                }

                index = (byteStart + byteOnce - _id / CHAR_BIT - 1)
                    * CHAR_BIT + _id % CHAR_BIT;
                flag = bindWarningTypeAndId(name.toString(), _level);

                wnSet.insert(index, name.toString());
                wnLevel.insert(index, _level);
                if (-1 < flag) {
                    if (0 < wnFlag.count(flag)) {
                        wnFlag.insertMulti(flag, index);
                    } else {
                        wnFlag.insert(flag, index);
                    }
                }
            } else {
                if (CHAR_BIT > _id) {
                    index = byteStart * CHAR_BIT + _id;
                    flag = bindWarningTypeAndId(name.toString(), _level);

                    wnSet.insert(index, name.toString());
                    wnLevel.insert(index, _level);
                    if (-1 < flag) {
                        if (0 < wnFlag.count(flag)) {
                            wnFlag.insertMulti(flag, index);
                        } else {
                            wnFlag.insert(flag, index);
                        }
                    }
                }
            }
        } else if (reader.isEndElement() && 0 == reader.name().compare("group", Qt::CaseInsensitive)) {
            break;
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

int ParamLoader::bindWarningTypeAndId(const QString &name, uint level)
{
    unsigned f = 0;

    if (1 > level || 3 < level || name.isEmpty()) {
        return -1;
    }

    switch (level) {
    case 1:
        f |= W::Level1;
        break;
    case 2:
        f |= W::Level2;
        break;
    case 3:
        f |= W::Level3;
        break;
    default:
        break;
    }

    if (name.toUtf8().contains("过高")) {
        f |= W::Over;
    } else if (name.toUtf8().contains("过低")) {
        f |= W::Low;
    } else if (name.toUtf8().contains("过") || name.toUtf8().contains("高")) {
        f |= W::Over;
    } else if (name.toUtf8().contains("欠") || name.toUtf8().contains("低")) {
        f |= W::Low;
    }

    if (name.toUtf8().contains("放")) {
        f |= W::Discharging;
    } else if (name.toUtf8().contains("充")) {
        f |= W::Charging;
    }

    if (name.toUtf8().contains("单体")) {
        f |= W::Cell;
    } else {
        f |= W::Module;
    }

    if (name.toUtf8().contains("温差")) {
        f |= W::TempDiff;
    } else if (name.toUtf8().contains("压差")) {
        f |= W::VoltDiff;
    } else if (name.toUtf8().contains("绝缘")) {
        f |= W::InsulationRes;
    } else if (name.toUtf8().contains("SOC")) {
        f |= W::Soc;
    } else if (name.toUtf8().contains("自检")) {
        f |= W::SelfCheck;
    } else if (name.toUtf8().contains("继电器") && name.toUtf8().contains("正")) {
        f |= W::PosRelay;
    } else if (name.toUtf8().contains("继电器") && name.toUtf8().contains("负")) {
        f |= W::NegRelay;
    } else if (name.toUtf8().contains("预充")) {
        f |= W::Precharge;
    } else if (name.toUtf8().contains("环境") && name.toUtf8().contains("温")) {
        f |= W::AmbientTemp;
    } else if (name.toUtf8().contains("供电")) {
        f |= W::PowerSupply;
    } else if (name.toUtf8().contains("SOH")) {
        f |= W::Soh;
    } else if (name.toUtf8().contains("异常高温")) {
        f |= W::AbnormalTemp;
    } else if (name.toUtf8().contains("断线")) {
        f |= W::Disconnect;
    } else if (name.toUtf8().contains("采样") || name.toUtf8().contains("采集")) {
        f |= W::Sample;
    } else if (name.toUtf8().contains("流")) {
        f |= W::Current;
    } else if (name.toUtf8().contains("压")) {
        f |= W::Voltage;
    } else if (name.toUtf8().contains("温")) {
        f |= W::Temperature;
    } else {
        return -1;
    }

    qDebug() << __func__ << name.toUtf8()
             << QString("Flag(Bin)= 0x%1").arg(f, 16, 2, QChar('0'))
             << QString("Flag(Hex)= 0x%1").arg(f, 4, 16, QChar('0'));
    return f;
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

QList<int> ParamLoader::getIdListByFlag(unsigned flag)
{
    return wnFlag.values(flag);
}
