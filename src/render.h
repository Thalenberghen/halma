#ifndef RENDER_H
#define RENDER_H

#include "GL/gl3w.h"
#include "GL/gl.h"
#include "GL/glu.h"

#include "SDL.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/quaternion.hpp"

#include "dynamic_array.h"
#include "static_array.h"
#include "hotel_array.h"

struct Shader;
struct Material;
struct Mesh;
struct BoundingBox;
struct ImVec4;
struct SDL_Window;

void print_gl_error();

// Player Colors
namespace PlayerColors
{
    enum
    {
        RED,
        BLUE,
        GREEN,
    };
};

struct PlayerColor
{
    const char *name;
    int colorId;
};

void create_window(SDL_Window **window);
void create_context(SDL_Window **window, SDL_GLContext *gl_context);
inline glm::mat4 translated_and_scaled(glm::vec3 position, glm::vec3 scale);

struct Camera
{
    glm::vec3 position;
    glm::vec3 lookAt;
    glm::vec3 up;
    glm::vec2 clipDist;
    float size;
    float aspectRatio;
};

void init_camera(Camera *camera);
glm::mat4 camera_view_matrix(Camera *camera);
glm::mat4 camera_projection_matrix(Camera *camera);
glm::ivec2 screen_pos(Camera *camera, glm::vec3 pos, glm::ivec2 displaySize);

// Entities
struct Entity
{
    GenId id;
    char *name;
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec4 color;
    glm::vec4 playerColor;
    Mesh *mesh;
    GLuint materialId;
};

void init_entity(Entity *entity, GenId id, const char *name, Mesh *mesh, GLuint materialId);
void delete_entity(Entity *entity);
glm::mat4 get_entity_matrix(Entity *entity);
void generate_entity_colors(Entity *entity, glm::vec4 *colors);
void draw_elements_in_mode(GLuint nVertices, GLenum mode, GLenum elementType, size_t offset);
void render_entity
(
    Entity *entity,
    size_t offset,
    Shader *shader,
    Material *material,
    glm::mat4 *view,
    glm::mat4 *projection,
    glm::vec2 *screenRes
);
BoundingBox combine_bounding_boxes(Entity *entities, int n);

struct EntityGroup
{
    GLuint vertexArrayIndex;
    StaticArray<GLuint, 4> vertexBufferIndex;
    HotelArray<Entity> entities;
    bool entitiesModified;
};

void init_entity_group(EntityGroup *group, int capacity);
void delete_entity_group(EntityGroup *group);
void reset_entity_group(EntityGroup *group);
void update_entity_group(EntityGroup *group, Arena *temporaryStorage);
void generate_vertex_attrib(GLuint attribId, GLuint vertexBufferIndex, GLuint nVertices, GLuint nCmpt, void *data);
void fill_entity_buffers(EntityGroup *group, Arena *temporaryStorage);

void render_lighting_pass
(
    EntityGroup *entities,
    DynamicArray<Shader> *shaders,
    DynamicArray<Material> *materials,
    glm::mat4 *view,
    glm::mat4 *projection,
    glm::vec2 *screenRes,
    Camera *camera
);

#endif //RENDER_H
