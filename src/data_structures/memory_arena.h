#ifndef MEMORY_ARENA_H
#define MEMORY_ARENA_H

#include "stdlib.h"
#include "stdio.h"
#include "stdint.h"
#include "string.h"

#include "SDL_assert.h"

#include "dynamic_array.h"

#define DEFAULT_ALIGNMENT 2*sizeof(void *)

static bool is_power_of_two(uintptr_t x)
{
	return (x & (x - 1)) == 0;
}

static uintptr_t align_forward(uintptr_t pointer, size_t alignment)
{
	SDL_assert(is_power_of_two(alignment));

	uintptr_t aligned = pointer;

    // Same as (pointer % alignment) but faster as power of two
	uintptr_t leftOver = pointer & (alignment - 1);

	if (leftOver != 0)
	{
		// Address was not aligned
		aligned += alignment - leftOver;
	}
	return aligned;
}

// Memory arena for bulk allocating memory
struct Arena
{
	char *data;
	int nAllocations;
	int nFrees;
	size_t capacity;
	DynamicArray<size_t> offsets;
};

static void init_arena(Arena *arena, int capacity)
{
	printf("Init Arena with %d bytes\n", capacity);
    arena->capacity = capacity;
	arena->nAllocations = 0;
	arena->nFrees = 0;
    arena->data = (char *) calloc(capacity, 1);
	SDL_assert(arena->data != NULL);
	init_dynamic_array(&arena->offsets, 1024, true);
	arena->offsets.append(size_t(0));
}

static void free_arena(Arena *arena)
{
	// printf("free arena with offset %zu %d cap %zu\n", arena->currentOffset, arena->nAllocations, arena->capacity);
	arena->nAllocations = 0;
	arena->nFrees = 0;

	// Keep the initial 0
	arena->offsets.size = 1;

	// Debug
	//memset(arena->data, 0, arena->capacity);
}

static void delete_arena(Arena *arena)
{
	free_arena(arena);
	delete_dynamic_array(&arena->offsets);
    free(arena->data);
}

static void *arena_alloc(Arena *arena, size_t size, size_t alignment)
{
	if (arena == NULL)
	{
		return malloc(size);
	}

	// printf("Arena alloc %zu bytes %d d\n", size, arena->nAllocations);

	SDL_assert(size > 0);
	SDL_assert(arena->data != NULL);
	size_t currentOffset = arena->offsets.data[arena->offsets.size - 1];
	uintptr_t currentPointer = (uintptr_t) arena->data + (uintptr_t) currentOffset;
	uintptr_t alignedOffset = align_forward(currentPointer, alignment) - (uintptr_t) arena->data;

	SDL_assert(alignedOffset + size <= arena->capacity);

	arena->nAllocations++;
    arena->offsets.data[arena->offsets.size - 1] = alignedOffset;
    arena->offsets.append(alignedOffset + size);

    void *result = &arena->data[alignedOffset];
    //memset(result, 0, size);  // Zero new memory by default
    return result;
}

static void *arena_alloc(Arena *arena, size_t size)
{
	return arena_alloc(arena, size, DEFAULT_ALIGNMENT);
}

static void *arena_realloc(Arena *arena, void *oldData, size_t oldSize, size_t newSize, size_t alignment)
{
	if (arena == NULL)
	{
		return realloc(oldData, newSize);
	}

	char *oldBytes = (char *) oldData;
	SDL_assert(is_power_of_two(alignment));

	if (oldBytes == NULL || oldSize == 0)
	{
		return arena_alloc(arena, newSize, alignment);
	}

	SDL_assert(arena->data <= oldBytes && oldBytes < arena->data + arena->capacity);
	size_t previousOffset = arena->offsets.data[arena->offsets.size - 2];

	if (arena->data + previousOffset == oldBytes)
	{
		arena->offsets.data[arena->offsets.size - 1] = previousOffset + newSize;
		if (newSize > oldSize)
		{
			// Zero the new memory by default
			memset(&arena->data[previousOffset + oldSize], 0, newSize - oldSize);
		}
		return oldData;
	}
	else
	{
		void *new_memory = arena_alloc(arena, newSize, alignment);
		size_t copySize = oldSize < newSize ? oldSize : newSize;
		memmove(new_memory, oldData, copySize);
		return new_memory;
	}
}

static void *arena_realloc(Arena *arena, void *oldData, size_t oldSize, size_t newSize)
{
	return arena_realloc(arena, oldData, oldSize, newSize, DEFAULT_ALIGNMENT);
}

static void arena_free(Arena *arena, void *data)
{
	if (arena == NULL)
	{
		return free(data);
	}

	SDL_assert(arena->data != NULL);
	size_t previousOffset = arena->offsets.data[arena->offsets.size - 2];
	uintptr_t previousPointer = (uintptr_t) arena->data + (uintptr_t) previousOffset;

	if ((uintptr_t) data == previousPointer)
	{
		arena->nFrees += 1;
		arena->offsets.size -= 1;
	}
	// else
	// {
	// 	printf("Here\n");
	// }
}

#endif //MEMORY_ARENA_H
