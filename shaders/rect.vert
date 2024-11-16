#version 450

layout (set = 1, binding = 0) uniform Rect {
	vec2 position;
    vec2 scale;
} rect;

layout (set = 1, binding = 1) uniform SourceRegion {
    vec2 position;
    vec2 scale;
} sourceRegion;

layout (location = 0) out vec2 texCoord;

void main() {
    vec2 vertices[4] = vec2[](
        vec2(-1.0, -1.0),
        vec2(1.0, -1.0),
        vec2(-1.0, 1.0),
        vec2(1.0, 1.0)
    );

    // vec2 uvs[4] = vec2[](
    //     vec2(0.0, 0.0),
    //     vec2(1.0, 0.0),
    //     vec2(-1.0, 1.0),
    //     vec2(1.0, 1.0)
    // );

    gl_Position = vec4(vertices[gl_VertexIndex] * rect.scale + rect.position, 0, 1.0);

    texCoord = (vertices[gl_VertexIndex] * 0.5 + 0.5) * sourceRegion.scale + sourceRegion.position;

}
