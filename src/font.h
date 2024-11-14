#pragma once

#include <SDL3/SDL.h>
#include <glm/vec2.hpp>
#include <vector>
#include "color.h"

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


namespace Font {
    void init_fonts(SDL_Renderer* renderer);
    void draw_text(SDL_Renderer* renderer, const char* text, float font_size, glm::vec2 position, RGBA color, float max_width);
    glm::vec2 text_dimensions(const char* text, float font_size);
    float text_height(const char* text, float font_size);
    void generate_text_font_atlas(std::vector<unsigned char>& bitmap);

    void save_font_atlas_image();
    //
    // void save_font_atlas_image();
    // void save_icons();
}
