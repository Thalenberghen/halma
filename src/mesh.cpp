#include "stdio.h"
#include "stdlib.h"

#include "SDL.h"

#include "mesh.h"
#include "dynamic_array.h"
#include "hash_table.h"

using namespace glm;

// BoundingBox
void calculate_bounding_box(Mesh *mesh)
{
    glm::vec3 minV(FLT_MAX);
    glm::vec3 maxV(FLT_MIN);

    for (int v=0; v<mesh->vertices.size; ++v)
    {
        minV = glm::min(mesh->vertices.data[v], minV);
        maxV = glm::max(mesh->vertices.data[v], maxV);
    }
    mesh->boundingBox.min = minV;
    mesh->boundingBox.max = maxV;
}

void print_bounding_box(BoundingBox *bb)
{
    printf("Bounding box min %f %f %f\n", bb->min[0], bb->min[1], bb->min[2]);
    printf("Bounding box max %f %f %f\n", bb->max[0], bb->max[1], bb->max[2]);
}

vec3 bounding_box_centre(BoundingBox *bb)
{
    return 0.5f*(bb->max + bb->min);
}

float bounding_box_size(BoundingBox *bb)
{
    return bb->max[0] - bb->min[0];
}

float bounding_box_radius(BoundingBox *bb)
{
    return 0.5f*length(bb->max - bb->min);
}


// Mesh
void init_mesh(Mesh *mesh, int16_t id, int capacity)
{
    init_dynamic_array(&mesh->triangles, capacity);
    init_dynamic_array(&mesh->vertices, capacity);
    init_dynamic_array(&mesh->uvCoordinates, capacity);
    init_dynamic_array(&mesh->objectNames, 32);
    mesh->id = id;
}

void reset_mesh(Mesh *mesh)
{
    mesh->triangles.size = 0;
    mesh->vertices.size = 0;
    mesh->uvCoordinates.size = 0;

    for (int i=0; i<mesh->objectNames.size; ++i)
    {
        free(mesh->objectNames.data[i]);
    }
    mesh->objectNames.size = 0;
}

void delete_mesh(Mesh *mesh)
{
    delete_dynamic_array(&mesh->vertices);
    delete_dynamic_array(&mesh->triangles);
    delete_dynamic_array(&mesh->uvCoordinates);

    for (int i=0; i<mesh->objectNames.size; ++i)
    {
        free(mesh->objectNames.data[i]);
    }
    delete_dynamic_array(&mesh->objectNames);
}

