#pragma once
#include "codegen/image_table.h"
#include "codegen/font_table.h"

#define ASSET_PATH(a, b) b,
#define ASSET_NAME(a, b) a,

constexpr const char * image_paths[] {
    IMAGE_TABLE(ASSET_PATH)
};

enum class ImageID {
    IMAGE_TABLE(ASSET_NAME)
    count,
};

constexpr const char * font_paths[] {
    FONT_TABLE(ASSET_PATH)
};

enum class FontID {
    FONT_TABLE(ASSET_NAME)
    count,
};
