#pragma once

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include <optional>

#include "SDL3/SDL_gpu.h"
#include "color.h"

struct Texture {
    SDL_GPUTexture* texture;
    SDL_GPUSampler* sampler;
    int w, h;
};

struct Renderer {
    const int window_width = 1024;
    const int window_height = 768;

    SDL_GPUDevice* device;
    SDL_GPUGraphicsPipeline* textured_rect_pipeline;
    SDL_GPUGraphicsPipeline* wire_rect_pipeline;
    SDL_GPUGraphicsPipeline* vertex_buffer_pipeline;

    SDL_GPUGraphicsPipeline* world_textured_rect_pipeline;

    SDL_GPUTexture* swapchain_texture;

    SDL_GPUCommandBuffer* draw_command_buffer;
    SDL_GPURenderPass* render_pass;

    bool null_swapchain = false;
};

struct Rect {
    glm::vec2 position;
    glm::vec2 scale;
};

struct Image {
    int w, h;
    void* data;
};

struct Camera2D {
    glm::vec2 position;
    glm::vec2 scale;
};

inline glm::vec2 screen_to_world_pos(const Camera2D& cam, glm::vec2 screen_pos, int s_w, int s_h) {
    screen_pos.y = s_h - screen_pos.y;

    return cam.position + (screen_pos / glm::vec2((float)s_w, (float)s_h) - 0.5f) * cam.scale;
}

struct PositionColorVertex {
    glm::vec2 position;
    glm::vec2 uv;
    RGBA color;
};

namespace renderer {

SDL_GPUShader* load_shader(
    SDL_GPUDevice* device,
    const char* shaderPath,
    Uint32 samplerCount,
    Uint32 uniformBufferCount,
    Uint32 storageBufferCount,
    Uint32 storageTextureCount
);

void draw_textured_rect(
    Renderer& renderer,
    const std::optional<Rect>& src_rect,
    const Rect& dst_rect,
    const Texture& texture,
    const RGBA& color_mod = rgba::white
);

void draw_world_textured_rect(
    Renderer& renderer,
    const Camera2D& camera,
    const std::optional<Rect>& src_rect,
    const Rect& world_rect,
    const Texture& texture,
    const RGBA& color_mod = rgba::white
);

void draw_wire_rect(Renderer& renderer, const Rect& rect, const RGBA& color);

int init_renderer(Renderer* renderer_out, SDL_Window* window);

void begin_rendering(Renderer& renderer, SDL_Window* window);

void end_rendering(Renderer& renderer);

Texture load_texture(Renderer& renderer, const Image& image);

void render_geometry_raw(Renderer& renderer, const Texture& texture, const PositionColorVertex* vertices, int num_vertices, const void* indices, int num_indices, int index_size);

} // namespace renderer
