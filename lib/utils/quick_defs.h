#ifndef QUICK_DEFS_H
#define QUICK_DEFS_H

static inline bool isPowerOfTwo(unsigned long x)
{
    return x && !(x & (x - 1));
}

#endif