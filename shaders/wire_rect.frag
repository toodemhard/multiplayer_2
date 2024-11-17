#version 450

layout (location = 0) out vec4 FragColor;

layout (set = 3, binding = 0) uniform ColorMod {
    vec4 color;
} colorMod;

void main() {
    FragColor = colorMod.color;
}
