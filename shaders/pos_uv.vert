#version 450

layout(location = 0) in vec2 Position;
layout(location = 1) in vec2 UV;

layout(location = 0) out struct {
    vec2 UV;
} Out;

void main() {
    Out.UV = UV;

    gl_Position = vec4(Position, 0, 1);
}
