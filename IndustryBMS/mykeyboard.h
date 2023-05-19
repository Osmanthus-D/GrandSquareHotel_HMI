#ifndef MYKEYBOARD_H
#define MYKEYBOARD_H

#include <QDialog>
#include <QGroupBox>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QMouseEvent>

namespace Ui {
class Mykeyboard;
}

class Mykeyboard : public QDialog
{
    Q_OBJECT

public:
    explicit Mykeyboard(QWidget *parent = NULL, QLineEdit::EchoMode echoMode = QLineEdit::Normal);
    ~Mykeyboard();

private slots:
    void btn_click(); //按钮被点击事件
    bool eventFilter(QObject *obj, QEvent *e);

private:
    Ui::Mykeyboard *ui;
    void init();

private:
    QLineEdit *currentLineEdit;     //当前焦点的文本框
    QString currentType;            //当前输入法类型
    bool mousePressed;                //是否按下


    int deskWidth; //屏幕宽度高度， 以及自身宽度高度
    int deskHeight;
    int frmWidth;
    int frmHeight;
    int _cur_pos;  //编辑框中, 当前光标的位置
    bool is_exit; //是否要退出 如果是true则表示要退出；

#if !defined(FONT_POINT_SIZE_TEST)
    void calibrateFontSize(int increment);
#endif

protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void keyReleaseEvent(QKeyEvent *e);

private:
    bool _is_down;
    QPoint _point;
};

#endif // MYKEYBOARD_H
