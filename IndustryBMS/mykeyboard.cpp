#include "mykeyboard.h"
#include "ui_mykeyboard.h"
#include <QDesktopWidget>
#include <QApplication>
#include <QDebug>
#include <QComboBox>
#include <QKeyEvent>
#include <QFile>
#include <QMouseEvent>

Mykeyboard::Mykeyboard(QWidget *parent, QLineEdit::EchoMode echoMode) :
    QDialog(parent),
    ui(new Ui::Mykeyboard)
{
    ui->setupUi(this);

//    this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint); //Qt::WindowStaysOnTopHint |
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    ui->btn_move->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    ui->input_lineedit->setEchoMode(echoMode);

#if (defined(WEIQIAN) || defined(WZD)) && !defined(FONT_POINT_SIZE_TEST)
    calibrateFontSize(0);
#elif defined(YCTEK) && !defined(FONT_POINT_SIZE_TEST)
    calibrateFontSize(11);
#endif

    deskWidth =  QApplication::desktop()->width();
    deskHeight = QApplication::desktop()->height();
    frmWidth = this->width();
    frmHeight = this->height();

    is_exit = false;
    init();
}

Mykeyboard::~Mykeyboard()
{
    delete ui;
}

#if !defined(FONT_POINT_SIZE_TEST)
void Mykeyboard::calibrateFontSize(int increment)
{
    QWidget *w = NULL;
    QGroupBox *b = NULL;
    QList<QObject *> objList = this->findChildren<QObject *>();

    foreach (QObject *obj, objList) {
        if(obj->inherits("QWidget"))
        {
            w = qobject_cast<QWidget *>(obj);

            /* number 4 is experimental value, define FONT_POINT_SIZE_TEST then use
               reset button and reboot button to tweak font size, mainwindow.ui has no
               layout, so we need to change font size one by one. */

            QFont font("BENMO Jingyuan Regular", w->font().pointSize() + increment);

            w->setFont(font);

            if(0 == qstrcmp(obj->metaObject()->className(), "QGroupBox"))
            {
                b = qobject_cast<QGroupBox *>(obj);

                QList<QObject *> childrenOfBox = b->findChildren<QObject *>();

                foreach (QObject *child, childrenOfBox) {
                    if(child->inherits("QWidget"))
                    {
                        w = qobject_cast<QWidget *>(child);

                        QFont font("BENMO Jingyuan Regular", w->font().pointSize());
                        w->setFont(font);
                    }
                }
            }
        }
    }

    update();
}
#endif

/*实现窗口移动*/
void Mykeyboard::mousePressEvent(QMouseEvent *e)
{
    if(e->button() != Qt::LeftButton)
        return;
    int x = ui->btn_move->x();
    int y = ui->btn_move->y();
    int rx = ui->btn_move->width();
    int ry = ui->btn_move->height();
    int cur_x = e->pos().x();
    int cur_y = e->pos().y();
    if(cur_x>x && cur_y>y && cur_x<rx+x && cur_y<ry+y)
    {
        _is_down = true;
        _point = e->globalPos();
    }

}
void Mykeyboard::mouseMoveEvent(QMouseEvent *e)
{
    if(_point.isNull())
        return;

    if(_is_down)
    {
        QPoint n_pos = e->globalPos();
        QPoint update_pos = mapToParent(n_pos - _point);
        move(update_pos);
        _point = n_pos;
    }

}

void Mykeyboard::mouseReleaseEvent(QMouseEvent *)
{
    _is_down = false;
}

void Mykeyboard::keyReleaseEvent(QKeyEvent *e)
{
    if (Qt::Key_Enter == e->key() || Qt::Key_Return == e->key()) {
        ui->btn_enter->click();
    }
}

void Mykeyboard::btn_click()
{
        QPushButton *btn = (QPushButton *)sender();
        QString objectName = btn->objectName();

        if (objectName == "btn_del") {
            ui->input_lineedit->backspace();
        } else if (objectName == "btn_close") {
            is_exit = true;
            currentLineEdit->setFocusPolicy(Qt::ClickFocus);
            this->close();
        } else if (objectName == "btn_enter") {
            currentLineEdit->setText( ui->input_lineedit->text() );
            QKeyEvent enterkey(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
            QCoreApplication::sendEvent( currentLineEdit, &enterkey );
            this->close();
        } else if (objectName == "btn_clear") {
            ui->input_lineedit->clear();
        } else {
            QString value = btn->text();
            if (value == "-") {
                QString text = ui->input_lineedit->text();
                int index = ui->input_lineedit->cursorPosition();
                if (text.left(1) == "-") {
                    text.remove(0, 1);
                    ui->input_lineedit->setText(text);
                    ui->input_lineedit->setCursorPosition(index-1);
                } else {
                    if (text == "0" || text == "") {
                        ui->input_lineedit->setCursorPosition(index);
                    } else {
                        text.insert(0, '-');
                        ui->input_lineedit->setText(text);
                        ui->input_lineedit->setCursorPosition(index+1);

                    }
                }
            } else {
                ui->input_lineedit->insert(value);
            }
        }
}

bool Mykeyboard::eventFilter(QObject *obj, QEvent *e)
{
    QString className = obj->metaObject()->className();

    if (0 == className.compare("QLineEdit")) {
        QLineEdit *edit = qobject_cast<QLineEdit *>(obj);

        if (edit->objectName() == "input_lineedit") {
//            return QObject::eventFilter(obj, e);
            return false;
        }

        if (e->type() == QEvent::MouseButtonPress && edit->isEnabled()) {
            currentLineEdit = (QLineEdit *)obj;
            ui->input_lineedit->setText(currentLineEdit->text());
            ui->input_lineedit->setFocus();

            int x = (deskWidth-(QCursor::pos().x()+(edit->rect().width()/2))) < frmWidth ?
                        (QCursor::pos().x()-(frmWidth+edit->rect().width())): (QCursor::pos().x()+(edit->rect().width()/2));
            int y = 150;
            this->move(x, y);
            this->repaint();
            this->exec();
            return true;
        } else {
            return QObject::eventFilter(obj, e);
        }
    } else {
        return QObject::eventFilter(obj, e);
    }
}

void Mykeyboard::init()
{
    currentLineEdit = 0;
    mousePressed = false;
    currentType = "min";

    QList<QPushButton *> btn = this->findChildren<QPushButton *>();
    foreach (QPushButton * b, btn) {
        b->setAutoDefault(false);
        connect(b, SIGNAL(clicked()), this, SLOT(btn_click()));
    }

//    qApp->installEventFilter(this);
}
