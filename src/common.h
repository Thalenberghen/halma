#ifndef COMMON_H
#define COMMON_H

#include "glm/glm.hpp"

template <typename T>
struct DynamicArray;

#ifndef _WIN32
#define _strdup strdup
#endif

// VERSION
#define MAJOR_VERSION 0
#define MINOR_VERSION 0
#define PATCH_VERSION 1

// NUMBERS
#define FLOAT_GREAT 1e6f
#define INT16_GREAT (SHRT_MAX - 1)  // 32766
#define INT_GREAT 1000000
#define FLOAT_SMALL 1e-3f

// GUI
#define MAX_TEXT_INPUT_LENGTH 256

// FILES
#define MAX_FILE_PATH_LENGTH 256

struct Arena;

glm::vec4 s_rgb_to_linear(glm::ivec4 rgb);
glm::ivec4 linear_to_s_rgb(glm::vec4 linear);

// String helpers
char *string_concat(const char *string1, char *string2, Arena *arena);
char *string_r_strip(char *string);
char *string_l_skip(const char *string);
char *string_copy_0(char *dest, const char *src, size_t size);
char *string_sanitize(char *string);
char *string_find_char(const char *string, int character);
float float_from_string(const char *value, float defaultValue);
int int_from_string(const char *value, int defaultValue);
int8_t int8_from_string(const char *value, int8_t defaultValue);
bool bool_from_string(const char *value, bool defaultValue);


// Hashing
uint32_t hash_bytes(uint8_t *buffer, int length);

template<class T>
const T& min_i(const T& a, const T& b)
{
    return (b < a) ? b : a;
}

template<class T>
const T& max_i(const T& a, const T& b)
{
    return (b < a) ? a : b;
}

// Random generator
struct RandomEngine
{
    uint32_t seed;
    uint32_t state;
    uint32_t modulus;
    uint32_t multiplier;
    uint32_t increment;
    int nInvocations;
    bool verbose;
};

void init_random_engine(RandomEngine *engine, bool verbose);
void reset_random_engine(RandomEngine *engine);
void delete_random_engine(RandomEngine *engine);
void set_rand_seed(RandomEngine *engine, uint32_t seed);
int rand_max();
int rand_evolve(RandomEngine *engine);
int rand_i(RandomEngine *engine, int minV, int maxV);
int weighted_rand_i(RandomEngine *engine, DynamicArray<float> *weights, int minV);
float rand_f(RandomEngine *engine, float minV, float maxV);

// Lerp
float lerp_f(float a, float b, float f);

// Indexed float
struct IndexedFloat
{
    int32_t index;
    float data;
};

int indexed_float_compare(const void *float1, const void *float2);

#endif //COMMON_H
