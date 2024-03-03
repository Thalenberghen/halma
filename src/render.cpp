#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "SDL.h"

#include "dynamic_array.h"
#include "memory_arena.h"

#include "mesh.h"
#include "render.h"
#include "assets.h"

using namespace glm;

#undef max

void print_gl_error()
{
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR)
    {
        // Process/log the error.
        printf("GL Error %d\n", error);
        SDL_assert(false);
    }
}

void create_window(SDL_Window **window)
{
    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);
    SDL_WindowFlags windowFlags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    *window = SDL_CreateWindow
    (
        "Halma",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1024,
        768,
        windowFlags
    );
}

void create_context(SDL_Window **window, SDL_GLContext *gl_context)
{
    SDL_GL_DeleteContext(*gl_context);
    *gl_context = SDL_GL_CreateContext(*window);
    SDL_GL_MakeCurrent(*window, *gl_context);
    SDL_GL_SetSwapInterval(0); // Enable vSync
}

mat4 translated_and_scaled(vec3 position, vec3 scale)
{
    mat4 result(1.0f);
    for (int i=0; i<3; ++i)
    {
        result[i][i] = scale[i];
        result[3][i] = position[i];
    }
    return result;
}

// Camera
void init_camera(Camera *camera)
{
    camera->clipDist = vec2(0.1f, 50.0f);
    camera->lookAt = vec3(0, 0, 0);
    camera->position = vec3(0, 0, 10);
    camera->up = vec3(0, 1, 0);
    camera->size = 5.0f;
    camera->aspectRatio = 16.0f/9.0f;
}

vec3 camera_ground_rectangle_centre(Camera *camera)
{
    return camera->lookAt;
}

float camera_distance(Camera *camera)
{
    return length(camera->lookAt - camera->position);
}

mat4 camera_view_matrix(Camera *camera)
{
    return lookAt(camera->position, camera->lookAt, camera->up);
}

mat4 camera_projection_matrix(Camera *camera)
{
    float dist = camera_distance(camera);
    float aspect = camera->aspectRatio;
    float span = 0.5f*dist*camera->size;
    return ortho<float>(-aspect*span, aspect*span, -span, span, dist*camera->clipDist[0], dist*camera->clipDist[1]);
}

ivec2 screen_pos(Camera *camera, vec3 pos, ivec2 displaySize)
{
    mat4 view = camera_view_matrix(camera);
    mat4 projection = camera_projection_matrix(camera);

    vec4 projPos = projection*view*vec4(pos, 1.0f);
    vec3 screenPos = projPos / projPos[3];
    screenPos = 0.5f*(screenPos + vec3(1, 1, 0));

    return ivec2(displaySize[0]*screenPos[0], displaySize[1]*(1.0f - screenPos[1]));
}

// Entity
void init_entity(Entity *entity, GenId id, const char *name, Mesh *mesh, GLuint materialId)
{
    entity->id = id;
    entity->name = _strdup(name);
    entity->position = vec3(0, 0, 0);
    entity->scale = vec3(1, 1, 1);
    entity->color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    entity->playerColor = ivec4(0, 0, 0, 255);
    entity->mesh = mesh;
    entity->materialId = materialId;
}

void delete_entity(Entity *entity)
{
    free(entity->name);
}

mat4 get_entity_matrix(Entity *entity)
{
    mat4 scaled = translated_and_scaled(entity->position, entity->scale);
    return scaled;
}

void draw_elements_in_mode(GLuint nVertices, GLenum mode, GLenum elementType, size_t offset)
{
    glPolygonMode(GL_FRONT_AND_BACK, mode);
    glDrawElements(elementType, nVertices, GL_UNSIGNED_INT, (GLvoid *) offset);
}

void render_entity
(
    Entity *entity,
    size_t offset,
    Shader *shader,
    Material *material,
    glm::mat4 *view,
    glm::mat4 *projection,
    glm::vec2 *screenRes
)
{
    glm::mat4 model = get_entity_matrix(entity);
    glUseProgram(shader->programId);

    // Setup texture
    glBindTexture(GL_TEXTURE_2D, material->textureId);
    glUniform1i(shader->diffuseMapId, 0);

    // Player
    glUniform3fv(shader->playerColorId, 1, &entity->playerColor[0]);

    // Setup model matrix
    glUniformMatrix4fv(shader->modelId, 1, GL_FALSE, &model[0][0]);

    // Setup projection
    glUniformMatrix4fv(shader->viewId, 1, GL_FALSE, (float *) view);
    glUniformMatrix4fv(shader->projectionId, 1, GL_FALSE, (float *) projection);

    draw_elements_in_mode(entity->mesh->triangles.size*3, GL_FILL, GL_TRIANGLES, offset);
    offset += entity->mesh->triangles.size*sizeof(glm::uvec3);
    glBindTexture(GL_TEXTURE_2D, 0);
}

#undef max
#undef min

// Entity Group
void init_entity_group(EntityGroup *group, int capacity)
{
    glGenVertexArrays(1, &group->vertexArrayIndex);
    glGenBuffers(group->vertexBufferIndex.size(), &group->vertexBufferIndex[0]);
    init_hotel_array(&group->entities, capacity);
    group->entitiesModified = false;
}

