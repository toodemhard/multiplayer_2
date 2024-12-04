#include "renderer.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_render.h"
#include "codegen/shaders.h"
#include "color.h"
#include <cstring>

namespace renderer {

int init_renderer(Renderer* renderer_out, SDL_Window* window) {
    auto device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    if (device == NULL) {
        SDL_Log("CreateGPUDevice failed");
        return 1;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        SDL_Log("ClaimWindow failed");
        return 1;
    }

    SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_IMMEDIATE);

    {
        auto color_target_description = SDL_GPUColorTargetDescription{
            .format = SDL_GetGPUSwapchainTextureFormat(device, window),
            .blend_state =
                {
                    .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                    .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .color_blend_op = SDL_GPU_BLENDOP_ADD,
                    .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
                    .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
                    .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                    .enable_blend = true,
                }
        };

        auto vertex_shader = load_shader(device, shaders::textured_rect_vert, 0, 2, 0, 0);
        auto fragment_shader = load_shader(device, shaders::textured_rect_frag, 1, 1, 0, 0);

        auto create_info = SDL_GPUGraphicsPipelineCreateInfo{
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
            .target_info =
                {
                    .color_target_descriptions = &color_target_description, .num_color_targets = 1,
                    // .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                },
        };
        // create_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        renderer_out->textured_rect_pipeline = SDL_CreateGPUGraphicsPipeline(device, &create_info);
        renderer_out->device = device;

        SDL_ReleaseGPUShader(device, vertex_shader);
        SDL_ReleaseGPUShader(device, fragment_shader);
    }

    {
        auto vertex_shader = load_shader(device, shaders::wire_rect_vert, 0, 1, 0, 0);
        auto fragment_shader = load_shader(device, shaders::wire_rect_frag, 0, 1, 0, 0);

        auto create_info = SDL_GPUGraphicsPipelineCreateInfo{
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .primitive_type = SDL_GPU_PRIMITIVETYPE_LINESTRIP,
            .target_info =
                {
                    .color_target_descriptions =
                        (SDL_GPUColorTargetDescription[]){{.format = SDL_GetGPUSwapchainTextureFormat(device, window)}},
                    .num_color_targets = 1,
                },
        };
        // create_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        renderer_out->wire_rect_pipeline = SDL_CreateGPUGraphicsPipeline(device, &create_info);
        renderer_out->device = device;

        SDL_ReleaseGPUShader(device, vertex_shader);
        SDL_ReleaseGPUShader(device, fragment_shader);
    }

    {
        auto color_target_description = SDL_GPUColorTargetDescription{
            .format = SDL_GetGPUSwapchainTextureFormat(device, window),
            .blend_state =
                {
                    .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                    .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .color_blend_op = SDL_GPU_BLENDOP_ADD,
                    .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
                    .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
                    .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                    .enable_blend = true,
                }
        };

        auto vertex_shader = load_shader(device, shaders::pos_color_uv_vert, 0, 0, 0, 0);
        auto fragment_shader = load_shader(device, shaders::color_texture_frag, 1, 0, 0, 0);

        auto pipeline_create_info = SDL_GPUGraphicsPipelineCreateInfo{
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .vertex_input_state = SDL_GPUVertexInputState{
                .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]) {SDL_GPUVertexBufferDescription{
                    .slot = 0,
                    .pitch = sizeof(PositionColorVertex),
                    .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                    .instance_step_rate = 0,
                }},
                .num_vertex_buffers = 1,
                .vertex_attributes = (SDL_GPUVertexAttribute[]) {
                SDL_GPUVertexAttribute{
                    .location = 0,
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                    .offset = 0,
                },
                SDL_GPUVertexAttribute {
                    .location = 1,
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                    .offset = sizeof(glm::vec2),
                },
                SDL_GPUVertexAttribute {
                    .location = 2,
                    .buffer_slot = 0,
                    .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
                    .offset = sizeof(glm::vec2) * 2,
                },
                },
                .num_vertex_attributes = 3,
            },
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .target_info = {
                .color_target_descriptions = &color_target_description,
                .num_color_targets = 1,
            },
        };
        // create_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        renderer_out->vertex_buffer_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_create_info);
        renderer_out->device = device;

