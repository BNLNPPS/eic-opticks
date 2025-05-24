#pragma once

#include <math.h>


template<typename T>
void ssincos(const T angle, T& s, T& c)
{
#ifdef __linux
    sincos( angle, &s, &c);
#else
    s = sin(angle);
    c = cos(angle) ;
#endif

}

