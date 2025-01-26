#version 450

// layout (location = 0) in vec2 Position;
// layout (location = 0) in vec2 Size;
// layout (location = 0) in vec2 TopLeftUV;
// layout (location = 0) in vec2 BottomRightUV;
//
// layout (set = 1, binding = 0) uniform Rect {
// 	vec2 position;
//     vec2 scale;
// } rect;
//
// layout (set = 1, binding = 1) uniform SourceRegion {
//     vec2 position;
//     vec2 scale;
// } sourceRegion;
//
// layout(location = 0) out struct {
//     vec4 Color;
//     vec2 UV;
// } Out;
//
void main() {
//     vec2 rectOffsets[4] = vec2[](
//         vec2(-1.0, -1.0),
//         vec2(1.0, -1.0),
//         vec2(-1.0, 1.0),
//         vec2(1.0, 1.0),
//     );
//
//
//     vec2 uvOffsets[4] = vec2[4](
//         vec2(0.0, 0.0),
//         vec2(1.0, 0.0),
//         vec2(0.0, 1.0), 
//         vec2(1.0, 1.0), 
//     );
//
//     gl_Position = vec4(vertices[gl_VertexIndex] * Size + Position, 0, 1.0);
//     vec2 uv = (vertices[gl_VertexIndex] * 0.5 + 0.5);
//     TexCoord = vec2(uv.x, 1.0 - uv.y) * sourceRegion.scale + sourceRegion.position;
}
