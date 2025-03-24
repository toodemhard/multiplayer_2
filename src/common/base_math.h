#pragma once

union float4 {
    struct {
        f32 x,y,z,w;
    };
    struct {
        f32 r, g, b, a;
    };
    f32 v[4];
};

union float2 {
    struct {
        f32 x;
        f32 y;
    };
    f32 v[2];
    b2Vec2 b2vec;

    f32& operator[](i32 index) {
        return v[index];
    }

    const f32& operator[](i32 index) const {
        return v[index];
    }
};

struct float2x2 {
    f32 v[2][2];
};

constexpr float2 float2_one = {1,1};

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

float2 operator+(const float2& a, const float2& b);
float2 operator-(const float2& a, const float2& b);
float2 operator/(const float2& a, const float2& b);
float2 operator*(const float2& a, const float2& b);
float2 operator*(float a, const float2& b);
float2 operator/(const float2& a, float b);
float2 operator*(const float2& a, float b);

float2 operator*(const float2x2& a, const float2& b);

float magnitude(const float2& a);
float2 normalize(const float2& a);
f32 f32_max(f32 a, f32 b);
