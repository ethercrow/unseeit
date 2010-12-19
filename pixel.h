#ifndef UNSEEIT_PIXEL_H
#define UNSEEIT_PIXEL_H

inline int ssd4(quint8* a, quint8* b)
{
    int result = 0;
    for (int c=0; c<4; ++c) {
        int d = ((int)*a-(int)*b);
        result += d*d;
        ++a;
        ++b;
    }
    return result;
}

#endif
