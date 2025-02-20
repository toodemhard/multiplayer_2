#version 450

layout(location = 0) in vec2 Position;
layout(location = 1) in vec4 Color;

layout(location = 0) out struct {
    vec4 Color;
} Out;

void main() {
    Out.Color = Color;

    gl_Position = vec4(Position, 0, 1);
}
