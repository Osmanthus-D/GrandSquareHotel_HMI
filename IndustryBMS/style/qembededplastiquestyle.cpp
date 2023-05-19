#include "qembededplastiquestyle.h"

QEmbededPlastiqueStyle::QEmbededPlastiqueStyle()
{

}

int QEmbededPlastiqueStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
#if 1
    if (PM_ScrollBarExtent == metric) {
        return 24;
    } else {
        return QPlastiqueStyle::pixelMetric(metric, option, widget);
    }
#else
    return QPlastiqueStyle::pixelMetric(metric, option, widget);
#endif
}
