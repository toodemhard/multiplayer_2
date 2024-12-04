#version 450

layout(location = 0) out vec4 FragColor;

layout(location = 0) in struct {
    vec4 Color;
    vec2 UV;
} In;

layout (set = 2, binding = 0) uniform sampler2D Sampler;

void main() {
    FragColor = texture(Sampler, In.UV);
}
