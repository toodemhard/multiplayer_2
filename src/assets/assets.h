#pragma once

#define ASSET_PATH(a, b) b,
#define ASSET_NAME(a, b) a,

constexpr const char * image_paths[] {
    "null",
    IMAGE_TABLE(ASSET_PATH)
};

#define IMAGE_ID(a,b) ImageID_##a,
enum ImageID {
    ImageID_NULL,
    IMAGE_TABLE(IMAGE_ID)
    ImageID_Count,
};

constexpr const char * font_paths[] {
    FONT_TABLE(ASSET_PATH)
};

enum FontID {
    FONT_TABLE(ASSET_NAME)
    font_count,
};

#define TEXTURE_ID(a,b) TextureID_##a,
enum TextureID : int {
    TextureID_NULL,
    IMAGE_TABLE(TEXTURE_ID)
    FONT_TABLE(TEXTURE_ID)
    TextureID_Count,
};

inline TextureID image_id_to_texture_id(ImageID image_id) {
    return TextureID((int)image_id);
}

inline TextureID font_id_to_texture_id(FontID font_id) {
    return TextureID((int)ImageID_Count + (int)font_id);
}
