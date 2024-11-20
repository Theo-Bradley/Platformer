#version 330 core
struct InstanceAttributes
{
mat4 pvmMatrix;
vec4 atlasRect;
};
layout (location = 0) in vec3 position;
layout (location = 1) in InstanceAttributes attribs;
void main()
{
gl_Position = attribs.pvmMatrix * vec4(position, 1.0);
}