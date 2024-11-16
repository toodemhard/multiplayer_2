#version 450

layout (location = 0) in vec2 TexCoord;
layout (location = 0) out vec4 FragColor;

layout (set = 3, binding = 0) uniform ColorMod {
    vec3 color;
} colorMod;

layout (set = 2, binding = 0) uniform sampler2D Sampler;

void main() {
    vec2 FlippedTexCoord = vec2(TexCoord.x, 1.0 - TexCoord.y);
    vec4 Color = texture(Sampler, FlippedTexCoord);
    Color.rgb *= colorMod.color.rgb;
    FragColor = Color;
}
