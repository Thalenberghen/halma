#ifndef COUNT_TABLE_H
#define COUNT_TABLE_H

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "SDL_assert.h"

#include "hash_table.h"

// CountTable
template<typename E>
struct CountTable
{
    HashTable<E, int> entries;

    float load_factor();
    int get_value(E *entry);
    int increment_value(E *entry);
};

template<typename E>
void init_count_table(CountTable<E>* table, int capacity, Arena *arena=NULL)
{
    init_hash_table(&table->entries, capacity, arena);
}

template<typename E>
void delete_count_table(CountTable<E> *table)
{
    delete_hash_table(&table->entries);
}

template<typename E>
float CountTable<E>::load_factor()
{
    return entries.load_factor();
}

template<typename E>
int CountTable<E>::get_value(E *entry)
{
    int *value = entries.get_or_default_value(entry, 0);
    return *value;
}

template<typename E>
int CountTable<E>::increment_value(E *entry)
{
    int *value = entries.get_or_default_value(entry, 0);
    return *value = *value + 1;
}

#endif //COUNT_TABLE_H