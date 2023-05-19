#ifndef EXTMSGTYPE_H
#define EXTMSGTYPE_H

#include <limits.h>

struct ExtMsgFlag
{
public:
    static unsigned getFlag()
    {
        return flag;
    }

    static int toggleBit(unsigned i)
    {
        int ret = -1;

        if (i < CHAR_BIT * sizeof(flag)) {
            ret = 0;
            flag |= 0x01 << i;
        }

        return ret;
    }

    static int clearBit(unsigned i)
    {
        int ret = -1;

        if (i < CHAR_BIT * sizeof(flag)) {
            ret = 0;
            flag &= ~(0x01 << i);
        }

        return ret;
    }

    static bool bit(unsigned i)
    {
        bool ret = false;

        if (i < CHAR_BIT * sizeof(flag)) {
            ret = 0 != (flag >> i & 0x01);
        }

        return ret;
    }

private:
    static unsigned flag;
};

#endif // EXTMSGTYPE_H
