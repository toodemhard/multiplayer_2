#pragma once
#include <cstdint>

struct RGBA {
    uint8_t r, g, b, a;
};

struct RGB {
    uint8_t r, g, b;
};

namespace rgba {
    constexpr RGBA white{255,255,255,255};
}

namespace color {
    constexpr RGB white{255, 255, 255};
}
