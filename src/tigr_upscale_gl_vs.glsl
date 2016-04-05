#version 330 core

layout (location = 0) in vec2 pos_in;
layout (location = 1) in vec2 uv_in;

out vec2 uv;

uniform mat4 model;
uniform mat4 projection;

void main()
{
	uv = uv_in;
	gl_Position = projection * model * vec4(pos_in, 0.0, 1.0);
}
