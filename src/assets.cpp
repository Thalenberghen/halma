#include "stdio.h"
#include "stdlib.h"

#include "imgui.h"
#include "dirent.h"

#include "GL/gl3w.h"
#include "GL/gl.h"
#include "GL/glu.h"
#include "GL/glext.h"

#include "SDL.h"
#include "SDL_image.h"

#include "dynamic_array.h"
#include "assets.h"
#include "render.h"
#include "mesh.h"
#include "common.h"

using namespace glm;

#undef min

// Shader
void init_shader(Shader *shader, GLuint programId, const char *name)
{
    shader->name = _strdup(name);
    shader->programId = programId;

    // Material
    shader->diffuseMapId = glGetUniformLocation(programId, "diffuseMap");

    // Model
    shader->viewId = glGetUniformLocation(programId, "view");
    shader->projectionId = glGetUniformLocation(programId, "projection");
    shader->modelId = glGetUniformLocation(programId, "model");

    // Player
    shader->playerColorId = glGetUniformLocation(programId, "playerColor");
}

void delete_shader(Shader *shader)
{
    free(shader->name);
}

void print_shader_info(Shader *shader)
{
    printf("programId %d\n", shader->programId);
    printf("diffuseMapId %d\n", shader->diffuseMapId);
    printf("\n");
}

GLuint load_shaders(const char *vertexPath, const char *fragmentPath, const char *geometryPath, const char *basePath)
{
    bool hasGeometry = geometryPath != NULL;

    // Create the shaders
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    load_and_compile_shader(vertexPath, vertexShaderId, basePath);

    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
    load_and_compile_shader(fragmentPath, fragmentShaderId, basePath);

    GLuint geometryShaderId = -1;
    if (hasGeometry)
    {
        geometryShaderId = glCreateShader(GL_GEOMETRY_SHADER);
        load_and_compile_shader(geometryPath, geometryShaderId, basePath);
    }

    // Link the program
    printf("Linking program\n");
    GLuint programId = glCreateProgram();
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);
    if (hasGeometry)
    {
        glAttachShader(programId, geometryShaderId);
    }
    glLinkProgram(programId);

    // Check the program
    GLint result = GL_FALSE;
    int infoLogLength;
    glGetProgramiv(programId, GL_LINK_STATUS, &result);
    glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0)
    {
        DynamicArray<char> programErrorMessage;
        init_dynamic_array(&programErrorMessage, infoLogLength++, "PROGRAM_ERROR_MESSAGE");
        glGetProgramInfoLog(programId, infoLogLength, NULL, programErrorMessage.data);
        printf("%s\n", programErrorMessage.data);
        delete_dynamic_array(&programErrorMessage);
    }

    glDetachShader(programId, vertexShaderId);
    glDeleteShader(vertexShaderId);
    glDetachShader(programId, fragmentShaderId);
    glDeleteShader(fragmentShaderId);

    if (hasGeometry)
    {
        glDetachShader(programId, geometryShaderId);
        glDeleteShader(geometryShaderId);
    }
    return programId;
}

GLuint load_and_compile_shader(const char *filePath, GLuint shaderId, const char *basePath)
{
    char path[MAX_FILE_PATH_LENGTH];
    snprintf(path, MAX_FILE_PATH_LENGTH, "%s/%s", basePath, filePath);

    printf("Compiling shader: %s\n", filePath);
    FILE *file = fopen (path, "rb");
    if (file == NULL)
    {
        printf("Code file '%s' could not be opened!\n", path);
        SDL_assert(false);
        return 0;
    }

    fseek (file, 0, SEEK_END);
    GLint length = ftell (file);
    fseek (file, 0, SEEK_SET);

    GLchar *shaderCode = (char*) malloc (length);
    if (shaderCode)
    {
        fread (shaderCode, 1, length, file);
    }
    fclose (file);

    GLint result = GL_FALSE;
    int infoLogLength;

    // Compile shader
    glShaderSource(shaderId, 1, &shaderCode, &length);
    glCompileShader(shaderId);
    free(shaderCode);

    // Check shader
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &result);
    glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0)
    {
        DynamicArray<char> shaderErrorMessage;
        init_dynamic_array(&shaderErrorMessage, infoLogLength+1, "SHADER_ERROR_MESSAGE");
        glGetShaderInfoLog(shaderId, infoLogLength, NULL, shaderErrorMessage.data);
        printf("%s\n", shaderErrorMessage.data);
        delete_dynamic_array(&shaderErrorMessage);
    }
    return result;
}


// Texture
SDL_Surface *load_surface(const char *filePath, const char *basePath)
{
    char buffer [MAX_FILE_PATH_LENGTH];
    bool good = snprintf(buffer, MAX_FILE_PATH_LENGTH, "%s/%s.png", basePath, filePath);
    if (!good)
    {
        SDL_assert(false);
    }
    printf("Loading texture '%s'\n", buffer);

    SDL_Surface *textureSurf = IMG_Load(buffer);
    if (!textureSurf)
    {
        printf("Image '%s' could not be opened.\n", buffer);
    }

    return textureSurf;
}

