#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "SDL_assert.h"

#include "dynamic_array.h"
#include "../common.h"

template<typename K, typename V>
struct Item
{
    bool occupied;
    K key;
    V value;
};

// Hash table
template<typename K, typename V>
struct HashTable
{
    int32_t nOccupied;
    int32_t capacity;
    Arena *arena;
    DynamicArray<Item<K, V>> items;

    float load_factor();

    V *get_value(K *key);
    V get_value(K key);
    V *set_value(K *key, V *value);
    V *set_value(K *key, V value);
    V *set_value(K key, V value);
    V *get_or_default_value(K *key, V *default_value);
    V *get_or_default_value(K *key, V default_value);
    bool remove(K *key);

    int32_t compute_hash(K key);
    int32_t double_capacity();
};

template<typename K, typename V>
void reset_hash_table(HashTable<K, V> *table)
{
    table->nOccupied = 0;
    table->items.size = table->capacity;

    for (int32_t i=0; i<table->items.size; ++i)
    {
        table->items.data[i].occupied = false;
    }
}

template<typename K, typename V>
void init_hash_table(HashTable<K, V> *table, int32_t capacity, Arena *arena=NULL)
{
    table->capacity = capacity;
    table->arena = arena;
    init_dynamic_array(&table->items, capacity, false, arena);
    reset_hash_table(table);
}

template<typename K, typename V>
void delete_hash_table(HashTable<K, V> *table)
{
    table->capacity = 0;
    delete_dynamic_array(&table->items);
}

template<typename K, typename V>
float HashTable<K, V>::load_factor()
{
    return float(nOccupied)/float(capacity);
}

template<typename K, typename V>
V *HashTable<K, V>::get_value(K *key)
{
    int32_t hash = compute_hash(*key);
    SDL_assert(hash < capacity);

    for (int32_t i=hash; i<items.size + hash; ++i)
    {
        Item<K, V> *item = &items.data[i % items.size];
        if (!item->occupied)
        {
            return NULL;
        }
        else if (*key == item->key)
        {
            return &item->value;
        }
    }
    return NULL;
}

template<typename K, typename V>
V HashTable<K, V>::get_value(K key)
{
    V *value = get_value(&key);
    if (value == NULL)
    {
        printf("ERROR] key '%d' not available\n", key);
        SDL_assert(false);
    }
    return *value;
}

template<typename K, typename V>
V* HashTable<K, V>::set_value(K *key, V *value)
{
    if (load_factor() > 0.5)
    {
        double_capacity();
    }

    int32_t hash = compute_hash(*key);

    int32_t i = hash;
    for (; i<items.size + hash; ++i)
    {
        Item<K, V> *item = &items.data[i % items.size];
        if (!item->occupied)
        {
            break;
        }
        else if (*key == item->key)
        {
            item->value = *value;
            return &item->value;
        }
    }

    Item<K, V> *item = &items.data[i];
    SDL_assert(item->occupied == false);

    item->occupied = true;
    item->key = *key;
    item->value =  *value;

    ++nOccupied;
    return &item->value;
}

template<typename K, typename V>
V* HashTable<K, V>::set_value(K key, V value)
{
    return set_value(&key, &value);
}

template<typename K, typename V>
V* HashTable<K, V>::set_value(K *key, V value)
{
    return set_value(key, &value);
}

template<typename K, typename V>
V* HashTable<K, V>::get_or_default_value(K *key, V *default_value)
{
    if (load_factor() > 0.5)
    {
        double_capacity();
    }

    int32_t hash = compute_hash(*key);

    int32_t i = hash;
    for (; i<items.size + hash; ++i)
    {
        Item<K, V> *item = &items.data[i % items.size];
        if (!item->occupied)
        {
            break;
        }
        else if (*key == item->key)
        {
            SDL_assert(&item->value != NULL);
            return &item->value;
        }
    }

    Item<K, V>* item = &items.data[i];
    SDL_assert(item->occupied == false);

    item->occupied = true;
    item->key = *key;
    item->value = *default_value;

    ++nOccupied;
    return &item->value;
}

template<typename K, typename V>
V* HashTable<K, V>::get_or_default_value(K *key, V default_value)
{
    return get_or_default_value(key, &default_value);
}

template<typename K, typename V>
bool HashTable<K, V>::remove(K *key)
{
    int32_t hash = compute_hash(*key);

    for (int32_t i=hash; i<items.size + hash; ++i)
    {
        Item<K, V> *item = &items.data[i % items.size];
        if (!item->occupied)
        {
            return false;
        }
        else if (*key == item->key)
        {
            item->occupied = false;
            return true;
        }
    }
    return false;
}

template<typename K, typename V>
int32_t HashTable<K, V>::compute_hash(K key)
{
    uint32_t hash = hash_bytes((uint8_t *) &key, sizeof(K));
    return (int32_t) (hash % capacity);
}

template<typename K, typename V>
int32_t HashTable<K, V>::double_capacity()
{
    printf("Hash table double capacity\n");
    int32_t oldCapacity = capacity;
    Item<K, V> *oldItems = items.data;

    capacity *= 2;
    init_dynamic_array<Item<K, V>>(&items, capacity, false, arena);
    reset_hash_table(this);

    // Recompute hashes
    for (int32_t i=0; i<oldCapacity; ++i)
    {
        Item<K, V> *item = &oldItems[i];
        if (item->occupied)
        {
            set_value(item->key, item->value);
        }
    }

    arena_free(arena, oldItems);
    return capacity;
}

#endif //HASH_TABLE_H