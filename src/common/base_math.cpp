f32 f32_max(f32 a, f32 b) {
    if (b > a) {
        return b;
    }
    return a;
}

float2 operator+(const float2& a, const float2& b) {
    float2 c;
    for (u32 i = 0; i < 2; i++) {
        c.v[i] = a.v[i] + b.v[i];
    }
    return c;
}

float2 operator-(const float2& a, const float2& b) {
    float2 c;
    for (u32 i = 0; i < 2; i++) {
        c.v[i] = a.v[i] - b.v[i];
    }
    return c;
}

float2 operator/(const float2& a, const float2& b) {
    float2 c;
    for (u32 i = 0; i < 2; i++) {
        c.v[i] = a.v[i] / b.v[i];
    }
    return c;
}

float2 operator*(const float2& a, const float2& b) {
    float2 c;
    for (u32 i = 0; i < 2; i++) {
        c.v[i] = a.v[i] * b.v[i];
    }
    return c;
}

float2 operator*(float a, const float2& b) {
    float2 c;
    for (u32 i = 0; i < 2; i++) {
        c.v[i] = a * b.v[i];
    }
    return c;
}

float2 operator*(const float2& a, float b) {
    return b * a;
}

float2 operator/(const float2& a, float b) {
    float2 c;
    for (u32 i = 0; i < 2; i++) {
        c.v[i] = a.v[i] / b;
    }
    return c;
}

float2 operator*(const float2x2& a, const float2& b) {
    float2 c;
    for (u32 j = 0; j < 2; j++) {
        float v = 0;
        for (u32 i = 0; i < 2; i++) {
            v += a.v[j][i] * b.v[i];
        }
        c.v[j] = v;
    }
    return c;
}

float magnitude(const float2& a) {
    return sqrtf(a.x * a.x + a.y * a.y);
}

float2 normalize(const float2& a) {
    return a / magnitude(a);
}
