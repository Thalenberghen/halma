#include "stdio.h"
#include "stdlib.h"

#include "imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"

// ASSERT
#define DEBUG 1
#define SDL_ASSERT_LEVEL 2

#include "SDL.h"
#include "SDL_image.h"

#include "GL/gl3w.h"
#include "GL/gl.h"
#include "GL/glu.h"

#include <Eigen/Dense>

#include "data_structures/memory_arena.h"

#include "assets.h"
#include "render.h"
#include "mesh.h"
#include "ui.h"

#undef main

#include "common.cpp"
#include "assets.cpp"
#include "mesh.cpp"
#include "render.cpp"
#include "ui.cpp"

#include "imgui_draw.cpp"
#include "imgui_widgets.cpp"
#include "imgui_demo.cpp"
#include "imgui_tables.cpp"
#include "imgui.cpp"
#include "imgui_impl_opengl3.cpp"
#include "imgui_impl_sdl.cpp"

#include "GL/gl3w.c"

using namespace Eigen;

int main(int argc, char *argv[])
{
    // Init logging
    const char* basePath = SDL_GetBasePath();

    // Init SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("SDL Error: %s\n", SDL_GetError());
        return -1;
    }
    int imgFormats = IMG_INIT_PNG;
    if ((IMG_Init(imgFormats) & imgFormats) == 0)
    {
        printf("SDL_Image Error: %s\n", IMG_GetError());
        return -1;
    }
    printf("Initialised SDL\n");

    // Choose GL version
    const char *glsl_version = "#version 440";
    // SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    // SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);

    // Create window
    SDL_Window *window = NULL;
    SDL_GLContext gl_context = NULL;
    create_window(&window);
    create_context(&window, &gl_context);
    printf("Create window\n");

    // Initialise gl3w
    {
        int error = gl3wInit();
        if (error > 0)
        {
            fprintf(stderr, "Failed to initialize OpenGL loader!\n");
            return 1;
        }
    }
    printf("Initialised gl3w\n");

    // Set blending/depth
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_FRAMEBUFFER_SRGB);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO *io = &ImGui::GetIO();

    ImGui::StyleColorsDark();
    ImGui::SetMouseCursor(ImGuiMouseCursor_None);
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load assets
    Assets assets;
    init_assets(&assets, io, basePath);
    DynamicArray<GLuint> *textureIds = &assets.textureIds;
    DynamicArray<glm::vec4> *colors = &assets.colors;

    // Starting state
    InputState inputState;
    init_input_state(&inputState);

    // Temporary storage
    Arena frameArena;
    init_arena(&frameArena, 64*1024*1024);

    // Camera
    Camera camera;
    init_camera(&camera);

    // Entity group
    EntityGroup entityGroup;
    init_entity_group(&entityGroup, 512);

    {
        HotelArray<Entity> *entities = &entityGroup.entities;
        HotelRoom<Entity> *room = entities->get_room();
        Entity *e = &room->data;

        init_entity(e, room->index, "Test", &assets.meshes.data[Meshes::QUAD], Materials::LEVEL_BOARD);
        e->position = vec3(0.0, 0.0, 1.0);
        e->scale = vec3(1, 1, 1);

        entityGroup.entitiesModified = true;
    }

    bool quit = false;
    int width = 0;
    int height = 0;

    while (!quit)
    {
        double frameStartTime = double(SDL_GetPerformanceCounter())/SDL_GetPerformanceFrequency();

        // Update resolution
        SDL_GetWindowSize(window, &width, &height);

        // Input
        update_input_state(&inputState);

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            switch (event.type)
            {
                case SDL_QUIT:
                {
                    quit = true;
                    break;
                }
                case SDL_WINDOWEVENT:
                {
                    if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                    {
                        quit = true;
                    }
                    break;
                }
            }
        }

        // Start Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        Matrix<float, 3, 3> A;
        A.setZero();
        printf("Test %f %f\n", A(0, 2), A(1, 1));

        update_entity_group(&entityGroup, &frameArena);

        // Render
        {
            ImGui::Render();
            glViewport(0, 0, width, height);
            glClearColor(0.6f, 0.6f, 0.8f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Render entities
            glm::mat4 projection = camera_projection_matrix(&camera);
            glm::mat4 view = camera_view_matrix(&camera);
            glm::vec2 resolution(width, height);

            render_lighting_pass
            (
                &entityGroup,
                &assets.shaders,
                &assets.materials,
                &view,
                &projection,
                &resolution,
                &camera
            );

            // Draw image
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(window);
        }

        free_arena(&frameArena);

        // Limit FPS
        {
            double time = double(SDL_GetPerformanceCounter())/SDL_GetPerformanceFrequency();
            double msSinceFrameStart = 1000.0*(time - frameStartTime);
            double fpsLimit = 30.0;
            if (msSinceFrameStart < 1000.0/fpsLimit)
            {
                SDL_Delay(Uint32(1000.0/fpsLimit - msSinceFrameStart));
            }
        }
    }

    delete_entity_group(&entityGroup);

    delete_arena(&frameArena);
    delete_assets(&assets, io);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}