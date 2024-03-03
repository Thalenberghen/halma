#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include "stdlib.h"
#include "stdio.h"
#include <cstring>

#include "SDL_assert.h"

#include "../common.h"

struct Arena;

static void *arena_alloc(Arena *arena, size_t size);


// Dynamic array, that doubles in size if capacity is reached
// Don't store pointers into these!

template<typename T>
struct DynamicArray
{
    int32_t size;
    int32_t capacity;
    Arena *arena;
    T *data;
    bool allowedToGrow;

    T& operator[](int32_t id);
    const T& operator[](int32_t id) const;

    T *allocate_data();
    int32_t reserve(int32_t newCapacity);
    int32_t insert(T *element, int32_t index);
    int32_t append(T *element, int32_t n=1);
    int32_t append(T element);
    int32_t set_to(T value);
    void swap(int32_t index1, int32_t index2);
    T *insert_slot(int32_t index);
    T *get_slot();
    int32_t remove(int32_t index);
    int32_t remove(int32_t index, int32_t n);
    int32_t double_capacity();
    int32_t find(T *element);
    int32_t find(T element);
    int32_t append_unique(T *element);
    int32_t append_unique(T element);
    void sort(int (*compFunction)(const void*, const void*));
    void reorder(DynamicArray<IndexedFloat> *ordering, bool inverse);
    void randomize(RandomEngine *random);
    void copy(DynamicArray<T> *target);
};

template<typename T>
void init_dynamic_array(DynamicArray<T> *array, int32_t cap, bool allowedToGrow=false, Arena *arena=NULL)
{
    SDL_assert((cap > 0 || cap == -1) && cap < 100000);
    array->arena = arena;
    array->size = 0;
    array->capacity = cap;
    array->allowedToGrow = allowedToGrow;
    array->data = array->allocate_data();
}

template<typename T>
void init_dynamic_array_to(DynamicArray<T> *array, int32_t cap, T value, bool allowedToGrow=false, Arena *arena=NULL)
{
    init_dynamic_array(array, cap, allowedToGrow, arena);
    array->size = cap;
    array->set_to(value);
}

template<typename T>
void init_dynamic_array_from(DynamicArray<T> *array, int32_t cap, T *data, bool allowedToGrow=false, Arena *arena=NULL)
{
    array->arena = arena;
    array->size = 0;
    array->capacity = cap;
    array->allowedToGrow = allowedToGrow;
    array->data = data;
}

template<typename T>
void copy_dynamic_array(DynamicArray<T> *destArray, DynamicArray<T> *srcArray)
{
    init_dynamic_array(destArray, srcArray->capacity, srcArray->allowedToGrow, srcArray->arena);
    destArray->size = srcArray->size;
    for (int i=0; i<destArray->size; ++i)
    {
        destArray->data[i] = srcArray->data[i];
    }
}

template<typename T>
void delete_dynamic_array(DynamicArray<T> *array)
{
    arena_free(array->arena, array->data);
    array->size = 0;
    array->capacity = 0;
}

template<typename T>
void reset_dynamic_array(DynamicArray<T> *array)
{
    array->size = 0;
}

template<typename T>
void identity_dynamic_array(DynamicArray<T> *array, int32_t n, bool allowedToGrow=false, Arena *arena=NULL)
{
    init_dynamic_array(array, n, allowedToGrow, arena);
    for (T i=0; i<n; ++i)
    {
        array->append(i);
    }
}

template<typename T>
T *DynamicArray<T>::allocate_data()
{
    if (capacity == -1)
    {
        return NULL;
    }

    T *newData = (T *) arena_alloc(arena, capacity*sizeof(T));

    if (newData == NULL)
    {
        printf("[ERROR] Could not allocate %d*%zd bytes of memory\n", capacity, sizeof(T));
        SDL_assert(false);
    }

    return newData;
}

template<typename T>
int32_t DynamicArray<T>::reserve(int32_t newCapacity)
{
    int32_t oldCapacity = capacity;
    capacity = newCapacity;

    T *newData;
    if (arena == NULL)
    {
        newData = (T*) realloc(data, capacity*sizeof(T));
    }
    else
    {
        newData = (T*) arena_realloc(arena, data, oldCapacity*sizeof(T), capacity*sizeof(T));
    }
    if (newData == NULL)
    {
        printf("[ERROR] Could not allocate %d*%zd bytes of memory\n", capacity, sizeof(T));
        SDL_assert(false);
    }
    else
    {
    }
    data = newData;
    return capacity;
}

