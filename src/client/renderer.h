#pragma once

#include <SDL3/SDL.h>
#include <cstdint>
#include <glm/glm.hpp>

#include <optional>

#include "SDL3/SDL_gpu.h"
#include "color.h"

struct Texture {
    SDL_GPUTexture* texture;
    SDL_GPUSampler* sampler;
    int w, h;
};

struct PositionColorVertex {
    glm::vec2 position;
    RGBA color;
};

struct Camera2D {
    glm::vec2 position;
    glm::vec2 scale;
};

struct Renderer {
    const int window_width = 1024;
    const int window_height = 768;

    Camera2D* active_camera;

    SDL_GPUDevice* device;
    SDL_GPUGraphicsPipeline* textured_rect_pipeline;
    SDL_GPUGraphicsPipeline* raw_mesh_textured_pipeline;


    SDL_GPUGraphicsPipeline* world_textured_rect_pipeline;

    SDL_GPUTexture* swapchain_texture;

    SDL_GPUCommandBuffer* draw_command_buffer;
    SDL_GPURenderPass* render_pass;

    SDL_GPUGraphicsPipeline* wire_pipeline;
    std::vector<PositionColorVertex>  line_vertices;
    std::vector<uint16_t> line_indices;

    SDL_GPUGraphicsPipeline* raw_mesh_pipeline;
    std::vector<PositionColorVertex> mesh_vertices;
    std::vector<uint16_t> mesh_indices;

    SDL_GPUBuffer* dynamic_vertex_buffer;
    SDL_GPUBuffer* dynamic_index_buffer;
    SDL_GPUTransferBuffer* transfer_buffer;


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


inline glm::vec2 screen_to_world_pos(const Camera2D& cam, glm::vec2 screen_pos, int s_w, int s_h) {
    screen_pos.y = s_h - screen_pos.y;

    return cam.position + (screen_pos / glm::vec2((float)s_w, (float)s_h) - 0.5f) * cam.scale;
}

struct PositionUvColorVertex {
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
    const RGBA& color_mod = color::white
);

void draw_world_textured_rect(
    Renderer& renderer,
    const Camera2D& camera,
    const std::optional<Rect>& src_rect,
    const Rect& world_rect,
    const Texture& texture,
    const RGBA& color_mod = color::white
);

void draw_world_lines(Renderer& renderer, const Camera2D& camera, glm::vec2* vertices, int vert_count, RGBA color);

void draw_lines(Renderer& renderer, glm::vec2* vertices, int vert_count, RGBA color);

void draw_world_rect(Renderer& renderer, Camera2D camera, Rect rect, RGBA rgba);
void draw_screen_rect(Renderer& renderer, Rect rect, RGBA rgba);

void draw_wire_rect(Renderer& renderer, const Rect& rect, const RGBA& color);

int init_renderer(Renderer* renderer_out, SDL_Window* window);

void begin_rendering(Renderer& renderer, SDL_Window* window);

void end_rendering(Renderer& renderer);

Texture load_texture(Renderer& renderer, const Image& image);

void draw_world_polygon(Renderer& renderer, const Camera2D& camera, glm::vec2* verts, int vert_count, RGBA color);

void render_geometry(Renderer& renderer, const Texture& texture, const PositionUvColorVertex* vertices, int num_vertices, const void* indices, int num_indices, int index_size);
void render_geometry_textured(Renderer& renderer, const Texture& texture, const PositionUvColorVertex* vertices, int num_vertices, const void* indices, int num_indices, int index_size);

} // namespace renderer
