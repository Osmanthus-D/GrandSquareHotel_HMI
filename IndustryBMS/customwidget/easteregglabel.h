#ifndef EASTEREGGLABEL_H
#define EASTEREGGLABEL_H

#include <QLabel>
#include <QTimer>
#include <QMouseEvent>
#include <QDebug>

#define EASTER_EGG_TRIGGER_NUM          7
#define EASTER_EGG_TRIGGER_INTERVAL     500

class EasterEggLabel : public QLabel
{
    Q_OBJECT
public:
    explicit EasterEggLabel(QWidget *parent = NULL);

signals:
    void triggered();

protected:
    virtual void mousePressEvent(QMouseEvent *event);

private:
    int count;
    QTimer timer;

private slots:
    void reset();
};

#endif // EASTEREGGLABEL_H
