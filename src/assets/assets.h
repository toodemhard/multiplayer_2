#pragma once

#define ASSET_PATH(a, b) b,
#define ASSET_NAME(a, b) a,

const char* shader_paths[] = {
    "null",
    SHADER_TABLE(ASSET_PATH)
};

#define SHADER_ID(a,b) ShaderID_##a,
typedef enum ShaderID {
    ShaderID_NULL,
    SHADER_TABLE(SHADER_ID)
    ShaderID_Count,
} ShaderID;

const char* image_paths[] = {
    "null",
    IMAGE_TABLE(ASSET_PATH)
};

#define IMAGE_ID(a,b) ImageID_##a,
typedef enum ImageID {
    ImageID_NULL,
    IMAGE_TABLE(IMAGE_ID)
    ImageID_Count,
} ImageID;

const char * font_paths[] = {
    FONT_TABLE(ASSET_PATH)
};

#define FONT_ID(a, b) FontID_##a,
typedef enum FontID {
    FONT_TABLE(FONT_ID)
    FontID_Count,
} FontID;

#define TEXTURE_ID(a,b) TextureID_##a,
typedef enum TextureID {
    TextureID_NULL,
    IMAGE_TABLE(TEXTURE_ID)
    FONT_TABLE(TEXTURE_ID)
    TextureID_Count,
} TextureID;

TextureID image_id_to_texture_id(ImageID image_id) {
    return (TextureID)((int)image_id);
}

TextureID font_id_to_texture_id(FontID font_id) {
    return (TextureID)((int)ImageID_Count + (int)font_id);
}
