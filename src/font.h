#pragma once

#include "color.h"
#include <SDL3/SDL.h>
#include <glm/vec2.hpp>
#include <vector>

#include "stb_truetype.h"
#include "renderer.h"

// void init_font(SDL_Renderer* renderer);
//
// void draw_text(SDL_Renderer* renderer, const char* text, float size, Vec2 position, RGBA color);
// void draw_text_cutoff(SDL_Renderer* renderer, const char* text, float font_size, Vec2 position, RGBA color, float max_width);
//
// Vec2 text_dimensions(const char* text, float font_size);
//
// float text_height(const char* text, float font_size);
//
// void kms();
struct Font {
    renderer::Texture atlas_texture;
    std::vector<stbtt_packedchar> char_data;
    int w, h, baked_font_size;
};

namespace font {
    void load_font(Font* font_output, renderer::Renderer renderer, const char* ttf_path, int w, int h, int font_size);
    void draw_text(renderer::Renderer& renderer, Font font, const char* text, float font_size, glm::vec2 position, RGBA color=rgba::white, float max_width = 99999);
    glm::vec2 text_dimensions(Font font, const char* text, float font_size);
    float text_height(const char* text, float font_size);
    void generate_font_atlas(std::vector<unsigned char>& bitmap);

    void save_font_atlas_image();
    //
    // void save_font_atlas_image();
    // void save_icons();
} // namespace Font
