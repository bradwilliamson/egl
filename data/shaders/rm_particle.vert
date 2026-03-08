#version 330 core

/*
 * Particle Vertex Shader
 * 
 * Transforms particle quad vertices to clip space.
 * Passes through texture coordinates and vertex colors.
 */

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec2 in_TexCoord;
layout(location = 2) in vec4 in_Color;

uniform mat4 u_Projection;
uniform mat4 u_View;

out vec2 v_TexCoord;
out vec4 v_Color;

void main()
{
    gl_Position = u_Projection * u_View * vec4(in_Position, 1.0);
    v_TexCoord = in_TexCoord;
    v_Color = in_Color;
}
