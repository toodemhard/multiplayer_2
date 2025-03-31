#pragma once

slice_def(stbtt_packedchar);

typedef struct Font Font;
struct Font {
    TextureID texture_atlas;
    Slice_stbtt_packedchar char_data;
    int w, h, baked_font_size;
};
slice_def(Font);

void font_load(Arena* arena, Font* font_output, Renderer* renderer, FontID font_id, int w, int h, int font_size);
void draw_text(Font font, const char* text, float font_size, float2 position, RGBA color, float max_width);
float2 text_dimensions(Font font, const char* text, f32 font_size);
float text_height(const char* text, float font_size);
void generate_font_atlas(Slice_u8* bitmap);

void save_font_atlas_image();
