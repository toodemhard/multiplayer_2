#version 450

layout(location = 0) out vec4 FragColor;

layout(location = 0) in struct {
    vec2 UV;
    vec4 MultColor;
    vec4 MixColor;
    float t;
} In;

layout (set = 2, binding = 0) uniform sampler2D Sampler;

void main() {
    vec3 rgb = mix(In.MultColor.rgb * texture(Sampler, In.UV).rgb, In.MixColor.rgb, In.t);
    FragColor = vec4(rgb, In.MultColor.a * texture(Sampler, In.UV).a);
}
