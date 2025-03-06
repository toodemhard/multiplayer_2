#include "pch.h"

#include "tracy/Tracy.hpp"

#include "assets.h"
#include "panic.h"
#include "font.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "renderer.h"

std::vector<char> read_file(const char* file_path) {
    std::ifstream file{file_path, std::ios::binary | std::ios::ate};
    if (!file) {
        panic("{} not found", file_path);
    }

    auto size = file.tellg();
    std::vector<char> data(size, '\0');
    file.seekg(0);
    file.read(data.data(), size);

    return data;
}

const static int char_start = 32;
const static int num_chars = 96;

struct FontAtlasConfig {
    int width, height, baked_font_size;
};

// constexpr FontAtlasConfig text_font_atlas_config = {512, 512, 64};
//
// constexpr FontAtlasConfig icon_font_atlas_config = {128, 128, 64};
//
// template <size_t char_count> struct FontAtlas {
//     SDL_Texture* texture;
//     std::array<stbtt_packedchar, char_count> char_data;
// };
//
// using TextFontAtlas = FontAtlas<96>;
// using IconFontAtlas = FontAtlas<4>;
//
// TextFontAtlas text_font_atlas;
// IconFontAtlas icon_font_atlas;

void load_font(Font* font_output, Renderer& renderer, FontID font_id, int w, int h, int font_size) {

    *font_output = Font{.char_data = std::vector<stbtt_packedchar>(num_chars), .w = w, .h = h, .baked_font_size = font_size};

    auto bitmap_length = w * h;
    auto bitmap = std::vector<unsigned char>(bitmap_length);

    auto ttf_buffer = read_file(font_paths[(int)font_id]);
    stbtt_pack_context spc;
    stbtt_PackBegin(&spc, bitmap.data(), w, h, 0, 1, nullptr);
    stbtt_PackFontRange(&spc, (unsigned char*)ttf_buffer.data(), 0, font_size, char_start, num_chars, font_output->char_data.data());

    stbtt_PackEnd(&spc);

    int image_size = bitmap_length * 4;
    auto image = std::vector<unsigned char>(image_size);
    for (int i = 0; i < bitmap_length; i++) {
        image[i * 4] = 255;
        image[i * 4 + 1] = 255;
        image[i * 4 + 2] = 255;
        image[i * 4 + 3] = bitmap[i];
    }
    
    auto texture_id = font_id_to_texture_id(font_id);
    load_texture(
        texture_id,
        Image{
            .w = w,
            .h = h,
            .data = image.data(),
        }
    );
    font_output->texture_atlas = texture_id;
}

    // void generate_font_atlas(std::vector<unsigned char>* bitmap_output, int w, int h, int font_size) {
    //     auto& config = text_font_atlas_config;
    //
    //     constexpr auto bitmap_length = config.width * config.height;
    //     bitmap = std::vector<unsigned char>(bitmap_length);
    //     auto& char_data = text_font_atlas.char_data;
    //
    //     auto ttf_buffer = read_file("Avenir LT Std 95 Black.ttf");
    //     stbtt_pack_context spc;
    //     stbtt_PackBegin(&spc, bitmap.data(), config.width, config.height, 0, 1, nullptr);
    //     stbtt_PackFontRange(
    //         &spc, (unsigned char*)ttf_buffer.data(), 0, config.baked_font_size, 32, 96, char_data.data()
    //     );
    //
    //     stbtt_PackEnd(&spc);

    // constexpr int image_size = config.width * config.height * 4;
    // auto image = std::vector<unsigned char>(image_size);
    // for (int i = 0; i < bitmap_length; i++) {
    //     image[i * 4] = bitmap[i];
    //     image[i * 4 + 1] = bitmap[i];
    //     image[i * 4 + 2] = bitmap[i];
    //     image[i * 4 + 3] = bitmap[i];
    // }
    //
    // auto surface = SDL_CreateSurfaceFrom(config.width, config.height, SDL_PIXELFORMAT_RGBA8888, image.data(),
    // config.width * 4); text_font_atlas.texture = SDL_CreateTextureFromSurface(renderer, surface);

    // stbtt_fontinfo icon_font_info;
    //
    // std::vector<int32_t> icons = {
    //     U'󰒫',
    //     U'󰒬',
    //     U'󰐊',
    //     U'󰏤',
    // };
    //
    // void generate_icon_font_atlas(SDL_Renderer* renderer) {
    //     auto& config = icon_font_atlas_config;
    //
    //     constexpr auto bitmap_length = config.width * config.height;
    //     std::vector<unsigned char> bitmap(bitmap_length);
    //
    //     auto ttf_buffer = read_file("JetBrainsMonoNLNerdFont-Regular.ttf");
    //     stbtt_InitFont(&icon_font_info, (unsigned char*)ttf_buffer.data(), 0);
    //     stbtt_pack_context spc;
    //     stbtt_PackBegin(&spc, bitmap.data(), config.width, config.height, 0, 1, nullptr);
    //
    //     stbtt_pack_range range{};
    //     range.font_size = config.baked_font_size;
    //     range.array_of_unicode_codepoints = icons.data();
    //     range.num_chars = icons.size();
    //     range.chardata_for_range = icon_font_atlas.char_data.data();
    //
    //
    //     stbtt_PackFontRanges(&spc, (unsigned char*)ttf_buffer.data(), 0, &range, 1);
    //
    //     stbtt_PackEnd(&spc);
    //
    //     constexpr int image_size = config.width * config.height * 4;
    //     auto image = std::vector<unsigned char>(image_size);
    //     for (int i = 0; i < bitmap_length; i++) {
    //         image[i * 4] = bitmap[i];
    //         image[i * 4 + 1] = bitmap[i];
    //         image[i * 4 + 2] = bitmap[i];
    //         image[i * 4 + 3] = bitmap[i];
    //     }
    //
    //     auto surface = SDL_CreateSurfaceFrom(config.width, config.height, SDL_PIXELFORMAT_RGBA8888, image.data(),
    //     config.width * 4); icon_font_atlas.texture = SDL_CreateTextureFromSurface(renderer, surface);
    // }
    //
    // stbtt_packedchar get_icon_char_info(int32_t codepoint) {
    //     for (int i = 0; i < icons.size(); i++) {
    //         if(icons[i] == codepoint) {
    //             return icon_font_atlas.char_data[i];
    //         }
    //     }
    //
    //     return {};
    // }

    // void save_font_atlas_image() {
    //     std::vector<unsigned char> bitmap;
    //     generate_font_atlas(bitmap);
    //     auto& config = text_font_atlas_config;
    //
    //     constexpr int image_size = config.width * config.height * 4;
    //     auto image = std::vector<unsigned char>(image_size);
    //     for (int i = 0; i < bitmap.size(); i++) {
    //         image[i * 4] = 255;
    //         image[i * 4 + 1] = 255;
    //         image[i * 4 + 2] = 255;
    //         image[i * 4 + 3] = bitmap[i];
    //     }
    //
    //     std::cout << stbi_write_png("font_atlas.png", config.width, config.height, 4, image.data(), 4 * config.width)
    //               << "\n";
    // }
    //
    // void save_icons() {
    //     auto [image, char_data] = generate_icon_font_atlas();
    //     auto& config = icon_font_atlas_config;
    //
    //     stbi_write_png("icons.png", config.width, config.height, 4, image.data(), 4 * config.width);
    // }