        SDL_ReleaseGPUShader(device, vertex_shader);
        SDL_ReleaseGPUShader(device, fragment_shader);
    }

    return 0;
}

SDL_GPUShader* load_shader(
    SDL_GPUDevice* device,
    const char* shaderPath,
    Uint32 samplerCount,
    Uint32 uniformBufferCount,
    Uint32 storageBufferCount,
    Uint32 storageTextureCount
) {
    SDL_GPUShaderStage stage;
    // Auto-detect the shader stage from the file name for convenience
    if (SDL_strstr(shaderPath, ".vert")) {
        stage = SDL_GPU_SHADERSTAGE_VERTEX;
    } else if (SDL_strstr(shaderPath, ".frag")) {
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    } else {
        SDL_Log("Invalid shader stage!");
        return NULL;
    }

    size_t codeSize;
    auto code = (const unsigned char*)SDL_LoadFile(shaderPath, &codeSize);
    if (code == NULL) {
        SDL_Log("Failed to load shader from disk! %s", shaderPath);
        return NULL;
    }

    SDL_GPUShaderCreateInfo shaderInfo = {
        .code_size = codeSize,
        .code = code,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = stage,
        .num_samplers = samplerCount,
        .num_storage_textures = storageTextureCount,
        .num_storage_buffers = storageBufferCount,
        .num_uniform_buffers = uniformBufferCount,
    };
    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderInfo);
    if (shader == NULL) {
        SDL_Log("Failed to create shader!");
        SDL_free((void*)code);
        return NULL;
    }

    SDL_free((void*)code);
    return shader;
}

void draw_wire_rect(Renderer& renderer, const Rect& rect, const RGBA& color) {
    SDL_BindGPUGraphicsPipeline(renderer.render_pass, renderer.wire_rect_pipeline);
    auto resolution = glm::vec2{renderer.window_width, renderer.window_height};

    auto normalized_screen_rect =
        Rect{.position = (rect.position + rect.scale / 2.0f) / resolution * 2.0f - 1.0f, .scale = rect.scale / resolution};
    normalized_screen_rect.position.y *= -1;

    auto color_normalized = glm::vec4(color.r, color.g, color.b, color.a) / 255.0f;

    SDL_PushGPUVertexUniformData(renderer.draw_command_buffer, 0, &normalized_screen_rect, sizeof(Rect));
    SDL_PushGPUFragmentUniformData(renderer.draw_command_buffer, 0, &color_normalized, sizeof(glm::vec4));

    SDL_DrawGPUPrimitives(renderer.render_pass, 5, 1, 0, 0);
}

void draw_textured_rect(
    Renderer& renderer,
    const std::optional<Rect>& src_rect,
    const Rect& dst_rect,
    const Texture& texture,
    const RGBA& color_mod
) {
    SDL_BindGPUGraphicsPipeline(renderer.render_pass, renderer.textured_rect_pipeline);

    auto texture_sampler_binding = SDL_GPUTextureSamplerBinding{.texture = texture.texture, .sampler = texture.sampler};
    SDL_BindGPUFragmentSamplers(renderer.render_pass, 0, &texture_sampler_binding, 1);

    auto resolution = glm::vec2{renderer.window_width, renderer.window_height};

    auto normalized_screen_rect =
        Rect{.position = (dst_rect.position + dst_rect.scale / 2.0f) / resolution * 2.0f - 1.0f, .scale = dst_rect.scale / resolution};
    normalized_screen_rect.position.y *= -1;

    Rect normalized_texture_rect;
    if (src_rect.has_value()) {
        normalized_texture_rect = {
            .position = src_rect->position / glm::vec2{texture.w, texture.h},
            .scale = src_rect->scale / glm::vec2{texture.w, texture.h},
        };
    } else {
        normalized_texture_rect = Rect{{0, 0}, {1, 1}};
    }

    auto color_mod_normalized = glm::vec4{color_mod.r, color_mod.g, color_mod.b, color_mod.a} / 255.0f;

    SDL_PushGPUVertexUniformData(renderer.draw_command_buffer, 0, &normalized_screen_rect, sizeof(Rect));
    SDL_PushGPUVertexUniformData(renderer.draw_command_buffer, 1, &normalized_texture_rect, sizeof(Rect));
    SDL_PushGPUFragmentUniformData(renderer.draw_command_buffer, 0, &color_mod_normalized, sizeof(glm::vec4));

    SDL_DrawGPUPrimitives(renderer.render_pass, 4, 1, 0, 0);
}

