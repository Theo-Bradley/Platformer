#version 330 core
struct InstanceAttributes
{
mat4 pvmMatrix;
vec4 atlasRect;
};
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 _uv;
layout (location = 2) in InstanceAttributes attribs;

out vec2 uv;
void main()
{
gl_Position = attribs.pvmMatrix * vec4(position, 1.0);
uv = _uv;
}