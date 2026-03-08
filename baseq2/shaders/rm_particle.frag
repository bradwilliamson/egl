#version 330 core

/*
 * Particle Fragment Shader
 *
 * Samples texture and modulates by vertex color for tinted/alpha particles.
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
