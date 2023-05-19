#ifndef QEMBEDEDPLASTIQUESTYLE_H
#define QEMBEDEDPLASTIQUESTYLE_H

#include <QPlastiqueStyle>

class QEmbededPlastiqueStyle : public QPlastiqueStyle
{
    Q_OBJECT

public:
    explicit QEmbededPlastiqueStyle();
    int pixelMetric(PixelMetric metric, const QStyleOption *option = 0, const QWidget *widget = 0) const;

signals:

};

#endif // QEMBEDEDPLASTIQUESTYLE_H
