#version 330 core

// Maximum dynamic lights supported per frame
#define MAX_DLIGHTS 16

in vec3 v_WorldPos;    // World-space position for dynamic lighting
in vec3 v_Normal;
in vec2 v_TexCoord;
in vec2 v_LmCoord;
in float v_Depth;

out vec4 out_FragColor;

uniform vec3 u_LightDir;
uniform sampler2D u_Texture;
uniform sampler2D u_Lightmap;  // Lightmap atlas (stubbed: use white until real impl)
uniform float u_UseLightmap;   // 0.0 = fake directional, 1.0 = real lightmap

// Dynamic lights: origin.xyz, intensity in w
uniform vec4 u_DLightPos[MAX_DLIGHTS];    // xyz = position, w = intensity
uniform vec4 u_DLightColor[MAX_DLIGHTS];  // rgb = color, a = unused
uniform int u_NumDLights;                  // Number of active dynamic lights

// Calculate contribution from a single dynamic light
vec3 calcDynamicLight(int idx, vec3 worldPos, vec3 normal)
{
    vec3 lightPos = u_DLightPos[idx].xyz;
    float intensity = u_DLightPos[idx].w;
    vec3 lightColor = u_DLightColor[idx].rgb;
    
    // Direction from surface to light
    vec3 lightDir = lightPos - worldPos;
    float dist = length(lightDir);
    
    // Skip if outside light radius
    if (dist > intensity)
        return vec3(0.0);
    
    lightDir = normalize(lightDir);
    
    // Attenuation: linear falloff based on intensity (radius)
    float attenuation = 1.0 - (dist / intensity);
    attenuation = attenuation * attenuation;  // Quadratic falloff looks better
    
    // Lambertian diffuse - facing surfaces get more light
    float NdotL = max(dot(normal, lightDir), 0.0);
    
    return lightColor * NdotL * attenuation;
}

void main()
{
    // Sample texture
    vec4 texColor = texture(u_Texture, v_TexCoord);
    
    float brightness;
    vec3 normal = normalize(v_Normal);
    
    if (u_UseLightmap > 0.5) {
        // Lightmap path (future): sample baked lighting
        vec3 lmColor = texture(u_Lightmap, v_LmCoord).rgb;
        brightness = (lmColor.r + lmColor.g + lmColor.b) / 3.0;
        brightness = max(brightness, 0.1);  // Minimum ambient
    } else {
        // Fake directional lighting (current)
        vec3 lightDir = normalize(u_LightDir);
        float diff = max(dot(normal, lightDir), 0.0);
        
        // Ambient + diffuse
        float ambient = 0.3;
        brightness = ambient + diff * 0.7;
    }
    
    // Base lighting
    vec3 finalColor = texColor.rgb * brightness;
    
    // Add dynamic lights contribution
    for (int i = 0; i < u_NumDLights && i < MAX_DLIGHTS; i++) {
        finalColor += texColor.rgb * calcDynamicLight(i, v_WorldPos, normal);
    }
    
    // Clamp to avoid oversaturation
    finalColor = min(finalColor, vec3(1.5));
    
    out_FragColor = vec4(finalColor, texColor.a);
}