template<typename T>
int32_t DynamicArray<T>::insert(T *element, int32_t index)
{
    if (!(size < capacity))
    {
        double_capacity();
    }
    SDL_assert(index < size);
    std::memmove(&data[index+1], &data[index], (size - index)*sizeof(T));
    return ++size;
}

template<typename T>
int32_t DynamicArray<T>::append(T *element, int32_t n)
{
    if (!(size + n - 1 < capacity))
    {
        double_capacity();
    }
    std::memcpy(&data[size], element, n*sizeof(T));
    size += n;
    return size;
}

template<typename T>
int32_t DynamicArray<T>::append(T element)
{
    if (!(size < capacity))
    {
        double_capacity();
    }
    data[size] = element;
    return ++size;
}

template<typename T>
int32_t DynamicArray<T>::set_to(T value)
{
    for (int32_t i=0; i<size; ++i)
    {
        data[i] = value;
    }
    return size;
}

template<typename T>
void DynamicArray<T>::swap(int32_t index1, int32_t index2)
{
    T temp = data[index1];
    data[index1] = data[index2];
    data[index2] = temp;
}

template<typename T>
T *DynamicArray<T>::insert_slot(int32_t index)
{
    if (!(size < capacity))
    {
        double_capacity();
    }
    SDL_assert(index <= size);
    std::memmove(&data[index+1], &data[index], (size - index)*sizeof(T));
    ++size;
    return &data[index];
}

template<typename T>
T *DynamicArray<T>::get_slot()
{
    if (!(size < capacity))
    {
        double_capacity();
    }
    ++size;
    return &data[size-1];
}

template<typename T>
int32_t DynamicArray<T>::remove(int32_t index)
{
    for (int32_t i=index; i<size-1; ++i)
    {
        data[i] = data[i+1];
    }
    return --size;
}

template<typename T>
int32_t DynamicArray<T>::remove(int32_t index, int32_t n)
{
    for (int32_t i=index; i<size-n; ++i)
    {
        data[i] = data[i+n];
    }
    return size -= n;
}

template<typename T>
int32_t DynamicArray<T>::double_capacity()
{
    printf("Double dynamic array capacity of %d\n", capacity);
    SDL_assert(allowedToGrow);
    return reserve(2*capacity);
}

template<typename T>
int32_t DynamicArray<T>::find(T *element)
{
    for (int32_t i=0; i<size; ++i)
    {
        if (data[i] == *element)
        {
            return i;
        }
    }
    return -1;
}

template<typename T>
int32_t DynamicArray<T>::find(T element)
{
    return find(&element);
}

template<typename T>
int32_t DynamicArray<T>::append_unique(T element)
{
    if (find(element) < 0)
    {
        return append(element);
    }
    return -1;
}

template<typename T>
int32_t DynamicArray<T>::append_unique(T *element)
{
    if (find(element) < 0)
    {
        return append(element);
    }
    return -1;
}

template<typename T>
void DynamicArray<T>::sort(int (*compFunction)(const void*, const void*))
{
    qsort(data, size, sizeof(T), compFunction);
}

template<typename T>
void DynamicArray<T>::reorder(DynamicArray<IndexedFloat> *ordering, bool inverse)
{
    SDL_assert(ordering->size == size);
    int32_t offset = 0;
    if (inverse)
    {
        offset = size - 1;
    }

    T *newData = allocate_data();
    for (int32_t i=0; i<ordering->size; ++i)
    {
        newData[i] = data[ordering->data[offset - i].index];
    }
    arena_free(arena, data);
    data = newData;
}

template<typename T>
void DynamicArray<T>::randomize(RandomEngine *random)
{
    DynamicArray<int32_t> slots;
    identity_dynamic_array(&slots, size, false, arena);

    for (int32_t i=0; i<size - 1; ++i)
    {
        int32_t rand = rand_i(random, 0, slots.size - 1);
        SDL_assert(slots.data[rand] > -1 && slots.data[rand] < size);

        swap(i, slots.data[rand]);
        slots.remove(rand);
    }
    delete_dynamic_array(&slots);
}

template<typename T>
void DynamicArray<T>::copy(DynamicArray<T> *target)
{
    init_dynamic_array(target, capacity, arena);
    for (int32_t i=0; i<size; ++i)
    {
        target->append(data[i]);
    }
}

#endif //DYNAMIC_ARRAY_H
