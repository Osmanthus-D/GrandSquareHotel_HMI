#ifndef CALLBACK_H
#define CALLBACK_H

#include <QString>

class Callback
{
public:
    Callback() {}
    ~Callback() {}

    virtual unsigned getN() const = 0;
    virtual int getChargeCurrentMax(unsigned n) const = 0;
    virtual int getDischargeCurrentMax(unsigned n) const = 0;
    virtual QString getWarningDesc(int id) const = 0;
    virtual QList<unsigned> getCheckList() const = 0;
    virtual void setCheckList(QList<unsigned>) = 0;
};
#endif // CALLBACK_H
