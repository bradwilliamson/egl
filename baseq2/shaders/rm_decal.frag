#version 330 core

/*
 * Decal Fragment Shader
 *
 * Samples decal texture and modulates by vertex color.
 * Used for bullet holes, scorch marks, blood splats, etc.
 */

in vec2 v_TexCoord;
in vec4 v_Color;

uniform sampler2D u_Texture;

out vec4 FragColor;

void main()
{
    vec4 texColor = texture(u_Texture, v_TexCoord);
    
    /* Modulate texture by vertex color (allows tinting and alpha fade) */
    FragColor = texColor * v_Color;
    
    /* Discard fully transparent pixels */
    if (FragColor.a < 0.01)
        discard;
}