void draw_world_textured_rect(
    Renderer& renderer,
    const Camera2D& camera,
    const std::optional<Rect>& src_rect,
    const Rect& world_rect,
    const Texture& texture,
    const RGBA& color_mod
) {
    SDL_BindGPUGraphicsPipeline(renderer.render_pass, renderer.textured_rect_pipeline);

    auto texture_sampler_binding = SDL_GPUTextureSamplerBinding{.texture = texture.texture, .sampler = texture.sampler};
    SDL_BindGPUFragmentSamplers(renderer.render_pass, 0, &texture_sampler_binding, 1);

    auto normalized_screen_rect = Rect{
        (world_rect.position - camera.position) / camera.scale * 2.0f,
        world_rect.scale / camera.scale,
    };

    Rect normalized_texture_rect;
    if (src_rect.has_value()) {
        normalized_texture_rect = {
            .position = src_rect->position / glm::vec2{texture.w, texture.h},
            .scale = src_rect->scale / glm::vec2{texture.w, texture.h},
        };
    } else {
        normalized_texture_rect = Rect{{0, 0}, {1, 1}};
    }

    auto color_mod_normalized = glm::vec4{color_mod.r, color_mod.g, color_mod.b, color_mod.a} / 255.0f;

    SDL_PushGPUVertexUniformData(renderer.draw_command_buffer, 0, &normalized_screen_rect, sizeof(Rect));
    SDL_PushGPUVertexUniformData(renderer.draw_command_buffer, 1, &normalized_texture_rect, sizeof(Rect));
    SDL_PushGPUFragmentUniformData(renderer.draw_command_buffer, 0, &color_mod_normalized, sizeof(glm::vec4));

    SDL_DrawGPUPrimitives(renderer.render_pass, 4, 1, 0, 0);
}

void begin_rendering(Renderer& renderer, SDL_Window* window) {
    renderer.draw_command_buffer = SDL_AcquireGPUCommandBuffer(renderer.device);

    SDL_GPUTexture* swapchain_texture;
    SDL_AcquireGPUSwapchainTexture(renderer.draw_command_buffer, window, &swapchain_texture, NULL, NULL);

    // if (swapchain_texture != NULL) {
    SDL_GPUColorTargetInfo color_target_info = {0};
    color_target_info.texture = swapchain_texture;
    color_target_info.clear_color = (SDL_FColor){0.0f, 0.0f, 0.0f, 1.0f};
    color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target_info.store_op = SDL_GPU_STOREOP_STORE;

    renderer.render_pass = SDL_BeginGPURenderPass(renderer.draw_command_buffer, &color_target_info, 1, NULL);
}

void end_rendering(Renderer& renderer) {
    SDL_EndGPURenderPass(renderer.render_pass);
    SDL_SubmitGPUCommandBuffer(renderer.draw_command_buffer);
}