GLuint load_obj_mesh(const char *filePath, int16_t id, DynamicArray<Mesh>* meshes, const char *basePath)
{
    printf("Loading obj file '%s'\n", filePath);
    // Record start time
    Uint64 start = SDL_GetPerformanceCounter();

    int capacity = 2000;
    Mesh *mesh = &meshes->data[id];
    init_mesh(mesh, id, capacity);
    DynamicArray<glm::uvec3> polyIds;
    init_dynamic_array(&polyIds, capacity);
    DynamicArray<int> polyLengths;
    init_dynamic_array(&polyLengths, capacity);

    DynamicArray<glm::vec3> verticesObj;
    init_dynamic_array(&verticesObj, capacity);
    DynamicArray<glm::vec2> uvsObj;
    init_dynamic_array(&uvsObj, capacity);
    DynamicArray<glm::vec3> normalsObj;
    init_dynamic_array(&normalsObj, capacity);

    // Open the file
    char path[MAX_FILE_PATH_LENGTH];
    snprintf(path, MAX_FILE_PATH_LENGTH, "%s/%s", basePath, filePath);
    FILE * file = fopen(path, "r");
    if (file == NULL)
    {
        printf("Obj file '%s' could not be opened!\n", path);
        SDL_assert(false);
        return 0;
    }

    DynamicArray<char> token;
    init_dynamic_array(&token, 64);
    bool skip = false;
    int8_t currentObject = 0;
    while (true)
    {
        char c = getc(file);
        if (c == EOF)
        {
            // End of file
            break;
        }
        else if (c == '\n')
        {
            // New line, reset token
            token.size = 0;
            token.data[0] = '\0';
            skip = false;
            continue;
        }
        else if (c == '#' or skip)
        {
            // Comment
            skip = true;
            continue;
        }

        // First line token
        if (c == ' ')
        {
            // End of token
            token.append('\0');

            if (strncmp(token.data, "v", 32) == 0)
            {
                glm::vec3 vertex;
                fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
                verticesObj.append(vertex);
            }
            else if (strncmp(token.data, "vt", 32) == 0)
            {
                glm::vec2 uv;
                fscanf(file, "%f %f\n", &uv.x, &uv.y);
                uvsObj.append(&uv);
            }
            else if (strncmp(token.data, "vn", 32) == 0)
            {
                glm::vec3 normal;
                fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
                normalsObj.append(&normal);
            }
            else if (strncmp(token.data, "f", 32) == 0)
            {
                uvec3 vi[4];
                int matches = fscanf
                (
                    file,
                    "%u/%u/%u %u/%u/%u %u/%u/%u %u/%u/%u\n",
                    &vi[0][0], &vi[0][1], &vi[0][2],
                    &vi[1][0], &vi[1][1], &vi[1][2],
                    &vi[2][0], &vi[2][1], &vi[2][2],
                    &vi[3][0], &vi[3][1], &vi[3][2]
                );

                if (matches % 3 == 0)
                {
                    SDL_assert(int(matches/3 < 5));
                    polyIds.append(&vi[0], int(matches/3));
                    polyLengths.append(int(matches/3));
                }
                else
                {
                    printf("File can't be read by this parser: (Try exporting with other options)\n");
                    SDL_assert(false);
                    return 0;
                }
            }
            else if (strncmp(token.data, "o", 32) == 0)
            {
                currentObject += 1;
                char buffer[256];
                fscanf(file, "%s\n", &buffer[0]);
                char **objectName = mesh->objectNames.get_slot();
                *objectName = _strdup(&buffer[0]);
            }
            else
            {
                skip = true;
            }
            // Reset token and skip
            token.size = 0;
            token.data[0] = '\0';
        }
        else
        {
            token.append(c);
        }
    }

    // Close the file
    delete_dynamic_array(&token);
    fclose(file);

    // Duplicate vertices with multiple uvs, normals
    HashTable<glm::uvec3, int32_t> ids;
    init_hash_table(&ids, 16*verticesObj.size);

    {
        int32_t polyCount = 0;
        int32_t polyEnd = -1;
        uvec4 quad(0, 0, 0, 0);

        for (int32_t i=0; i<polyIds.size; ++i)
        {
            int32_t length = polyLengths.data[polyCount];

            // Obj indices are 1 based
            uvec3 id = polyIds.data[i] - uvec3(1, 1, 1);

            int32_t *v = ids.get_value(&id);
            if (v != NULL)
            {
                // Vertex exists
                quad[(uint32_t) (i % length)] = (uint32_t) *v;
            }
            else
            {
                // Add vertex
                quad[(uint32_t) (i % length)] = (uint32_t) mesh->vertices.size;
                ids.set_value(&id, &mesh->vertices.size);
                mesh->vertices.append(&verticesObj.data[id[0]]);
                mesh->uvCoordinates.append(&uvsObj.data[id[1]]);
            }

            if (i - polyEnd == length)
            {
                mesh->triangles.append(uvec3(quad[0], quad[1], quad[2]));

                if (length == 4)
                {
                    mesh->triangles.append(uvec3(quad[0], quad[2], quad[3]));
                }
                polyEnd = i;
                ++polyCount;
            }
        }
    }
    delete_hash_table(&ids);
    delete_dynamic_array(&polyIds);
    delete_dynamic_array(&polyLengths);
    delete_dynamic_array(&verticesObj);
    delete_dynamic_array(&uvsObj);
    delete_dynamic_array(&normalsObj);

    calculate_bounding_box(mesh);

    // Record end time
    {
        Uint64 finish = SDL_GetPerformanceCounter();
        int ms = int(1000.0f*(finish - start)/SDL_GetPerformanceFrequency());
        printf("Finished loading in %d ms\n", ms);
    }
    printf("Objects: %d, Vertices: %d, Triangles: %d\n", mesh->objectNames.size, mesh->vertices.size, mesh->triangles.size);
    return (GLuint) mesh->vertices.size;
}

void generate_quadmesh(DynamicArray<Mesh> *meshes, int16_t id)
{
    Mesh *mesh = &meshes->data[id];
    init_mesh(mesh, id, 64);
    char **objectName = mesh->objectNames.get_slot();
    *objectName = _strdup("Square");
    append_square(mesh, vec3(0, 0, 0), vec3(0, 1, 0), vec3(1, 0, 0), vec2(0, 0), vec2(1, 1));
    append_square(mesh, vec3(0, 0, 0), vec3(1, 0, 0), vec3(0, 1, 0), vec2(0, 0), vec2(1, 1));
}

void append_square(Mesh *mesh, vec3 v1, vec3 v2, vec3 v3, vec2 uvMin, vec2 uvMax)
{
    int vI = mesh->vertices.size;
    vec3 v4 = v2 + v3 - v1;

    mesh->vertices.append(v1);
    mesh->vertices.append(v2);
    mesh->vertices.append(v3);
    mesh->vertices.append(v4);

    mesh->uvCoordinates.append(vec2(uvMin[0], uvMin[1]));
    mesh->uvCoordinates.append(vec2(uvMax[0], uvMin[1]));
    mesh->uvCoordinates.append(vec2(uvMin[0], uvMax[1]));
    mesh->uvCoordinates.append(vec2(uvMax[0], uvMax[1]));

    mesh->triangles.append(uvec3(vI+0, vI+1, vI+2));
    mesh->triangles.append(uvec3(vI+1, vI+3, vI+2));
}
