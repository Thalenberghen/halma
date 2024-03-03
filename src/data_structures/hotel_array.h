#ifndef HOTEL_ARRAY_H
#define HOTEL_ARRAY_H

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "SDL_assert.h"

// Generational index
struct GenId
{
    int generation;
    int id;
};

// Hotel room
template<typename T>
struct HotelRoom
{
    GenId index;
    bool occupied;
    T data;
};

template<typename T>
void init_hotel_room(HotelRoom<T> *room, int index)
{
    room->index = {-1, index};
    room->occupied = false;
}

// Hotel floor
template<typename T>
struct HotelFloor
{
    int index;
    int capacity;
    HotelRoom<T> *rooms;
    HotelFloor<T> *next;
};

template<typename T>
void init_hotel_floor(HotelFloor<T> *floor, int capacity, int index)
{
    floor->index = index;
    floor->next = NULL;
    floor->capacity = capacity;

    floor->rooms = (HotelRoom<T> *) malloc(capacity*sizeof(HotelRoom<T>));
    for (int i=0; i<capacity; ++i)
    {
        init_hotel_room(&floor->rooms[i], capacity*index + i);
    }
}

template<typename T>
void delete_hotel_floor(HotelFloor<T> *floor)
{
    floor->capacity = 0;
    free(floor->rooms);
}

template<typename T>
struct HotelArray
{
    int floorCapacity;
    int nFloors;
    int nOccupied;
    HotelFloor<T> *firstFloor;
    HotelFloor<T> *lastFloor;

    int n_rooms();
    int add_floor();
    HotelRoom<T>* place(T *item);
    int clear(HotelRoom<T> *room);
    int clear(GenId roomIndex);
    int clear_floor(HotelFloor<T> *floor);

    HotelFloor<T>* on_floor(int roomNumber);
    HotelRoom<T>* get_room(GenId roomIndex);
    HotelRoom<T>* get_room();
    HotelRoom<T>* first_room();
    HotelRoom<T>* first_occupied_room();
    HotelRoom<T>* next_room(HotelRoom<T> *room);
    HotelRoom<T>* next_occupied_room(HotelRoom<T> *room);
};

template <typename T>
void init_hotel_array(HotelArray<T>* array, int floorCapacity)
{
    SDL_assert(floorCapacity > 0);
    array->floorCapacity = floorCapacity;
    array->nFloors = 1;
    array->nOccupied = 0;
    array->firstFloor = (HotelFloor<T> *) malloc(sizeof(HotelFloor<T>));
    SDL_assert(array->firstFloor != NULL);
    init_hotel_floor<T>(array->firstFloor, floorCapacity, 0);
    array->lastFloor = array->firstFloor;
}

template <typename T>
void delete_hotel_array(HotelArray<T>* array)
{
    HotelFloor<T> *floor = array->firstFloor;
    HotelFloor<T> *next = NULL;

    while (floor != NULL)
    {
        next = floor->next;
        delete_hotel_floor(floor);
        floor = next;
    }
    array->firstFloor = NULL;
    array->lastFloor = NULL;
    array->nOccupied = 0;
}

template <typename T>
void reset_hotel_array(HotelArray<T>* array)
{
    HotelFloor<T> *floor = array->firstFloor;
    HotelFloor<T> *next = NULL;

    while (floor != NULL)
    {
        next = floor->next;
        array->clear_floor(floor);
        floor = next;
    }
    array->nOccupied = 0;
}

template <typename T>
int HotelArray<T>::n_rooms()
{
    return nFloors*floorCapacity;
}

template <typename T>
int HotelArray<T>::add_floor()
{
    HotelFloor<T> *newFloor = (HotelFloor<T> *) malloc(sizeof(HotelFloor<T>));
    init_hotel_floor<T>(newFloor, floorCapacity, nFloors);
    lastFloor->next = newFloor;
    lastFloor = newFloor;
    SDL_assert(lastFloor->next == NULL);
    ++nFloors;
    return nFloors;
}

