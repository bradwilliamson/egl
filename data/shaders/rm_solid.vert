#version 330 core

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec2 in_TexCoord;
layout(location = 3) in vec2 in_LmCoord;  // Lightmap UVs (stubbed for future)

uniform mat4 u_Projection;
uniform mat4 u_View;
uniform mat4 u_Model;  // Optional model matrix (identity for BSP)

out vec3 v_WorldPos;   // World-space position for dynamic lighting
out vec3 v_Normal;
out vec2 v_TexCoord;
out vec2 v_LmCoord;
out float v_Depth;

void main()
{
    // Transform position through model -> view -> projection
    vec4 worldPos = u_Model * vec4(in_Position, 1.0);
    vec4 viewPos = u_View * worldPos;
    gl_Position = u_Projection * viewPos;
    
    // Pass world position for dynamic light calculations
    v_WorldPos = worldPos.xyz;
    
    // Transform normal by model matrix (for rotated entities)
    mat3 normalMatrix = mat3(u_Model);
    v_Normal = normalize(normalMatrix * in_Normal);
    
    // Pass texture coordinates
    v_TexCoord = in_TexCoord;
    v_LmCoord = in_LmCoord;  // Pass lightmap coords (will be 0,0 until implemented)
    
    // Pass depth for optional distance fog (normalized to 0-1 range)
    v_Depth = clamp(-viewPos.z / 2000.0, 0.0, 1.0);
}
