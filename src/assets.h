#ifndef ASSETS_H
#define ASSETS_H

#include "GL/gl3w.h"
#include "KHR/khrplatform.h"
#include "GL/gl.h"
#include "GL/glu.h"

#include "glm/glm.hpp"

#include "dynamic_array.h"

struct Mesh;
struct ImFont;
struct ImGuiIO;

// Shaders
struct Shader
{
    char *name;
    GLuint programId;
    GLuint diffuseMapId;
    GLuint screenResId;
    GLuint viewId;
    GLuint projectionId;
    GLuint modelId;
    GLuint playerColorId;
};

void init_shader(Shader *shader, GLuint programId, const char *name);
void delete_shader(Shader *shader);
GLuint load_shaders(const char *vertexPath, const char *fragmentPath, const char *geometryPath, const char *basePath);
GLuint load_and_compile_shader(const char *filePath, GLuint shaderId, const char *basePath);
void print_shader_info(Shader *shader);

SDL_Surface *load_surface(const char *filePath, const char *basePath);
GLuint load_texture(const char *filepath, const char *basePath);

struct Material
{
    char *name;
    GLuint shaderId;
    GLuint textureId;
};

void init_material(Material *material, const char *name, GLuint shaderId, GLuint textureId);
void delete_material(Material *material);

struct TextureIds
{
    enum
    {
        BOARD,
        _LAST,
    };
};

struct Shaders
{
    enum
    {
        SIMPLE,
        _LAST,
    };
};

struct Materials
{
    enum
    {
        LEVEL_BOARD,
        _LAST,
    };
};

struct Meshes
{
    enum
    {
        QUAD,
        LEVEL_BOARD,
        _LAST,
    };
};

namespace Fonts
{
    enum
    {
        STD_FONT,
        _LAST,
    };
};


// Colors
namespace Colors
{
    enum
    {
        PLAYER_RED,
        PLAYER_BLUE,
        PLAYER_GREEN,
        _LAST,
    };
}

struct Assets
{
    DynamicArray<GLuint> textureIds;
    DynamicArray<Shader> shaders;
    DynamicArray<Material> materials;
    DynamicArray<Mesh> meshes;
    DynamicArray<ImFont*> fonts;
    DynamicArray<glm::vec4> colors;
};

void init_assets(Assets *assets, ImGuiIO *io, const char *basePath);
void delete_assets(Assets *assets, ImGuiIO *io);

// Fonts
ImFont *load_font(ImGuiIO *io, const char *filePath, const char *basePath, float size);
ImFont* get_font(DynamicArray<ImFont*> *fonts, int8_t font, int8_t scalingMode);

#endif //ASSETS_H
