#version 440

in vec4 fragColor;
in vec2 uvCoord;

layout(location = 0) out vec4 color;

uniform sampler2D diffuseMap;
uniform vec3 playerColor;

void main()
{
    // vec4 diffuseColor = texture(diffuseMap, vec2(uvCoord.x, 1.0-uvCoord.y)).rgba;

    // if (length(diffuseColor.xyz - vec3(1.0, 0.0, 1.0)) < 0.2)
    // {
    //     diffuseColor = vec4(playerColor, 1.0);
    // }
    // diffuseColor *= fragColor.rgba;
    // if (diffuseColor.a < 0.1)
    // {
    //     discard;
    // }

    vec4 diffuseColor = vec4(1, 0, 0, 1);

	color = diffuseColor;
}