template <typename T>
HotelFloor<T>* HotelArray<T>::on_floor(int roomNumber)
{
    HotelFloor<T> *floor = firstFloor;
    int nFloors = roomNumber/floorCapacity;
    for (int i=0; i < nFloors; ++i)
    {
        // SDL_assert(floor->next != NULL);
        // SDL_assert(floor != lastFloor);
        floor = floor->next;
    }
    return floor;
}

template<typename T>
HotelRoom<T>* HotelArray<T>::first_room()
{
    SDL_assert(firstFloor != NULL);

    if (firstFloor->next != NULL)
    {
        SDL_assert(firstFloor != lastFloor);
    }

    return &firstFloor->rooms[0];
}

template<typename T>
HotelRoom<T>* HotelArray<T>::first_occupied_room()
{
    HotelRoom<T> *room = first_room();
    if (room->occupied)
    {
        return room;
    }
    return next_occupied_room(room);
}

template<typename T>
HotelRoom<T>* HotelArray<T>::next_room(HotelRoom<T> *room)
{
    HotelFloor<T> *floor = on_floor(room->index.id);

    while (true)
    {
        if (room->index.id % floorCapacity == floorCapacity - 1)
        {
            if (floor->next == NULL)
            {
                return NULL;
            }
            SDL_assert(floor != lastFloor);
            floor = floor->next;
            room = &floor->rooms[0];
        }
        else
        {
            ++room;
        }
        return room;
    }
}

template<typename T>
HotelRoom<T>* HotelArray<T>::next_occupied_room(HotelRoom<T> *room)
{
    HotelFloor<T> *floor = on_floor(room->index.id);
    while (true)
    {
        if (room->index.id % floorCapacity == floorCapacity - 1)
        {
            if (floor->next == NULL)
            {
                return NULL;
            }
            SDL_assert(floor != lastFloor);
            floor = floor->next;
            room = floor->rooms;
        }
        else
        {
            ++room;
        }
        if (room->occupied)
        {
            return room;
        }
    }
}

#define for_all_occupied(T, hotelArray, room) for (HotelRoom<T> *room = (hotelArray)->first_occupied_room(); room != NULL; room = (hotelArray)->next_occupied_room(room))
#define for_all(T, hotelArray, room) for (HotelRoom<T> *room = (hotelArray)->first_room(); room != NULL; room = (hotelArray)->next_room(room))

template<typename T>
HotelRoom<T>* HotelArray<T>::place(T *item)
{
    HotelRoom<T> *room = get_room();
    room->data = *item;
    return room;
}

template<typename T>
int HotelArray<T>::clear(HotelRoom<T> *room)
{
    SDL_assert(room != NULL);

    if (room->occupied)
    {
        room->occupied = false;
        nOccupied -= 1;
    }
    return nOccupied;
}

template<typename T>
int HotelArray<T>::clear(GenId roomIndex)
{
    HotelRoom<T> *room = get_room(roomIndex);
    return clear(room);
}

template<typename T>
int HotelArray<T>::clear_floor(HotelFloor<T> *floor)
{
    for (int i=0; i<floor->capacity; ++i)
    {
        clear(&floor->rooms[i]);
    }
    return nOccupied;
}

template<typename T>
HotelRoom<T> *HotelArray<T>::get_room(GenId roomIndex)
{
    SDL_assert(roomIndex.id > -1);
    HotelFloor<T> *floor = firstFloor;
    for (int i=0; i < roomIndex.id/floorCapacity; ++i)
    {
        SDL_assert(floor->next != NULL);
        floor = floor->next;
    }
    SDL_assert(roomIndex.id < (floor->index + 1)*floorCapacity);

    HotelRoom<T> *room = &floor->rooms[roomIndex.id % floorCapacity];
    if (roomIndex.generation == room->index.generation)
    {
        return room;
    }
    return NULL;
}

template<typename T>
HotelRoom<T> *HotelArray<T>::get_room()
{
    if (this->nOccupied == n_rooms())
    {
        add_floor();
    }

    for_all(T, this, room)
    {
        if (!room->occupied)
        {
            room->occupied = true;
            room->index.generation += 1;
            this->nOccupied += 1;
            return room;
        }
    }
    return NULL;
}

#endif //HOTEL_ARRAY_H