void delete_entity_group(EntityGroup *group)
{
    HotelArray<Entity> *entities = &group->entities;
    for_all_occupied(Entity, entities, room)
    {
        delete_entity(&room->data);
    }
    delete_hotel_array(entities);
}

void reset_entity_group(EntityGroup *group)
{
    HotelArray<Entity> *entities = &group->entities;
    for_all_occupied(Entity, entities, room)
    {
        delete_entity(&room->data);
    }
    reset_hotel_array(&group->entities);
}

void update_entity_group(EntityGroup *group, Arena *frameArena)
{
    if (group->entitiesModified)
    {
        group->entitiesModified = false;
        fill_entity_buffers(group, frameArena);
    }
}

void generate_vertex_attrib(GLuint attribId, GLuint vertexBufferIndex, GLuint nVertices, GLuint nCmpt, void *data)
{
    glEnableVertexAttribArray(attribId);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferIndex);
    glBufferData(GL_ARRAY_BUFFER, nVertices*nCmpt*sizeof(float), data, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(attribId, nCmpt, GL_FLOAT, GL_FALSE, 0, NULL);
}

void fill_entity_buffers(EntityGroup *group, Arena *frameArena)
{
    HotelArray<Entity> *entities = &group->entities;

    // Bind vao as used object
    glBindVertexArray(group->vertexArrayIndex);

    int nVertices = 0;
    int nTriangles = 0;

    for_all_occupied(Entity, entities, room)
    {
        Entity *e = &room->data;
        SDL_assert(room->index.id < entities->n_rooms());
        SDL_assert(room->index.id >= 0);

        Mesh *mesh = e->mesh;
        nVertices += mesh->vertices.size;
        nTriangles += mesh->triangles.size;
    }
    printf("Entity Group, Entities %d, Vertices %d, Triangles %d\n", entities->n_rooms(), nVertices, nTriangles);

    // Vertex buffer
    {
        generate_vertex_attrib(0, group->vertexBufferIndex[0], nVertices, 3, NULL);
        int offset = 0;
        for_all_occupied(Entity, entities, room)
        {
            Entity *e = &room->data;
            Mesh *mesh = e->mesh;
            int size = mesh->vertices.size*3*sizeof(float);
            glBufferSubData(GL_ARRAY_BUFFER, offset, size, mesh->vertices.data);
            offset += size;
        }
    }

    // Color buffer
    {
        generate_vertex_attrib(1, group->vertexBufferIndex[1], nVertices, 4, NULL);
        int offset = 0;
        for_all_occupied(Entity, entities, room)
        {
            Entity *e = &room->data;
            vec4 color = e->color;
            Mesh *mesh = e->mesh;
            int size = mesh->vertices.size*4*sizeof(float);
            vec4 *colors = (vec4*) arena_alloc(frameArena, size);
            for (int v=0; v < mesh->vertices.size; ++v)
            {
                colors[v] = color;
            }
            glBufferSubData(GL_ARRAY_BUFFER, offset, size, colors);
            arena_free(frameArena, colors);

            offset += size;
        }
    }

    // UV Buffer
    {
        generate_vertex_attrib(2, group->vertexBufferIndex[2], nVertices, 2, NULL);
        int offset = 0;
        for_all_occupied(Entity, entities, room)
        {
            Entity *e = &room->data;
            Mesh *mesh = e->mesh;
            int size = mesh->vertices.size*2*sizeof(float);
            glBufferSubData(GL_ARRAY_BUFFER, offset, size, mesh->uvCoordinates.data);
            offset += size;
        }
    }

    // Triangles
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, group->vertexBufferIndex[3]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, nTriangles*sizeof(uvec3), NULL, GL_STATIC_DRAW);

        int offset = 0;
        GLuint vertexOffset = 0;
        for_all_occupied(Entity, entities, room)
        {
            Entity *e = &room->data;
            printf("Entity %s offset %d\n", e->name, offset);

            Mesh *mesh = e->mesh;
            int size = mesh->triangles.size*sizeof(uvec3);
            uvec3* indices = (uvec3*) arena_alloc(frameArena, size);
            for (int j=0; j < mesh->triangles.size; ++j)
            {
                indices[j] = mesh->triangles.data[j] + vertexOffset*uvec3(1, 1, 1);
            }
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, size, indices);
            arena_free(frameArena, indices);

            offset += size;
            vertexOffset += mesh->vertices.size;
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void render_lighting_pass
(
    EntityGroup *group,
    DynamicArray<Shader> *shaders,
    DynamicArray<Material> *materials,
    glm::mat4 *view,
    glm::mat4 *projection,
    glm::vec2 *screenRes,
    Camera *camera
)
{
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(group->vertexArrayIndex);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, group->vertexBufferIndex[3]);

    size_t offset = 0;
    HotelArray<Entity> *entities = &group->entities;
    for_all_occupied(Entity, entities, room)
    {
        Entity *e = &room->data;
        Material *m = &materials->data[e->materialId];
        Shader *s = &shaders->data[m->shaderId];
        render_entity
        (
            e,
            offset,
            s,
            m,
            view,
            projection,
            screenRes
        );
        SDL_assert(e->mesh != NULL);
        offset += e->mesh->triangles.size*sizeof(glm::uvec3);
    }
}
