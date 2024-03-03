#include "stdio.h"

#include "SDL.h"

#include "imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"

#include "ui.h"
#include "assets.h"
#include "assets.h"

using namespace glm;

#undef max

// UI
ImVec4 srgb_to_imvec_color_3(ivec3 rgb)
{
	return srgb_to_imvec_color_4(ivec4(rgb, 1.0f));
}

ImVec4 srgb_to_imvec_color_4(ivec4 rgb)
{
    vec4 linear = s_rgb_to_linear(rgb);
    return ImVec4(linear[0], linear[1], linear[2], linear[3]);
}

ImVec4 linear_to_imvec_color_3(vec3 linear)
{
	return linear_to_imvec_color_4(vec4(linear, 1.0f));
}

ImVec4 linear_to_imvec_color_4(vec4 linear)
{
	return ImVec4(linear[0], linear[1], linear[2], linear[3]);
}

// Input
void init_input_state(InputState *inputState)
{
    inputState->mousePos = glm::ivec2(0, 0);
    update_input_state(inputState);
}


void update_input_state(InputState *inputState)
{
    inputState->mouseState = SDL_GetMouseState(NULL, NULL);
    inputState->keyboardState = SDL_GetKeyboardState(NULL);
    inputState->lastKeyPressed = -1;
}
