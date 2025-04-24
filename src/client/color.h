#pragma once

typedef union RGBA RGBA;
union RGBA {
    struct {
        u8 r, g, b, a;
    };
    u8 v[4];
};
opt_def(RGBA);

typedef struct RGB RGB;
struct RGB {
    u8 r, g, b;
};

const RGBA rgba_white = {255, 255, 255, 255};
// constexpr RGBA black{0, 0, 0, 255};
// constexpr RGBA red{255, 0, 0, 255};
// constexpr RGBA grey{200, 200, 200, 255};
// constexpr RGBA bg{31, 31, 31, 200};
// constexpr RGBA bg_highlight{90, 90, 90, 200};
// constexpr RGBA none{0, 0, 0, 0};

// namespace rgba {
//     constexpr RGBA white{255,255,255,255};
// }
//
// namespace color {
//     constexpr RGB white{255, 255, 255};
// }
