#include "easteregglabel.h"

EasterEggLabel::EasterEggLabel(QWidget *parent) : QLabel(parent)
{
    count = 0;
    timer.setSingleShot(true);

    connect(&timer, SIGNAL(timeout()), this, SLOT(reset()));
}

void EasterEggLabel::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        if(EASTER_EGG_TRIGGER_NUM > count + 1)
        {
            ++count;

            if(timer.isActive()) {
                timer.stop();
            }

            timer.start(EASTER_EGG_TRIGGER_INTERVAL);
        }
        else {
            emit triggered();
            count = 0;
        }
    }

    QLabel::mousePressEvent(event);
}

void EasterEggLabel::reset()
{
    qDebug() << __func__;
    count = 0;
}
