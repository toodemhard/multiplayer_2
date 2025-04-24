#pragma once

typedef union float4 float4;
union float4 {
    struct {
        f32 x,y,z,w;
    };
    struct {
        f32 r, g, b, a;
    };
    f32 v[4];
};

typedef union float2 float2;
union float2 {
    struct {
        f32 x;
        f32 y;
    };
    f32 v[2];
    b2Vec2 b2vec;
};

typedef struct float2x2 float2x2;
struct float2x2 {
    f32 v[2][2];
};

const global float2 float2_one = {1,1};

typedef union Rect Rect;
union Rect {
    struct {
        float2 position;
        float2 size;
    };
    struct {
        f32 x;
        f32 y;
        f32 w;
        f32 h;
    };
};

float lerp(float v0, float v1, float t);
f32 f32_max(f32 a, f32 b);
float2 float2_add(float2 a, float2 b);
float2 float2_sub(float2 a, float2 b);
float2 float2_div(float2 a, float2 b);
float2 float2_mult(float2 a, float2 b);
float2 float2_scale(float2 a, float b);
float2 float2x2_mult_float2(float2x2 a, float2 b);
float magnitude(float2 a);
float2 normalize(float2 a);
float4 float4_lerp(float4 a, float4 b, f32 t);