GLuint load_texture(const char *filePath, const char *basePath)
{
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    SDL_Surface *textureSurf = load_surface(filePath, basePath);
    if (!textureSurf)
    {
        return -1;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, textureSurf->w, textureSurf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureSurf->pixels);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // When MAGnifying the image (no bigger mipmap available), use LINEAR filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // When MINifying the image, use a LINEAR blend of two mipmaps, each filtered LINEARLY too
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);

    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(textureSurf);
    return textureId;
}

// Material
void init_material(Material *material, const char *name, GLuint shaderId, GLuint textureId)
{
    material->name = _strdup(name);
    material->shaderId = shaderId;
    material->textureId = textureId;
}

void delete_material(Material *material)
{
    free(material->name);
}

// Assets
void init_assets(Assets *assets, ImGuiIO *io, const char *basePath)
{
    DynamicArray<GLuint> *textureIds = &assets->textureIds;
    DynamicArray<Shader> *shaders = &assets->shaders;
    DynamicArray<Material> *materials = &assets->materials;
    DynamicArray<Mesh> *meshes = &assets->meshes;
    DynamicArray<ImFont*> *fonts = &assets->fonts;
    DynamicArray<vec4> *colors = &assets->colors;

    init_dynamic_array(textureIds, 64);
    {
        textureIds->size = TextureIds::_LAST;
        textureIds->data[TextureIds::BOARD] = load_texture("../textures/board", basePath);
    }
    printf("\n");

    init_dynamic_array(shaders, 10);
    {
        init_shader
        (
            shaders->get_slot(),
            load_shaders("../shaders/simple_vertex.glsl", "../shaders/simple_fragment.glsl", NULL, basePath),
            "Simple"
        );
    }
    printf("\n");

    init_dynamic_array(materials, 64);
    {
        init_material(materials->get_slot(), "Level Board", Shaders::SIMPLE, textureIds->data[TextureIds::BOARD]);
    }
    printf("\n");

    init_dynamic_array(meshes, 64);
    {
        meshes->size = Meshes::_LAST;
        generate_quadmesh(meshes, Meshes::QUAD);
        init_mesh(&meshes->data[Meshes::LEVEL_BOARD], Meshes::LEVEL_BOARD, 2048);
    }
    printf("\n");

    init_dynamic_array(fonts, 24);
    {
        fonts->size = Fonts::_LAST;
        fonts->data[Fonts::STD_FONT] = load_font(io, "../fonts/roboto/Roboto-Black.ttf", basePath, 16);
    }

    init_dynamic_array(colors, Colors::_LAST);
    {
        colors->size = Colors::_LAST;
        SDL_assert(colors->size <= colors->capacity);

        colors->data[Colors::PLAYER_RED] = s_rgb_to_linear(ivec4(210, 50, 50, 255));
        colors->data[Colors::PLAYER_BLUE] = s_rgb_to_linear(ivec4(60, 60, 175, 255));
        colors->data[Colors::PLAYER_GREEN] = s_rgb_to_linear(ivec4(170, 40, 150, 255));
    }
}

void delete_assets(Assets *assets, ImGuiIO *io)
{
    DynamicArray<GLuint> *textureIds = &assets->textureIds;
    DynamicArray<Shader> *shaders = &assets->shaders;
    DynamicArray<Material> *materials = &assets->materials;
    DynamicArray<Mesh> *meshes = &assets->meshes;
    DynamicArray<ImFont*> *fonts = &assets->fonts;

    for (int i=0; i<meshes->size; ++i)
    {
        delete_mesh(&meshes->data[i]);
    }
    delete_dynamic_array(meshes);

    for (int i=0; i<materials->size; ++i)
    {
        delete_material(&materials->data[i]);
    }
    delete_dynamic_array(materials);

    for (int i=0; i<shaders->size; ++i)
    {
        delete_shader(&shaders->data[i]);
    }
    delete_dynamic_array(shaders);

    for (int i=0; i<textureIds->size; ++i)
    {
        glDeleteTextures(1, &textureIds->data[i]);
    }
    delete_dynamic_array(textureIds);

    io->Fonts->ClearFonts();
    delete_dynamic_array(fonts);
}

// Fonts
ImFont *load_font(ImGuiIO *io, const char *filePath, const char *basePath, float size)
{
    char path[MAX_FILE_PATH_LENGTH];
    snprintf(path, MAX_FILE_PATH_LENGTH, "%s/%s", basePath, filePath);
    return io->Fonts->AddFontFromFileTTF(path, size);
}

ImFont *get_font(DynamicArray<ImFont*> *fonts, int8_t font, int8_t scalingMode)
{
    return fonts->data[scalingMode*Fonts::_LAST + font];
}
