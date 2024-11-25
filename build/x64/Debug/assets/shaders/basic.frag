#version 450 core

in vec2 _uv;
in vec4 _atlasRect;
out vec4 color;

uniform sampler2D tex;
uniform vec2 atlasSize;

void main()
{
//convert 0-1 uv coords to coords relative to atlas
float x = (_uv.x * _atlasRect.y - _uv.x * _atlasRect.x + _atlasRect.x) / atlasSize.x;
float y = (_uv.y * _atlasRect.w - _uv.y * _atlasRect.z + _atlasRect.z) / atlasSize.y;
color = texture(tex,  vec2(x, y));
}