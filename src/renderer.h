#pragma once

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "SDL3/SDL_gpu.h"
#include "color.h"

namespace renderer {

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

        SDL_GPUTexture* swapchain_texture;

        SDL_GPUCommandBuffer* draw_command_buffer;
        SDL_GPURenderPass* render_pass;
    };

    struct Rect {
        glm::vec2 position;
        glm::vec2 scale;
    };

    struct Image {
        int w, h;
        void* data;
    };

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
        const Rect* src_rect,
        const Rect& dst_rect,
        const Texture& texture,
        const RGB& color_mod = color::white
    );

    void draw_wire_rect(Renderer& renderer, const Rect& rect, const RGBA& color);

    int init_renderer(Renderer* renderer_out, SDL_Window* window);

    void begin_rendering(Renderer& renderer, SDL_Window* window);

    void end_rendering(Renderer& renderer);

    Texture load_texture(Renderer& renderer, const Image& image);

} // namespace renderer
