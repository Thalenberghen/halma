#ifndef STATIC_ARRAY_H
#define STATIC_ARRAY_H

#include "stdlib.h"
#include "stdio.h"

#include "SDL_assert.h"

template<typename T, int32_t N>
struct StaticArray
{
    T data[N];

    T& operator[](int32_t i)
    {
        return data[i];
    }
    const T& operator[](int32_t i) const
    {
        return data[i];
    }

    int32_t size()
    {
        return N;
    }
};

#endif