#ifndef MESH_H
#define MESH_H

#include "GL/gl3w.h"
#include "GL/gl.h"
#include "GL/glu.h"
#include "glm/glm.hpp"

#include "dynamic_array.h"

struct BoundingBox
{
    glm::vec3 min;
    glm::vec3 max;
};

void print_bounding_box(BoundingBox *bb);
inline glm::vec3 bounding_box_centre(BoundingBox *bb);
inline float bounding_box_size(BoundingBox *bb);
inline float bounding_box_radius(BoundingBox *bb);

struct Mesh
{
    int16_t id;
    BoundingBox boundingBox;
    DynamicArray<glm::vec3> vertices;
    DynamicArray<glm::uvec3> triangles;
    DynamicArray<glm::vec2> uvCoordinates;
    DynamicArray<char *> objectNames;
};

void init_mesh(Mesh *mesh, int16_t id, int capacity);
void reset_mesh(Mesh *mesh);
void delete_mesh(Mesh* mesh);
void generate_quadmesh(DynamicArray<Mesh> *meshes, int16_t id);
void append_square(Mesh *mesh, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec2 uvMin, glm::vec2 uvMax);
GLuint load_obj_mesh(const char *filePath, int16_t id, DynamicArray<Mesh>* meshes, const char *basePath);
void calculate_bounding_box(Mesh *mesh);

#endif //MESH_H
