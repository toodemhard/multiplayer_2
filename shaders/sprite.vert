#version 450

layout(location = 0) in vec2 VertexPosition;
layout(location = 1) in vec2 VertexUV;

layout(location = 2) in vec2 RectPosition;
layout(location = 3) in vec2 RectSize;
layout(location = 4) in vec2 UVPosition;
layout(location = 5) in vec2 UVSize;

layout(location = 6) in vec4 MultColor;
layout(location = 7) in vec4 MixColor;
layout(location = 8) in float t;

layout(location = 0) out struct {
    vec2 UV;
    vec4 MultColor;
    vec4 MixColor;
    float t;
} Out;

void main() {
    Out.UV = VertexUV * UVSize + UVPosition;
    Out.MultColor = MultColor;
    Out.MixColor = MixColor;
    Out.t = t;

    vec2 Position = RectPosition + VertexPosition * RectSize;

    gl_Position = vec4(Position, 0, 1);
}
