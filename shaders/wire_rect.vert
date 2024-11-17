#version 450

layout (set = 1, binding = 0) uniform Rect {
	vec2 position;
    vec2 scale;
} rect;

void main() {
    vec2 vertices[5] = vec2[](
        vec2(-1.0, -1.0),
        vec2(1.0, -1.0),
        vec2(1.0, 1.0),
        vec2(-1.0, 1.0),
        vec2(-1.0, -1.0)
    );

    gl_Position = vec4(vertices[gl_VertexIndex] * rect.scale + rect.position, 0, 1.0);
}
