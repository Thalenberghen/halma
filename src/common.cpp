#include "stdio.h"
#include "ctype.h"
#include "stdlib.h"
#include "inttypes.h"
#define _USE_MATH_DEFINES
#ifdef _WIN32
#undef M_PI
#endif
#include "math.h"
#include "string.h"
#include <chrono>

#include "SDL.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/constants.hpp"

#include "dynamic_array.h"
#include "memory_arena.h"
#include "common.h"

using namespace glm;

// Color space conversion
vec4 s_rgb_to_linear(ivec4 rgb)
{
    return pow(vec4(rgb)/255.0f, vec4(2.2f));
}

ivec4 linear_to_s_rgb(vec4 linear)
{
    return ivec4(255.0f*pow(linear, vec4(1.0f/2.2f)));
}

// String helpers
char *string_concat(const char *string1, char *string2, Arena *arena)
{
    size_t strLen1 = strlen(string1);
    size_t strLen2 = strlen(string2);
    size_t newLen = (strLen1 + strLen2 + 1);

    char *result = (char *) arena_alloc(arena, newLen*sizeof(char));
    strncpy(result, string1, newLen);

    for (size_t i=0; i<strLen2; ++i)
    {
        result[strLen1 + i] = string2[i];
    }
    result[strLen1 + strLen2] = '\0';
    return result;
}

char *string_r_strip(char *string)
{
    // Strip whitespace chars off end of string, in place
    char *end = string + strlen(string);
    while (end > string && isspace((unsigned char) (*--end)))
    {
        *end = '\0';
    }
    return string;
}

char *string_l_skip(const char *string)
{
    // Return pointer to first non-whitespace char in string
    while (*string && isspace((unsigned char) (*string)))
    {
        string++;
    }
    return (char*) string;
}

char *string_sanitize(char *string)
{
    // Sanitize string in place
    char *start = string;

    while (*start)
    {
        if (*start == '"' || *start == '\'')
        {
            *start ='$';
        }
        start++;
    }
    return string;
}

char *string_copy_0(char *dest, const char *src, size_t size)
{
    // Version of strncpy that ensures dest is 0-terminated
    strncpy(dest, src, size);
    dest[size - 1] = '\0';
    return dest;
}

char *string_find_char(const char *string, int character)
{
    char *result = (char *) string;
    // Return pointer to first occurence of character
    while (*result && (*result != character))
    {
        result++;
    }
    return result;
}

float float_from_string(const char *value, float defaultValue)
{
    char *end;
    float parsedValue = strtof(value, &end);
    return end > value ? parsedValue : defaultValue;
}

int int_from_string(const char *value, int defaultValue)
{
    char *end;
    int parsedValue = strtol(value, &end, 0);
    return end > value ? parsedValue : defaultValue;
}

int8_t int8_from_string(const char *value, int8_t defaultValue)
{
    char *end;
    int parsedValue = strtol(value, &end, 0);
    return end > value ? parsedValue : defaultValue;
}

bool bool_from_string(const char *value, bool defaultValue)
{
    if (strncmp(value, "yes", 32) == 0)
    {
        return true;
    }
    else if (strncmp(value, "no", 32) == 0)
    {
        return false;
    }
    SDL_assert(false);
    return defaultValue;
}

// Hashing
uint32_t hash_bytes(uint8_t *__restrict buffer, int length)
{
    // jenkins_one_at_a_time_hash
    uint32_t hash = 0;
    for (int i=0; i < length; ++i)
    {
        hash += buffer[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

// Random
void init_random_engine(RandomEngine *engine, bool verbose)
{
    // Simple LCG random number generator
    engine->seed = (uint32_t) SDL_GetPerformanceCounter();
    engine->verbose = verbose;
    engine->modulus = INT_MAX;
    engine->multiplier = 1103515245;
    engine->increment = 12345;
    reset_random_engine(engine);
}

void reset_random_engine(RandomEngine *engine)
{
    engine->nInvocations = 0;
    engine->state = engine->seed;
    if (engine->verbose)
    {
        printf("Reset random engine, seed %u\n", engine->seed);
    }
}

void delete_random_engine(RandomEngine *engine)
{
    engine->nInvocations = 0;
    engine->seed = -1;
}

void set_rand_seed(RandomEngine *engine, uint32_t seed)
{
    engine->seed = seed;
    reset_random_engine(engine);
    if (engine->verbose)
    {
        printf("Set random seed to %u\n", seed);
    }
}

int rand_evolve(RandomEngine *engine)
{
    engine->nInvocations += 1;
    engine->state = (engine->multiplier*engine->state + engine->increment) % engine->modulus;
    return int((engine->state/65536) % 32768);
}

int rand_max()
{
    return SHRT_MAX;
}

int weighted_rand_i(RandomEngine *engine, DynamicArray<float> *weights, int minV)
{
    float sum = 0;
    for (int i=0; i<weights->size; ++i)
    {
        sum += weights->data[i];
    }

    float rand = rand_f(engine, 0, sum);

    sum = 0;
    for (int i=0; i<weights->size; ++i)
    {
        sum += weights->data[i];
        if (sum >= rand)
        {
            return minV + i;
        }
    }
    SDL_assert(false);
    return -1;
}

int rand_i(RandomEngine *engine, int minV, int maxV)
{
    SDL_assert(maxV > minV);
    SDL_assert(maxV - minV < rand_max());
    int result = rand_evolve(engine) % (maxV - minV) + minV;

    if (engine->verbose && engine->nInvocations < 10)
    {
        printf("Rand %d Integer %d %d: %d\n", engine->nInvocations, minV, maxV, result);
    }

    return result;
}

float rand_f(RandomEngine *engine, float minV, float maxV)
{
    float result = (float) rand_evolve(engine) / ((float) rand_max() / (maxV - minV)) + minV;

    if (engine->verbose && engine->nInvocations < 10)
    {
        printf("Rand %d Float %f %f: %f\n", engine->nInvocations, minV, maxV, result);
    }

    return result;
}

// Lerp
float lerp_f(float a, float b, float f)
{
    return a + f*(b - a);
}

// Direction
float directionToRad(vec2 dir)
{
    return std::fmod(atan2f(dir[1], dir[0]) + 2.0f*glm::pi<float>(), 2.0f*glm::pi<float>());
}

float direction_to_deg(vec2 dir)
{
    return degrees(directionToRad(dir));
}

// Indexed float
int indexed_float_compare(const void *float1, const void *float2)
{
    IndexedFloat *if1 = (IndexedFloat *) float1;
    IndexedFloat *if2 = (IndexedFloat *) float2;
    if (if1->data > if2->data) return  1;
    if (if1->data < if2->data) return -1;
    return 0;
}
