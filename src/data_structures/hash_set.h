#ifndef HASH_SET_H
#define HASH_SET_H

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "SDL_assert.h"

#include "dynamic_array.h"
#include "../common.h"

#define DYNAMIC_ARRAY_CAPACITY 4

template<typename K>
struct Element
{
    bool occupied;
    K key;
};

// Hash set
template<typename K>
struct HashSet
{
    int nElems;
    int capacity;
    Arena *arena;
    DynamicArray<Element<K>> elems;

    float load_factor();

    bool contains(K *key);
    bool contains(K key);
    void add(K *key);
    void add(K key);
    bool remove(K *key);

    int compute_hash(K key);
    int double_capacity();
};

template<typename K>
void reset_hash_set(HashSet<K> *set)
{
    set->nElems = 0;
    set->elems.size = set->capacity;

    for (int i=0; i<set->elems.size; ++i)
    {
        set->elems.data[i].occupied = false;
    }
}

template<typename K>
void init_hash_set(HashSet<K> *set, int capacity, Arena *arena=NULL)
{
    set->capacity = capacity;
    set->arena = arena;
    init_dynamic_array(&set->elems, capacity, false, arena);
    reset_hash_set(set);
}

template<typename K>
void delete_hash_set(HashSet<K> *set)
{
    set->capacity = 0;
    delete_dynamic_array(&set->elems);
}

template<typename K>
float HashSet<K>::load_factor()
{
    return float(nElems)/float(capacity);
}

template<typename K>
bool HashSet<K>::contains(K *key)
{
    int hash = compute_hash(*key);
    SDL_assert(hash < capacity);

    for (int i=hash; i<elems.size + hash; ++i)
    {
        Element<K> *elem = &elems.data[i % elems.size];
        if (!elem->occupied)
        {
            return false;
        }
        else if (*key == elem->key)
        {
            return true;
        }
    }
    return false;
}

template<typename K>
bool HashSet<K>::contains(K key)
{
    return contains(&key);
}

template<typename K>
void HashSet<K>::add(K *key)
{
    if (load_factor() > 0.5)
    {
        double_capacity();
    }

    int hash = compute_hash(*key);

    int i=hash;
    for (; i<elems.size + hash; ++i)
    {
        Element<K> *elem = &elems.data[i % elems.size];
        if (!elem->occupied)
        {
            break;
        }
        else if (*key == elem->key)
        {
            return;
        }
    }

    Element<K> *elem = &elems.data[i];
    SDL_assert(elem->occupied == false);

    elem->occupied = true;
    elem->key = *key;
    ++nElems;

    return;
}

template<typename K>
void HashSet<K>::add(K key)
{
    return add(&key);
}

template<typename K>
bool HashSet<K>::remove(K *key)
{
    int hash = compute_hash(*key);

    for (int i=hash; i<elems->size + hash; ++i)
    {
        Element<K> *elem = &elems->data[i % elems.size];
        if (!elem->occupied)
        {
            return false;
        }
        else if (*key == elem->key)
        {
            elem->occupied = false;
            return true;
        }
    }
    return false;
}

template<typename K>
int HashSet<K>::compute_hash(K key)
{
    uint32_t hash = hash_bytes((uint8_t *) &key, sizeof(K));
    return (int) (hash % capacity);
}

template<typename K>
int HashSet<K>::double_capacity()
{
    printf("Hash set double capacity\n");
    int oldCapacity = capacity;
    Element<K> *oldItems = elems.data;

    capacity *= 2;
    init_dynamic_array<Element<K>>(&elems, capacity, false, arena);
    reset_hash_set(this);

    // Recompute hashes
    for (int i=0; i<oldCapacity; ++i)
    {
        Element<K> *elem = &oldItems[i];
        if (elem->occupied)
        {
            add(elem->key);
        }
    }

    arena_free(arena, oldItems);
    return capacity;
}

#endif //HASH_SET_H