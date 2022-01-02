#version 330
layout(location = 0) out vec4 o_fragColor;

uniform vec3 u_color;

void main()
{
    o_fragColor = vec4(u_color, 1);
}