#version 450 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in mat4 pvmMatrix;
layout(location = 6) in vec4 atlasRect;

out vec2 _uv;
out vec4 _atlasRect;
void main()
{
gl_Position = pvmMatrix * vec4(position, 1.0);
_uv = uv;
_atlasRect = atlasRect;
}