int utf8decode_unsafe(const char* s, int32_t* c) {
    switch (s[0] & 0xf0) {
    default:
        *c = (int32_t)(s[0] & 0xff) << 0;
        if (*c > 0x7f)
            break;
        return 1;
    case 0xc0:
    case 0xd0:
        if ((s[1] & 0xc0) != 0x80)
            break;
        *c = (int32_t)(s[0] & 0x1f) << 6 | (int32_t)(s[1] & 0x3f) << 0;
        if (*c < 0x80) {
            break;
        }
        return 2;
    case 0xe0:
        if ((s[1] & 0xc0) != 0x80)
            break;
        if ((s[2] & 0xc0) != 0x80)
            break;
        *c = (int32_t)(s[0] & 0x0f) << 12 | (int32_t)(s[1] & 0x3f) << 6 | (int32_t)(s[2] & 0x3f) << 0;
        if (*c < 0x800 || (*c >= 0xd800 && *c <= 0xdfff)) {
            break;
        }
        return 3;
    case 0xf0:
        if ((s[1] & 0xc0) != 0x80)
            break;
        if ((s[2] & 0xc0) != 0x80)
            break;
        if ((s[3] & 0xc0) != 0x80)
            break;
        *c = (int32_t)(s[0] & 0x0f) << 18 | (int32_t)(s[1] & 0x3f) << 12 | (int32_t)(s[2] & 0x3f) << 6 | (int32_t)(s[3] & 0x3f) << 0;
        if (*c < 0x10000 || *c > 0x10ffff) {
            break;
        }
        return 4;
    }
    *c = 0xfffd;
    return 1;
}

void draw_text(const Font& font, const char* text, float font_size, float2 position, RGBA color, float max_width) {
    ZoneScoped;

    if (text == nullptr) {
        return;
    }

    float size_ratio = font_size / font.baked_font_size;

    float x = position.x;
    while (*text && x - position.x <= max_width) {
        int32_t c;
        auto bytes_skip = utf8decode_unsafe(text, &c);
        if (c == '\n') {
            position.y += font_size;
            x = position.x;
        } else {
            auto texture = font.texture_atlas;
            stbtt_packedchar char_info = font.char_data[c - char_start];

            float w = char_info.x1 - char_info.x0;
            float h = char_info.y1 - char_info.y0;
            auto src = Rect{(float)char_info.x0, (float)char_info.y0, w, h};
            auto dst = Rect{
                float2{x + char_info.xoff * size_ratio, font_size + position.y + char_info.yoff * size_ratio},
                {w * size_ratio, h * size_ratio}
            };

            //TODO: fix
            // draw_textured_rect(&renderer, font.texture_atlas, src, dst);
            draw_sprite_screen(dst, {.texture_id=font.texture_atlas, .src_rect=src});

            x += char_info.xadvance * size_ratio;
        }

        text += bytes_skip;
    }
}

float2 text_dimensions(const Font& font, const char* text, f32 font_size) {
    float max_width = 0;
    float width = 0;

    if (text == nullptr) {
        return {};
    }

    float size_ratio = font_size / font.baked_font_size;

    int lines = 1;

    while (*text) {
        int32_t c;
        auto bytes_skip = utf8decode_unsafe(text, &c);
        if (c == '\n') {
            max_width = f32_max(max_width, width);
            width = 0;
            lines++;
        } else {
            stbtt_packedchar char_info = font.char_data[c - char_start];

            width += char_info.xadvance;
        }

        text += bytes_skip;
    }
    max_width = f32_max(max_width, width);

    return float2{max_width * size_ratio, lines * font_size};
}

float text_height(const char* text, float font_size) {
    if (text == nullptr) {
        return 0;
    }

    return font_size;
}
