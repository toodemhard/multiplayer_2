#pragma once

struct Font {
    TextureID texture_atlas;
    Slice<stbtt_packedchar> char_data;
    int w, h, baked_font_size;
};

void font_load(Arena* arena, Font* font_output, Renderer* renderer, FontID font_id, int w, int h, int font_size);
void draw_text(const Font& font, const char* text, float font_size, float2 position, RGBA color=color::white, float max_width = 99999);
float2 text_dimensions(const Font& font, const char* text, f32 font_size);
float text_height(const char* text, float font_size);
void generate_font_atlas(Slice<u8>* bitmap);

void save_font_atlas_image();
