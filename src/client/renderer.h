#pragma once

// #include <SDL3/SDL.h>
// #include <glm/glm.hpp>
//
//
// #include "SDL3/SDL_gpu.h"

#include "assets.h"
#include "color.h"
#include "common/allocator.h"

struct Rect {
    glm::vec2 position;
    glm::vec2 scale;
};

struct Texture {
    SDL_GPUTexture* texture;
    SDL_GPUSampler* sampler;
    int w, h;
};

struct PositionColorVertex {
    glm::vec2 position;
    RGBA color;
};

struct PositionUvColorVertex {
    glm::vec2 position;
    glm::vec2 uv;
    RGBA color;
};

struct MixColor {
    glm::vec4 color;
    float t;
};

// struct TextureRectVert {
//     glm::vec2 position;
//     glm::vec2 
// }

struct Camera2D {
    glm::vec2 position;
    glm::vec2 scale;
};

struct RectVertex {
    glm::vec2 position;
    glm::vec2 uv;
};

struct SpriteVertex {
    glm::vec2 position;
    glm::vec2 size;
    glm::vec2 uv_position;
    glm::vec2 uv_size;
    
    RGBA mult_color;
    RGBA mix_color;
    float t;
};

struct TextureDrawInfo {
    std::vector<PositionUvColorVertex> verts;
    std::vector<uint16_t> indices;
};

// struct Slice {
//     int start, count;
// };
//
// struct TextureDraw {
//     Slice verts_slice;
//     Slice indices_slice;
//
//     TextureID texture;
//     MixColor mix_color;
// };

struct SpriteProperties {
    TextureID texture_id;
    std::optional<Rect> src_rect;
    RGBA mult_color=color::white;
    RGBA mix_color;
    float t;
};

enum class PipelineType {
    Sprite,
    Mesh,
    Line,
};

struct DrawItem {
    PipelineType type;

    TextureID texture_id;
    u32 vertex_buffer_offset;
    u32 vertices_size;

    u32 index_buffer_offset;
    u32 indices_size;
};

struct Renderer {
    const int window_width = 1024;
    const int window_height = 768;

    Camera2D* active_camera;

    SDL_GPUDevice* device;

    Texture textures[(int)TextureID::texture_count];

    SDL_GPUTexture* swapchain_texture;
    SDL_GPUTexture* depth_texture;
    SDL_GPUCommandBuffer* draw_command_buffer;
    SDL_GPURenderPass* render_pass;

    Slice<DrawItem> draw_list;
    Slice<u8> vertex_data;
    Slice<u8> index_data;
    SDL_GPUBuffer* dynamic_vertex_buffer;
    SDL_GPUBuffer* dynamic_index_buffer;
    SDL_GPUTransferBuffer* transfer_buffer;

    SDL_GPUGraphicsPipeline* line_pipeline;
    SDL_GPUGraphicsPipeline* solid_mesh_pipeline;
    SDL_GPUGraphicsPipeline* sprite_pipeline;
    SDL_GPUBuffer* rect_vertex_buffer;
    SDL_GPUBuffer* rect_index_buffer;

    bool null_swapchain = false;
};


struct Image {
    int w, h;
    void* data;
};


inline glm::vec2 screen_to_world_pos(const Camera2D& cam, glm::vec2 screen_pos, int s_w, int s_h) {
    screen_pos.y = s_h - screen_pos.y;

    return cam.position + (screen_pos / glm::vec2((float)s_w, (float)s_h) - 0.5f) * cam.scale;
}

glm::vec2 snap_pos(glm::vec2 pos);


namespace renderer {

SDL_GPUShader* load_shader( SDL_GPUDevice* device, const char* shaderPath, Uint32 samplerCount, Uint32 uniformBufferCount, Uint32 storageBufferCount, Uint32 storageTextureCount);

int init_renderer(Renderer* renderer_out, SDL_Window* window);
void begin_rendering(Renderer* renderer, SDL_Window* window, Arena* temp_arena);
void end_rendering(Renderer* renderer);

glm::vec2 world_to_normalized(Camera2D camera, glm::vec2 world_pos);
Rect world_rect_to_normalized(Camera2D camera, Rect world_rect);
Rect screen_rect_to_normalized(Rect rect, glm::vec2 resolution);

void draw_world_sprite(Renderer* renderer, Camera2D camera, Rect world_rect, const SpriteProperties& properties);
void draw_world_lines(Renderer* renderer, Camera2D camera, glm::vec2* vertices, int vert_count, RGBA color);
void draw_world_rect(Renderer* renderer, Camera2D camera, Rect rect, RGBA rgba);
void draw_screen_rect(Renderer* renderer, Rect rect, RGBA rgba);

Texture load_texture(Renderer* renderer, std::optional<TextureID> texture_id, const Image& image);

void draw_world_polygon(Renderer* renderer, const Camera2D& camera, glm::vec2* verts, int vert_count, RGBA color);

} // namespace renderer
