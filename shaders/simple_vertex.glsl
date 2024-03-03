#version 440

layout (location = 0) in vec3 vertexPosModel;
layout (location = 1) in vec4 vertexColor;
layout (location = 2) in vec2 vertexUv;

out vec4 fragColor;
out vec2 uvCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	vec4 vertexPos = model*vec4(vertexPosModel, 1);
	gl_Position = projection*view*vertexPos;
	fragColor = vertexColor;
	uvCoord = vertexUv;
}