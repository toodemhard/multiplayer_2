#version 450

layout (location = 0) in vec2 TexCoord;

layout (location = 0) out vec4 FragColor;

layout (set = 3, binding = 0) uniform ColorMod {
    vec4 color;
} colorMod;

layout (set = 2, binding = 0) uniform sampler2D Sampler;

void main() {
    vec4 Color = texture(Sampler, TexCoord);
    Color *= colorMod.color;
    FragColor = Color;
}
