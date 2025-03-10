#pragma once

#include "color.h"

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
    TextureID texture_atlas;
    std::vector<stbtt_packedchar> char_data;
    int w, h, baked_font_size;
};

void font_load(Font* font_output, Renderer& renderer, FontID font_id, int w, int h, int font_size);
void draw_text(const Font& font, const char* text, float font_size, float2 position, RGBA color=color::white, float max_width = 99999);
float2 text_dimensions(const Font& font, const char* text, f32 font_size);
float text_height(const char* text, float font_size);
void generate_font_atlas(std::vector<unsigned char>& bitmap);

void save_font_atlas_image();
