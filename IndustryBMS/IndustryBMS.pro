# -------------------------------------------------
# Project created by QtCreator 2019-05-08T14:58:22
# -------------------------------------------------
TARGET = IndustryBMS

TEMPLATE = app

QT += testlib network \
    widgets

DEFINES += CAN_BUS_MONITOR
DEFINES += WEIQIAN
#DEFINES += XINLONG
#DEFINES += YCTEK

if(contains(DEFINES, WEIQIAN)){
    target.path = /home/asd
    INSTALLS += target
}

QMAKE_CXXFLAGS += -std=c++0x

SOURCES += main.cpp \
    bitcalculator.cpp \
    portmgrdialog.cpp \
    style/qembededplastiquestyle.cpp \
    customwidget/customlabel.cpp \
    customwidget/easteregglabel.cpp \
    customwidget/switchbutton.cpp \
    customwidget/tumbler.cpp \
    qcustomplot/qcustomplot.cpp \
    factorydialog.cpp \
    filecopier.cpp \
    mainwindow.cpp \
    login.cpp \
    hmi.cpp \
    comm.cpp \
    bgworker.cpp \
    config.cpp \
    mykeyboard.cpp \
    mymodel.cpp\
    paramloader.cpp \
    systemtypedialog.cpp \
    uihelper.cpp \
    hisrecordmodel.cpp \
    weiqianfunctions.cpp

HEADERS += mainwindow.h \
    ExampleKbdHandler.h \
    ExtMsgType.h \
    base.h \
    bitcalculator.h \
    callback.h \
    portmgrdialog.h \
    style/qembededplastiquestyle.h \
    customwidget/customlabel.h \
    customwidget/easteregglabel.h \
    customwidget/switchbutton.h \
    customwidget/tumbler.h \
    qcustomplot/qcustomplot.h \
    factorydialog.h \
    filecopier.h \
    global.h \
    login.h \
    hmi.h \
    bms.h \
    module.h \
    comm.h \
    bgworker.h \
    config.h \
    para.h \
    mykeyboard.h \
    mymodel.h \
    paramloader.h \
    systemtypedialog.h \
    uihelper.h \
    hisrecordmodel.h \
    weiqianfunctions.h

FORMS += mainwindow.ui \
    factorydialog.ui \
    login.ui \
    mykeyboard.ui \
    systemtypedialog.ui

LIBS += -L../3rd/libmodbus/lib -lmodbus \
    -L../3rd/libserialport/lib -lserialport \
    -L../3rd/libdlt645/lib/ -lDLT645 \
    -L../3rd/qslog/lib -lQsLog \
    ../3rd/iec104slave/lib/104slave.a \
    -L/usr/local/Trolltech/Qt-4.8.5/lib/libQtSerialPort.so
#    -lQtSerialPort
#    ../3rd/iec104slave/lib/libplaincprj.a

INCLUDEPATH += ../3rd/libmodbus/include \
    ../3rd/libserialport/include \
    ../3rd/iec104slave/include \
    ../3rd/libdlt645/include \
    ../3rd/qslog/include \
    customwidget \
    qcustomplot \
    style \
    animation

RESOURCES += res.qrc

OTHER_FILES += zh_CN.ts \
    Changes.txt

TRANSLATIONS += zh_CN.ts

unix {
    linux-arm-g++ {
        message(arm_linux)
        DEFINES += ARM_LINUX
#        DEFINES += FONT_POINT_SIZE_TEST
    }
}

include(animation/animationwidget.pri)
