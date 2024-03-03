#ifndef UI_H
#define UI_H

#include "../libs/glm/glm/glm.hpp"
#include "imgui.h"

#include "dynamic_array.h"

struct UvCoordinates
{
    ImVec2 min;
    ImVec2 max;
};

// UI
ImVec4 srgb_to_imvec_color_3(glm::ivec3 rgb);
ImVec4 srgb_to_imvec_color_4(glm::ivec4 rgb);
ImVec4 linear_to_imvec_color_3(glm::vec3 linear);
ImVec4 linear_to_imvec_color_4(glm::vec4 linear);

// Input
struct InputState
{
    glm::ivec2 mousePos;
    int mouseState;
    const Uint8* keyboardState;
    int lastKeyPressed;
};

void init_input_state(InputState *inputState);
void update_input_state(InputState *inputState);

#endif //UI_H