Texture load_texture(Renderer& renderer, const Image& image) {
    const int comp = 4;

    auto texture_create_info = SDL_GPUTextureCreateInfo{
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = (uint32_t)image.w,
        .height = (uint32_t)image.h,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };

    auto texture = SDL_CreateGPUTexture(renderer.device, &texture_create_info);

    auto sampler_create_info = SDL_GPUSamplerCreateInfo{
        .min_filter = SDL_GPU_FILTER_NEAREST,
        .mag_filter = SDL_GPU_FILTER_NEAREST,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    };

    auto sampler = SDL_CreateGPUSampler(renderer.device, &sampler_create_info);

    auto upload_command_buffer = SDL_AcquireGPUCommandBuffer(renderer.device);
    auto copy_pass = SDL_BeginGPUCopyPass(upload_command_buffer);

    auto transfer_create_info = SDL_GPUTransferBufferCreateInfo{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = (uint32_t)(image.w * image.h) * comp,
    };

    auto texture_transfer_buffer = SDL_CreateGPUTransferBuffer(renderer.device, &transfer_create_info);

    auto texture_transfer_ptr = SDL_MapGPUTransferBuffer(renderer.device, texture_transfer_buffer, false);
    memcpy(texture_transfer_ptr, image.data, image.w * image.h * comp);

    SDL_UnmapGPUTransferBuffer(renderer.device, texture_transfer_buffer);

    auto texture_transfer_info = SDL_GPUTextureTransferInfo{
        .transfer_buffer = texture_transfer_buffer,
    };
    auto texture_region = SDL_GPUTextureRegion{
        .texture = texture,
        .w = (Uint32)image.w,
        .h = (Uint32)image.h,
        .d = 1,
    };

    SDL_UploadToGPUTexture(copy_pass, &texture_transfer_info, &texture_region, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_command_buffer);
    SDL_ReleaseGPUTransferBuffer(renderer.device, texture_transfer_buffer);

    return Texture{.texture = texture, .sampler = sampler, .w = image.w, .h = image.h};
}


void render_geometry_raw(Renderer& renderer, const Texture& texture, const PositionColorVertex* vertices, int num_vertices) {
    auto buffer_size = (uint32_t)(sizeof(PositionColorVertex) * num_vertices);


    auto vertex_buffer_create_info = SDL_GPUBufferCreateInfo{
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = buffer_size,
    };

    auto vertex_buffer = SDL_CreateGPUBuffer(renderer.device, &vertex_buffer_create_info);

    auto transfer_buffer_create_info = SDL_GPUTransferBufferCreateInfo {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = buffer_size,
    };

    auto transfer_buffer = SDL_CreateGPUTransferBuffer(renderer.device, &transfer_buffer_create_info);

    auto transfer_data = (PositionColorVertex*)SDL_MapGPUTransferBuffer(renderer.device, transfer_buffer, false);

    memcpy(transfer_data, vertices, buffer_size);

    SDL_UnmapGPUTransferBuffer(renderer.device, transfer_buffer);

    auto upload_command_buffer = SDL_AcquireGPUCommandBuffer(renderer.device);

    auto copy_pass = SDL_BeginGPUCopyPass(upload_command_buffer);

    auto transfer_buffer_location = SDL_GPUTransferBufferLocation {
        .transfer_buffer = transfer_buffer,
        .offset = 0,
    };

    auto gpu_buffer_region = SDL_GPUBufferRegion {
        .buffer = vertex_buffer,
        .offset = 0,
        .size = buffer_size,
    };

    SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &gpu_buffer_region, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_command_buffer);
    SDL_ReleaseGPUTransferBuffer(renderer.device, transfer_buffer);

    SDL_BindGPUGraphicsPipeline(renderer.render_pass, renderer.vertex_buffer_pipeline);

    auto texture_sampler_binding = SDL_GPUTextureSamplerBinding{.texture = texture.texture, .sampler = texture.sampler};
    SDL_BindGPUFragmentSamplers(renderer.render_pass, 0, &texture_sampler_binding, 1);

    auto buffer_binding = SDL_GPUBufferBinding {
        .buffer = vertex_buffer,
        .offset = 0,
    };
    SDL_BindGPUVertexBuffers(renderer.render_pass, 0, &buffer_binding, 1);

    SDL_DrawGPUPrimitives(renderer.render_pass, num_vertices, 1, 0, 0);


    SDL_ReleaseGPUBuffer(renderer.device, vertex_buffer);
}

// SDL_RenderGeometryRaw(SDL_Renderer *renderer, SDL_Texture *texture, const float *xy, int xy_stride, const SDL_FColor *color, int color_stride, const float *uv, int uv_stride, int num_vertices, const void *indices, int num_indices, int size_indices)
// return SDL_RenderGeometryRaw(renderer, texture, xy, xy_stride, color3, sizeof(*color3), uv, uv_stride, num_vertices, indices, num_indices, size_indices);

} // namespace renderer
