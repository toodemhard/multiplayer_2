#version 450

layout(location = 0) in vec2 Position;
layout(location = 1) in vec2 Uv;
layout(location = 2) in vec4 Color;

layout(location = 0) out struct {
    vec4 Color;
    vec2 UV;
} Out;

void main() {
    Out.Color = Color;
    Out.UV = Uv;

    vec2 scale = vec2(2.0/1024.0, 2.0/768.0);
    vec2 translate = vec2(-1.0f, -1.0f);
    vec2 idk = Position * scale + translate;
    idk.y *= -1;
    gl_Position = vec4(idk, 0, 1);
}
