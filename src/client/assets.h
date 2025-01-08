#pragma once
#include "codegen/image_table.h"
#include "codegen/font_table.h"

#define ASSET_PATH(a, b) b,
#define ASSET_NAME(a, b) a,

constexpr const char * image_paths[] {
    IMAGE_TABLE(ASSET_PATH)
};

enum class ImageID : int {
    IMAGE_TABLE(ASSET_NAME)
    image_count,
};

constexpr const char * font_paths[] {
    FONT_TABLE(ASSET_PATH)
};

enum class FontID : int {
    FONT_TABLE(ASSET_NAME)
    font_count,
};

enum class TextureID : int {
    IMAGE_TABLE(ASSET_NAME)
    FONT_TABLE(ASSET_NAME)
    texture_count,
};

inline TextureID image_id_to_texture_id(ImageID image_id) {
    return TextureID((int)image_id);
}

inline TextureID font_id_to_texture_id(FontID font_id) {
    return TextureID((int)ImageID::image_count + (int)font_id);
}
