#include "renderer.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_stdinc.h"
#include "assets.h"
#include "codegen/shaders.h"
#include "color.h"
#include "glm/ext/vector_float2.hpp"
#include "panic.h"
#include "types.h"
#include <cstdint>
#include <cstring>

#include <tracy/Tracy.hpp>

namespace renderer {

int init_renderer(Renderer* renderer, SDL_Window* window) {
    ZoneScoped;

    // *renderer = {};

    auto device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    if (device == NULL) {
        SDL_Log("CreateGPUDevice failed");
        return 1;
    }
    renderer->device = device;

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        SDL_Log("ClaimWindow failed");
        return 1;
    }

    SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_IMMEDIATE);

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

    // lines pipeline
    {
        auto vertex_shader = load_shader(device, shaders::raw_vertex_buffer_vert, 0, 0, 0, 0);
        auto fragment_shader = load_shader(device, shaders::solid_color_frag, 0, 0, 0, 0);

        SDL_GPUVertexBufferDescription descriptions[] = {SDL_GPUVertexBufferDescription{
            .slot = 0,
            .pitch = sizeof(PositionColorVertex),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            .instance_step_rate = 0,
        }};

        SDL_GPUVertexAttribute attributes[] = {
            SDL_GPUVertexAttribute{
                .location = 0,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                .offset = 0,
            },
            SDL_GPUVertexAttribute{
                .location = 1,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
                .offset = sizeof(glm::vec2),
            },
        };

        auto create_info = SDL_GPUGraphicsPipelineCreateInfo{
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .vertex_input_state =
                SDL_GPUVertexInputState{
                    .vertex_buffer_descriptions = descriptions,
                    .num_vertex_buffers = 1,
                    .vertex_attributes = attributes,
                    .num_vertex_attributes = 2,
                },
            .primitive_type = SDL_GPU_PRIMITIVETYPE_LINELIST,
            .target_info =
                {
                    .color_target_descriptions = &color_target_description,
                    .num_color_targets = 1,
                },
        };
        // create_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        renderer->line_pipeline = SDL_CreateGPUGraphicsPipeline(device, &create_info);

        SDL_ReleaseGPUShader(device, vertex_shader);
        SDL_ReleaseGPUShader(device, fragment_shader);
    }


    // auto transfer_data = (PositionUvColorVertex*)SDL_MapGPUTransferBuffer(renderer->device, renderer->transfer_buffer, false);

    // filled mesh pipeline
    {
        auto vertex_shader = load_shader(device, shaders::raw_vertex_buffer_vert, 0, 0, 0, 0);
        auto fragment_shader = load_shader(device, shaders::solid_color_frag, 0, 0, 0, 0);

        SDL_GPUVertexBufferDescription descriptions[] = {SDL_GPUVertexBufferDescription{
            .slot = 0,
            .pitch = sizeof(PositionColorVertex),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            .instance_step_rate = 0,
        }};

        SDL_GPUVertexAttribute attributes[] = {
            SDL_GPUVertexAttribute{
                .location = 0,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                .offset = 0,
            },
            SDL_GPUVertexAttribute{
                .location = 1,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
                .offset = sizeof(glm::vec2),
            },
        };

        auto pipeline_create_info = SDL_GPUGraphicsPipelineCreateInfo{
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .vertex_input_state =
                SDL_GPUVertexInputState{
                    .vertex_buffer_descriptions = descriptions,
                    .num_vertex_buffers = 1,
                    .vertex_attributes = attributes,
                    .num_vertex_attributes = 2,
                },
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .target_info =
                {
                    .color_target_descriptions = &color_target_description,
                    .num_color_targets = 1,
                },
        };

        // create_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        renderer->solid_mesh_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_create_info);
        if (renderer->solid_mesh_pipeline == NULL) {
            panic("{}", SDL_GetError());
        }

        SDL_ReleaseGPUShader(device, vertex_shader);
        SDL_ReleaseGPUShader(device, fragment_shader);
    }

    // mesh textured pipeline
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

        SDL_GPUVertexBufferDescription descriptions[] = {
            SDL_GPUVertexBufferDescription{
                .slot = 0,
                .pitch = sizeof(PositionUvColorVertex),
                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            },
        };

        SDL_GPUVertexAttribute attributes[] = {
            SDL_GPUVertexAttribute{
                .location = 0,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            },
            SDL_GPUVertexAttribute{
                .location = 1,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            },
            SDL_GPUVertexAttribute{
                .location = 2,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
            },
        };

        int sizes[] = { 8, 8, 4 };
        int offset = 0;
        for (int i = 0; i < sizeof(sizes) / sizeof(int) ; i++) {
            attributes[i].offset = offset;
            offset += sizes[i];
        }

        auto pipeline_create_info = SDL_GPUGraphicsPipelineCreateInfo{
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .vertex_input_state =
                SDL_GPUVertexInputState{
                    .vertex_buffer_descriptions = descriptions,
                    .num_vertex_buffers = 1,
                    .vertex_attributes = attributes,
                    .num_vertex_attributes = 3,
                },
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .target_info =
                {
                    .color_target_descriptions = &color_target_description,
                    .num_color_targets = 1,
                },
        };
        // create_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        renderer->textured_mesh_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_create_info);

        SDL_ReleaseGPUShader(device, vertex_shader);
        SDL_ReleaseGPUShader(device, fragment_shader);
    }

    // reusable buffers
    {
        auto index_buffer_size = (uint32_t)(2 * 1500);
        auto vertex_buffer_size = (uint32_t)(sizeof(PositionColorVertex) * 1000);

        auto vertex_buffer_create_info = SDL_GPUBufferCreateInfo{
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = vertex_buffer_size,
        };

        renderer->dynamic_vertex_buffer = SDL_CreateGPUBuffer(renderer->device, &vertex_buffer_create_info);

        auto index_buffer_create_info = SDL_GPUBufferCreateInfo{
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = index_buffer_size,
        };

        renderer->dynamic_index_buffer = SDL_CreateGPUBuffer(renderer->device, &index_buffer_create_info);

        auto transfer_buffer_create_info = SDL_GPUTransferBufferCreateInfo{
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = vertex_buffer_size + index_buffer_size,
        };

        renderer->transfer_buffer = SDL_CreateGPUTransferBuffer(renderer->device, &transfer_buffer_create_info);
    }

    // create static normalized rect buffers
    {
        float x = 0.5;
        RectVertex verts[] = {
            {{-x, x}, {0, 0}},
            {{x, x}, {1, 0}},
            {{-x,-x}, {0, 1}},
            {{x,-x}, {1, 1}},
        };

        u32 verts_size = sizeof(verts);

        u16 indices[] = {
            0,3,1,0,2,3
        };

        u32 indices_size = sizeof(indices);

        {
            auto createinfo = SDL_GPUBufferCreateInfo{
                .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                .size = verts_size,
            };

            renderer->rect_vertex_buffer = SDL_CreateGPUBuffer(renderer->device, &createinfo);
        }

        {
            SDL_GPUBufferCreateInfo createinfo = {
                .usage = SDL_GPU_BUFFERUSAGE_INDEX,
                .size = indices_size,
            };

            renderer->rect_index_buffer = SDL_CreateGPUBuffer(renderer->device, &createinfo);
        }
        // auto transfer_buffer_create_info = SDL_GPUTransferBufferCreateInfo{
        //     .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        //     .size = ,
        // };


        // auto transfer_buffer = SDL_CreateGPUTransferBuffer(renderer->device, &transfer_buffer_create_info);

        auto map_pointer = SDL_MapGPUTransferBuffer(renderer->device, renderer->transfer_buffer, false);

        memcpy(map_pointer, verts, verts_size);
        memcpy((u8*)map_pointer + verts_size, indices, indices_size);

        SDL_UnmapGPUTransferBuffer(renderer->device, renderer->transfer_buffer);

        auto command_buffer = SDL_AcquireGPUCommandBuffer(renderer->device);
        auto copy_pass = SDL_BeginGPUCopyPass(command_buffer);

        {
            SDL_GPUTransferBufferLocation buffer_location = {
                .transfer_buffer = renderer->transfer_buffer,
                .offset = 0,
            };

            SDL_GPUBufferRegion buffer_region = {
                .buffer = renderer->rect_vertex_buffer,
                .offset = 0,
                .size = verts_size,
            };

            SDL_UploadToGPUBuffer(copy_pass, &buffer_location, &buffer_region, false);
        }

        {
            SDL_GPUTransferBufferLocation buffer_location = {
                .transfer_buffer = renderer->transfer_buffer,
                .offset = verts_size,
            };

            SDL_GPUBufferRegion buffer_region = {
                .buffer = renderer->rect_index_buffer,
                .offset = 0,
                .size = indices_size,
            };

            SDL_UploadToGPUBuffer(copy_pass, &buffer_location, &buffer_region, false);
        }

        SDL_EndGPUCopyPass(copy_pass);
        SDL_SubmitGPUCommandBuffer(command_buffer);
    }

    // sprite pipeline
    {
        auto vertex_shader = load_shader(device, shaders::sprite_vert, 0, 0, 0, 0);
        auto fragment_shader = load_shader(device, shaders::sprite_frag, 1, 0, 0, 0);

        SDL_GPUVertexBufferDescription descriptions[] = {
            SDL_GPUVertexBufferDescription{
                .slot = 0,
                .pitch = sizeof(RectVertex),
                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            },
            SDL_GPUVertexBufferDescription{
                .slot = 1,
                .pitch = sizeof(SpriteDraw),
                .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
                .instance_step_rate = 1,
            },
        };

        SDL_GPUVertexAttribute attributes[] = {
            SDL_GPUVertexAttribute {
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                .offset = 0,
            },
            SDL_GPUVertexAttribute {
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                .offset = sizeof(glm::vec2),
            },
            SDL_GPUVertexAttribute {
                .buffer_slot = 1,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            },
            SDL_GPUVertexAttribute {
                .buffer_slot = 1,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            },
            SDL_GPUVertexAttribute {
                .buffer_slot = 1,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            },
            SDL_GPUVertexAttribute {
                .buffer_slot = 1,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            },
            SDL_GPUVertexAttribute {
                .buffer_slot = 1,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
            },
            SDL_GPUVertexAttribute {
                .buffer_slot = 1,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
            },
            SDL_GPUVertexAttribute {
                .buffer_slot = 1,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
            },
        };

        for (int i = 0; i < 9; i++) {
            attributes[i].location = i;
        };

        int sizes[] = { 8, 8, 8, 8, 4, 4, 4 };
        int offset = 0;
        for (int i = 0; i < 7; i++) {
            attributes[i + 2].offset = offset;
            offset += sizes[i];
        }

        SDL_GPUGraphicsPipelineCreateInfo createinfo = {
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .vertex_input_state =
                SDL_GPUVertexInputState{
                    .vertex_buffer_descriptions = descriptions,
                    .num_vertex_buffers = 2,
                    .vertex_attributes = attributes,
                    .num_vertex_attributes = 9,
                },
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .target_info =
                {
                    .color_target_descriptions = &color_target_description,
                    .num_color_targets = 1,
                },
        };

        renderer->test_pipeline = SDL_CreateGPUGraphicsPipeline(renderer->device, &createinfo);

        
    }

    return 0;
}

void draw_world_sprite(Renderer& renderer, const Camera2D camera, Rect world_rect, const SpriteProperties& properties) {
    auto normalized_rect = world_rect_to_normalized(camera, world_rect);

    auto normalized_texture_rect = Rect{{0,0}, {1,1}};
    auto& texture = renderer.textures[(int)properties.texture_id];
    if (properties.src_rect.has_value()) {
        auto& src_rect = properties.src_rect.value();
        normalized_texture_rect = {
            .position = src_rect.position / glm::vec2{texture.w, texture.h},
            .scale = src_rect.scale / glm::vec2{texture.w, texture.h},
        };
    }

    SpriteDraw sprite_draw = {
        .position = normalized_rect.position,
        .size = normalized_rect.scale,
        .uv_position = normalized_texture_rect.position,
        .uv_size = normalized_texture_rect.scale,
        .mult_color = properties.mult_color,
        .mix_color = properties.mix_color,
        .t = properties.t,
    };

    renderer.sprite_draws[(int)properties.texture_id].push_back(sprite_draw);
}


void draw_screen_rect_2(Renderer &renderer, Rect rect, RGBA rgba) {

    SDL_GPUBuffer* vertex_buffer;
    {
        SDL_GPUBufferCreateInfo createinfo = {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = 1000,
        };

        vertex_buffer = SDL_CreateGPUBuffer(renderer.device, &createinfo);
    }

    SDL_GPUBuffer* index_buffer;
    {
        SDL_GPUBufferCreateInfo createinfo = {
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = 1000,
        };

        index_buffer = SDL_CreateGPUBuffer(renderer.device, &createinfo);
    }

    void* ptr = SDL_MapGPUTransferBuffer(renderer.device, renderer.transfer_buffer, false);

    Rect rects[] = {
        {{0,0}, {0.2,0.2}},
        {{-1,1}, {0.5,0.5}},
    };
    u16 indices[] = {
        0,3,1,0,2,3
    };

    memcpy(ptr, rects, sizeof(Rect) * 2);
    memcpy((uint8_t*)ptr + sizeof(Rect) * 2, indices, sizeof(int) * 12);

    SDL_UnmapGPUTransferBuffer(renderer.device, renderer.transfer_buffer);

    auto command_buffer = SDL_AcquireGPUCommandBuffer(renderer.device);
    auto copy_pass = SDL_BeginGPUCopyPass(command_buffer);

    {
        SDL_GPUTransferBufferLocation transfer_location = {
            .transfer_buffer = renderer.transfer_buffer,
            .offset = 0,
        };
        SDL_GPUBufferRegion buffer_region = {
            .buffer = vertex_buffer,
            .offset = 0,
            .size = sizeof(Rect) * 2,
        };
        SDL_UploadToGPUBuffer(copy_pass, &transfer_location, &buffer_region, false);
    }
    {
        SDL_GPUTransferBufferLocation transfer_location = {
            .transfer_buffer = renderer.transfer_buffer,
            .offset = sizeof(Rect) * 2,
        };
        SDL_GPUBufferRegion buffer_region = {
            .buffer = index_buffer,
            .offset = 0,
            .size = sizeof(u16) * 6,
        };
        SDL_UploadToGPUBuffer(copy_pass, &transfer_location, &buffer_region, false);
    }

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);

    SDL_BindGPUGraphicsPipeline(renderer.render_pass, renderer.test_pipeline);
    {
        SDL_GPUBufferBinding bindings[] = {
            {
                .buffer = renderer.rect_vertex_buffer,
                .offset = 0,
            },
            {
                .buffer = vertex_buffer,
                .offset = 0,
            }
        };

        SDL_BindGPUVertexBuffers(renderer.render_pass, 0, bindings, 2);
    }

    SDL_GPUBufferBinding binding = {
        .buffer = index_buffer,
        .offset = 0,
    };
    SDL_BindGPUIndexBuffer(renderer.render_pass, &binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    // SDL_
    // SDL_BindGPUIndexBuffer(renderer.in, const SDL_GPUBufferBinding *binding, SDL_GPUIndexElementSize index_element_size)
    //
    // SDL_DrawGPUIndexedPrimitives(renderer.r, 4, Uint32 num_instances, Uint32 first_index, Sint32 vertex_offset, Uint32 first_instance)

    // SDL_DrawGPUPrimitives(renderer.render_pass, 4, 1, 0, 0);
    SDL_DrawGPUIndexedPrimitives(renderer.render_pass, 6, 2, 0, 0, 0);

}


SDL_GPUShader* load_shader(
    SDL_GPUDevice* device,
    const char* shaderPath,
    Uint32 samplerCount,
    Uint32 uniformBufferCount,
    Uint32 storageBufferCount,
    Uint32 storageTextureCount
) {
    ZoneScoped;

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

// void draw_wire_rect(Renderer& renderer, const Rect& rect, const RGBA& color) {
//     if (renderer.null_swapchain) {
//         return;
//     }
//
//     SDL_BindGPUGraphicsPipeline(renderer.render_pass, renderer.line_pipeline);
//     auto resolution = glm::vec2{renderer.window_width, renderer.window_height};
//
//     auto normalized_screen_rect =
//         Rect{.position = (rect.position + rect.scale / 2.0f) / resolution * 2.0f - 1.0f, .scale = rect.scale / resolution};
//     normalized_screen_rect.position.y *= -1;
//
//     auto color_normalized = glm::vec4(color.r, color.g, color.b, color.a) / 255.0f;
//
//     SDL_PushGPUVertexUniformData(renderer.draw_command_buffer, 0, &normalized_screen_rect, sizeof(Rect));
//     SDL_PushGPUFragmentUniformData(renderer.draw_command_buffer, 0, &color_normalized, sizeof(glm::vec4));
//
//     SDL_DrawGPUPrimitives(renderer.render_pass, 5, 1, 0, 0);
// }

// void draw_textured_rect(
//     Renderer& renderer,
//     const std::optional<Rect>& src_rect,
//     const Rect& dst_rect,
//     const Texture& texture,
//     const RGBA& color_mod
// ) {
//     ZoneScoped;
//
//     if (renderer.null_swapchain) {
//         return;
//     }
//
//     SDL_BindGPUGraphicsPipeline(renderer.render_pass, renderer.textured_rect_pipeline);
//
//     auto texture_sampler_binding = SDL_GPUTextureSamplerBinding{.texture = texture.texture, .sampler = texture.sampler};
//     SDL_BindGPUFragmentSamplers(renderer.render_pass, 0, &texture_sampler_binding, 1);
//
//     auto resolution = glm::vec2{renderer.window_width, renderer.window_height};
//
//     auto normalized_screen_rect =
//         Rect{.position = (dst_rect.position + dst_rect.scale / 2.0f) / resolution * 2.0f - 1.0f, .scale = dst_rect.scale / resolution};
//     normalized_screen_rect.position.y *= -1;
//
//     Rect normalized_texture_rect;
//     if (src_rect.has_value()) {
//         normalized_texture_rect = {
//             .position = src_rect->position / glm::vec2{texture.w, texture.h},
//             .scale = src_rect->scale / glm::vec2{texture.w, texture.h},
//         };
//     } else {
//         normalized_texture_rect = Rect{{0, 0}, {1, 1}};
//     }
//
//     auto color_mod_normalized = glm::vec4{color_mod.r, color_mod.g, color_mod.b, color_mod.a} / 255.0f;
//
//     SDL_PushGPUVertexUniformData(renderer.draw_command_buffer, 0, &normalized_screen_rect, sizeof(Rect));
//     SDL_PushGPUVertexUniformData(renderer.draw_command_buffer, 1, &normalized_texture_rect, sizeof(Rect));
//     SDL_PushGPUFragmentUniformData(renderer.draw_command_buffer, 0, &color_mod_normalized, sizeof(glm::vec4));
//
//     SDL_DrawGPUPrimitives(renderer.render_pass, 4, 1, 0, 0);
// }

glm::vec2 world_to_normalized(Camera2D camera, glm::vec2 world_pos) {
    return ((world_pos - camera.position) / camera.scale * 2.0f);
}

Rect world_rect_to_normalized(Camera2D camera, Rect world_rect) {
    return Rect{
        (world_rect.position - camera.position) / camera.scale * 2.0f,
        world_rect.scale / camera.scale,
    };
}

// void draw_world_textured_rect(
//     Renderer& renderer,
//     const Camera2D& camera,
//     const std::optional<Rect>& src_rect,
//     const Rect& world_rect,
//     const Texture& texture,
//     const RGBA& color_mod
// ) {
//     ZoneScoped;
//
//     if (renderer.null_swapchain) {
//         return;
//     }
//
//     SDL_BindGPUGraphicsPipeline(renderer.render_pass, renderer.textured_rect_pipeline);
//
//     auto texture_sampler_binding = SDL_GPUTextureSamplerBinding{.texture = texture.texture, .sampler = texture.sampler};
//     SDL_BindGPUFragmentSamplers(renderer.render_pass, 0, &texture_sampler_binding, 1);
//
//     auto normalized_screen_rect = Rect{
//         (world_rect.position - camera.position) / camera.scale * 2.0f,
//         world_rect.scale / camera.scale,
//     };
//
//     Rect normalized_texture_rect;
//     if (src_rect.has_value()) {
//         normalized_texture_rect = {
//             .position = src_rect->position / glm::vec2{texture.w, texture.h},
//             .scale = src_rect->scale / glm::vec2{texture.w, texture.h},
//         };
//     } else {
//         normalized_texture_rect = Rect{{0, 0}, {1, 1}};
//     }
//
//     auto color_mod_normalized = glm::vec4{color_mod.r, color_mod.g, color_mod.b, color_mod.a} / 255.0f;
//
//     SDL_PushGPUVertexUniformData(renderer.draw_command_buffer, 0, &normalized_screen_rect, sizeof(Rect));
//     SDL_PushGPUVertexUniformData(renderer.draw_command_buffer, 1, &normalized_texture_rect, sizeof(Rect));
//     SDL_PushGPUFragmentUniformData(renderer.draw_command_buffer, 0, &color_mod_normalized, sizeof(glm::vec4));
//
//     SDL_DrawGPUPrimitives(renderer.render_pass, 4, 1, 0, 0);
// }

void begin_rendering(Renderer& renderer, SDL_Window* window) {
    renderer.draw_command_buffer = SDL_AcquireGPUCommandBuffer(renderer.device);

    SDL_GPUTexture* swapchain_texture;
    SDL_AcquireGPUSwapchainTexture(renderer.draw_command_buffer, window, &swapchain_texture, NULL, NULL);

    if (swapchain_texture == NULL) {
        renderer.null_swapchain = true;
        return;
    } else {
        renderer.null_swapchain = false;
    }

    SDL_GPUColorTargetInfo color_target_info = {0};
    color_target_info.texture = swapchain_texture;
    color_target_info.clear_color = (SDL_FColor){0.0f, 0.0f, 0.0f, 1.0f};
    color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target_info.store_op = SDL_GPU_STOREOP_STORE;

    renderer.render_pass = SDL_BeginGPURenderPass(renderer.draw_command_buffer, &color_target_info, 1, NULL);

    renderer.mesh_vertices.clear();
    renderer.mesh_indices.clear();

    renderer.line_vertices.clear();
    renderer.line_indices.clear();

    for (int i = 0; i < (int)TextureID::texture_count; i++) {
        renderer.texture_draws[i].verts.clear();
        renderer.texture_draws[i].indices.clear();
    }

    for (int i = 0; i < (int)TextureID::texture_count; i++) {
        renderer.sprite_draws[i].clear();
    }

}

void* pointer_offset_bytes(void* ptr, int bytes) {
    return (uint8_t*)ptr + bytes;
}

struct DrawResources {
    uint32_t vertex_data_size;
    uint32_t index_data_size;

    uint32_t vertex_buffer_offset;
    uint32_t index_buffer_offset;
};

struct SpriteBuffer {
    u32 size;
    u32 offset;
};

void copy_to_transfer(void** transfer_ptr, void* data, uint32_t size) {
    memcpy(*transfer_ptr, data, size);
    *transfer_ptr = (uint8_t*)*transfer_ptr + size;
};

void end_rendering(Renderer& renderer) {
    ZoneScoped;

    if (renderer.null_swapchain) {
        return;
    }

    void* transfer_ptr = SDL_MapGPUTransferBuffer(renderer.device, renderer.transfer_buffer, true);

    bool do_mesh_pipeline = renderer.mesh_vertices.size() > 0;
    bool do_line_pipeline = renderer.line_vertices.size() > 0;

    bool do_texture[(int)TextureID::texture_count]{};
    for (int i = 0; i < (int)TextureID::texture_count; i++) {
        if (renderer.texture_draws[i].verts.size() > 0) {
            do_texture[i] = true;
        }
    }

    bool do_sprite[(int)TextureID::texture_count]{};
    for (int i = 0; i < (int)TextureID::texture_count; i++) {
        if (renderer.sprite_draws[i].size() > 0) {
            do_sprite[i] = true;
        }
    }

    DrawResources mesh_pipeline{};
    DrawResources line_pipeline{};
    DrawResources texture_draws[(int)TextureID::texture_count]{};

    uint32_t vertex_buffer_offset = 0;
    uint32_t index_buffer_offset = 0;

    // map mesh data
    if (do_mesh_pipeline) {
        mesh_pipeline.vertex_buffer_offset = vertex_buffer_offset;
        mesh_pipeline.vertex_data_size = (uint32_t)(renderer.mesh_vertices.size() * sizeof(PositionColorVertex));
        vertex_buffer_offset += mesh_pipeline.vertex_data_size;

        mesh_pipeline.index_buffer_offset = index_buffer_offset;
        mesh_pipeline.index_data_size = (uint32_t)(renderer.mesh_indices.size() * sizeof(uint16_t));
        index_buffer_offset += mesh_pipeline.index_data_size;

        copy_to_transfer(&transfer_ptr, renderer.mesh_vertices.data(), mesh_pipeline.vertex_data_size);
        copy_to_transfer(&transfer_ptr, renderer.mesh_indices.data(), mesh_pipeline.index_data_size);
    }

    // map line data
    if (do_line_pipeline) {
        line_pipeline.vertex_buffer_offset = vertex_buffer_offset;
        line_pipeline.vertex_data_size = (uint32_t)(renderer.line_vertices.size() * sizeof(decltype(renderer.line_vertices)::value_type));
        vertex_buffer_offset += line_pipeline.vertex_data_size;

        line_pipeline.index_buffer_offset = index_buffer_offset;
        line_pipeline.index_data_size = (uint32_t)(renderer.line_indices.size() * sizeof(uint16_t));
        index_buffer_offset += line_pipeline.index_data_size;

        copy_to_transfer(&transfer_ptr, renderer.line_vertices.data(), line_pipeline.vertex_data_size);
        copy_to_transfer(&transfer_ptr, renderer.line_indices.data(), line_pipeline.index_data_size);
    }

    // map texture draw data
    for (int i = 0; i < (int)TextureID::texture_count; i++) {
        if (do_texture[i]) {
            auto& draw_info = texture_draws[i];
            auto& draw_data = renderer.texture_draws[i];
            draw_info.vertex_buffer_offset = vertex_buffer_offset;
            draw_info.vertex_data_size = draw_data.verts.size() * sizeof(decltype(draw_data.verts)::value_type);
            vertex_buffer_offset += draw_info.vertex_data_size;

            draw_info.index_buffer_offset = index_buffer_offset;
            draw_info.index_data_size = draw_data.indices.size() * sizeof(uint16_t);
            index_buffer_offset += draw_info.index_data_size;

            copy_to_transfer(&transfer_ptr, draw_data.verts.data(), draw_info.vertex_data_size);
            copy_to_transfer(&transfer_ptr, draw_data.indices.data(), draw_info.index_data_size);
        }
    }

    SpriteBuffer sprite_buffers[(int)TextureID::texture_count];
    for (int i = 0; i < (int)TextureID::texture_count; i++) {
        if (do_sprite[i]) {
            auto& draw_list = renderer.sprite_draws[i];
            auto& buffer = sprite_buffers[i];
            buffer.size = renderer.sprite_draws[i].size() * sizeof(SpriteDraw);
            buffer.offset = vertex_buffer_offset;

            vertex_buffer_offset += buffer.size;

            copy_to_transfer(&transfer_ptr, draw_list.data(), buffer.size);
        }
    }

    SDL_UnmapGPUTransferBuffer(renderer.device, renderer.transfer_buffer);

    auto upload_command_buffer = SDL_AcquireGPUCommandBuffer(renderer.device);
    auto copy_pass = SDL_BeginGPUCopyPass(upload_command_buffer);

    //upload to buffers
    {
        uint32_t transfer_buffer_offset = 0;
        // this shit currently relies on copy to transfer and upload to be in same order for transfer buffer offset to be correct
        // copy to transfer should return offset but more shit to keep track of
        auto upload_to_buffer =
            [&renderer, &transfer_buffer_offset, &copy_pass](SDL_GPUBuffer* buffer, uint32_t offset, uint32_t size, bool cycle) {
                auto transfer_buffer_location = SDL_GPUTransferBufferLocation{
                    .transfer_buffer = renderer.transfer_buffer,
                    .offset = transfer_buffer_offset,
                };
                transfer_buffer_offset += size;

                auto gpu_buffer_region = SDL_GPUBufferRegion{
                    .buffer = buffer,
                    .offset = offset,
                    .size = size,
                };

                SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &gpu_buffer_region, cycle);
            };


        bool cycle = true;
        if (do_mesh_pipeline) {
            upload_to_buffer(renderer.dynamic_vertex_buffer, mesh_pipeline.vertex_buffer_offset, mesh_pipeline.vertex_data_size, cycle);
            upload_to_buffer(renderer.dynamic_index_buffer, mesh_pipeline.index_buffer_offset, mesh_pipeline.index_data_size, cycle);

            cycle = false;
        }

        if (do_line_pipeline) {
            upload_to_buffer(renderer.dynamic_vertex_buffer, line_pipeline.vertex_buffer_offset, line_pipeline.vertex_data_size, cycle);
            upload_to_buffer(renderer.dynamic_index_buffer, line_pipeline.index_buffer_offset, line_pipeline.index_data_size, cycle);

            cycle = false;
        }

        for (int i = 0; i < (int)TextureID::texture_count; i++) {
            if (do_texture[i]) {
                const auto& draw_info = texture_draws[i];
                upload_to_buffer(renderer.dynamic_vertex_buffer, draw_info.vertex_buffer_offset, draw_info.vertex_data_size, cycle);
                upload_to_buffer(renderer.dynamic_index_buffer, draw_info.index_buffer_offset, draw_info.index_data_size, cycle);

                cycle = false;
            }
        }

        for (int i = 0; i < (int)TextureID::texture_count; i++) {
            if (do_sprite[i]) {
                auto& buffer = sprite_buffers[i];
                upload_to_buffer(renderer.dynamic_vertex_buffer, buffer.offset, buffer.size, cycle);
                cycle = false;
            }
        }
    }

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_command_buffer);

    // draw mesh pipeline
    if (do_mesh_pipeline) {
        SDL_BindGPUGraphicsPipeline(renderer.render_pass, renderer.solid_mesh_pipeline);

        auto buffer_binding = SDL_GPUBufferBinding{
            .buffer = renderer.dynamic_vertex_buffer,
            .offset = mesh_pipeline.vertex_buffer_offset,
        };
        SDL_BindGPUVertexBuffers(renderer.render_pass, 0, &buffer_binding, 1);

        buffer_binding = SDL_GPUBufferBinding{
            .buffer = renderer.dynamic_index_buffer,
            .offset = mesh_pipeline.index_buffer_offset,
        };

        auto index_element_size = SDL_GPU_INDEXELEMENTSIZE_16BIT;

        SDL_BindGPUIndexBuffer(renderer.render_pass, &buffer_binding, index_element_size);

        SDL_DrawGPUIndexedPrimitives(renderer.render_pass, renderer.mesh_indices.size(), 1, 0, 0, 0);
    }

    // draw line pipeline
    if (do_line_pipeline) {
        SDL_BindGPUGraphicsPipeline(renderer.render_pass, renderer.line_pipeline);

        auto buffer_binding = SDL_GPUBufferBinding{
            .buffer = renderer.dynamic_vertex_buffer,
            .offset = line_pipeline.vertex_buffer_offset,
        };
        SDL_BindGPUVertexBuffers(renderer.render_pass, 0, &buffer_binding, 1);

        buffer_binding = SDL_GPUBufferBinding{
            .buffer = renderer.dynamic_index_buffer,
            .offset = line_pipeline.index_buffer_offset,
        };

        auto index_element_size = SDL_GPU_INDEXELEMENTSIZE_16BIT;

        SDL_BindGPUIndexBuffer(renderer.render_pass, &buffer_binding, index_element_size);

        SDL_DrawGPUIndexedPrimitives(renderer.render_pass, renderer.line_indices.size(), 1, 0, 0, 0);
    }

    SDL_BindGPUGraphicsPipeline(renderer.render_pass, renderer.textured_mesh_pipeline);
    for (int i = 0; i < (int)TextureID::texture_count; i++) {
        if (!do_texture[i]) {
            continue;
        }

        auto& draw_info = texture_draws[i];
        auto& draw_data = renderer.texture_draws[i];

        auto buffer_binding = SDL_GPUBufferBinding{
            .buffer = renderer.dynamic_vertex_buffer,
            .offset = draw_info.vertex_buffer_offset,
        };
        SDL_BindGPUVertexBuffers(renderer.render_pass, 0, &buffer_binding, 1);

        buffer_binding = SDL_GPUBufferBinding{
            .buffer = renderer.dynamic_index_buffer,
            .offset = draw_info.index_buffer_offset,
        };


        auto index_element_size = SDL_GPU_INDEXELEMENTSIZE_16BIT;

        SDL_BindGPUIndexBuffer(renderer.render_pass, &buffer_binding, index_element_size);

        auto& texture = renderer.textures[i];
        auto sampler_binding = SDL_GPUTextureSamplerBinding{.texture=texture.texture, .sampler=texture.sampler};
        SDL_BindGPUFragmentSamplers(renderer.render_pass, 0, &sampler_binding, 1);

        SDL_DrawGPUIndexedPrimitives(renderer.render_pass, draw_data.indices.size(), 1, 0, 0, 0);
    }

    // maybe can bind more stuff once only
    SDL_BindGPUGraphicsPipeline(renderer.render_pass, renderer.test_pipeline);
    for (int i = 0; i < (int)TextureID::texture_count; i++) {
        if (!do_sprite[i]) {
            continue;
        }

        auto& instances = renderer.sprite_draws[i];

        SDL_GPUBufferBinding bindings[] = {
            SDL_GPUBufferBinding {
                .buffer = renderer.rect_vertex_buffer,
                .offset = 0,
            },
            SDL_GPUBufferBinding {
                .buffer = renderer.dynamic_vertex_buffer,
                .offset = sprite_buffers[i].offset,
            },
        };

        SDL_BindGPUVertexBuffers(renderer.render_pass, 0, bindings, 2);

        SDL_GPUBufferBinding binding = {
            .buffer = renderer.rect_index_buffer,
            .offset = 0,
        };
        SDL_BindGPUIndexBuffer(renderer.render_pass, &binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

        auto& texture = renderer.textures[i];
        auto sampler_binding = SDL_GPUTextureSamplerBinding{.texture=texture.texture, .sampler=texture.sampler};
        SDL_BindGPUFragmentSamplers(renderer.render_pass, 0, &sampler_binding, 1);


        SDL_DrawGPUIndexedPrimitives(renderer.render_pass, 6, instances.size(), 0, 0, 0);
    }

    SDL_EndGPURenderPass(renderer.render_pass);
    SDL_SubmitGPUCommandBuffer(renderer.draw_command_buffer);
}

// very retarded api
// renderer can store texture refs for known stuff
// other stuff user has to keep it
// texture id only for renderer managed texture
// renderer needs to know about textures for bucketing draw calls
Texture load_texture(Renderer& renderer, std::optional<TextureID> texture_id, const Image& image) {
    ZoneScoped;

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

    auto texture_ref = Texture{.texture = texture, .sampler = sampler, .w = image.w, .h = image.h};
    if (texture_id.has_value()) {
        renderer.textures[(int)texture_id.value()] = texture_ref;
    }
    return texture_ref;
}

void draw_lines(Renderer& renderer, glm::vec2* vertices, int vert_count, RGBA color) {
    int start_index = renderer.line_vertices.size();
    for (int i = start_index; i < start_index + vert_count - 1; i++) {
        renderer.line_indices.emplace_back(i);
        renderer.line_indices.emplace_back(i + 1);
    }

    for (int i = 0; i < vert_count; i++) {
        renderer.line_vertices.emplace_back(PositionColorVertex{
            .position = vertices[i],
            .color = color,
        });
    }
}

void draw_world_lines(Renderer& renderer, const Camera2D& camera, glm::vec2* vertices, int vert_count, RGBA color) {
    int start_index = renderer.line_vertices.size();
    for (int i = start_index; i < start_index + vert_count - 1; i++) {
        renderer.line_indices.emplace_back(i);
        renderer.line_indices.emplace_back(i + 1);
    }

    for (int i = 0; i < vert_count; i++) {
        renderer.line_vertices.emplace_back(PositionColorVertex{
            .position = world_to_normalized(camera, vertices[i]),
            .color = color,
        });
    }
}

// void draw_world_lines(Renderer& renderer, const Camera2D& camera, glm::vec2* vertices, int vert_count, RGBA color) {
//     int start_index = renderer.line_vertices.size();
//     for (int i = start_index; i < start_index + vert_count - 1; i++) {
//         renderer.line_indices.emplace_back(i);
//         renderer.line_indices.emplace_back(i + 1);
//     }
//
//     for (int i = 0; i < vert_count; i++) {
//         renderer.line_vertices.emplace_back(PositionColorVertex{
//             .position = world_to_normalized(camera, vertices[i]),
//             .color = color,
//         });
//     }
// }

void draw_lines_indexed(Renderer& renderer, glm::vec2* vertices, int vert_count, uint16_t* indices, int indices_count, RGBA color) {
    for (int i = 0; i < indices_count; i++) {
        indices[i] += renderer.line_vertices.size();
    }

    for (int i = 0; i < vert_count; i++) {
        renderer.line_vertices.emplace_back(PositionColorVertex{
            .position = vertices[i],
            .color = color,
        });
    }

    renderer.line_indices.insert(renderer.mesh_indices.end(), indices, indices + indices_count);
}

void draw_world_polygon(Renderer& renderer, const Camera2D& camera, glm::vec2* verts, int vert_count, RGBA color) {
    int start_index = renderer.mesh_vertices.size();
    for (int i = 1; i < vert_count - 1; i++) {
        renderer.mesh_indices.emplace_back(start_index + 0);
        renderer.mesh_indices.emplace_back(start_index + i);
        renderer.mesh_indices.emplace_back(start_index + i+1);
    }

    for (int i = 0; i < vert_count; i++) {
        renderer.mesh_vertices.emplace_back(PositionColorVertex{
            .position = world_to_normalized(camera, verts[i]),
            .color = color,
        });
    }
}

void draw_geometry(Renderer& renderer, glm::vec2* vertices, int vert_count, uint16_t* indices, int indices_count, RGBA color) {
    int start_index = renderer.mesh_vertices.size();
    for (int i = 0; i < indices_count; i++) {
        renderer.mesh_indices.emplace_back(start_index + indices[i]);
    }

    for (int i = 0; i < vert_count; i++) {
        renderer.mesh_vertices.emplace_back(PositionColorVertex{
            .position = vertices[i],
            .color = color,
        });
    }
}

void draw_rect(Renderer& renderer, Rect normalized_rect, RGBA rgba) {
    PositionColorVertex vertices[4] = {
        {{-1.0, -1.0}},
        {{1.0, -1.0}},
        {{-1.0, 1.0}},
        {{1.0, 1.0}},
    };

    uint16_t indices[6]{
        0,
        1,
        2,
        2,
        1,
        3,
    };

    for (int i = 0; i < 6; i++) {
        indices[i] += renderer.mesh_vertices.size();
    }

    auto resolution = glm::vec2{renderer.window_width, renderer.window_height};
    // normalized_screen_rect.position.y *= -1;

    for (int i = 0; i < 4; i++) {
        auto& vert = vertices[i].position;
        vert = vert * normalized_rect.scale + normalized_rect.position;
        vertices[i].color = rgba;
    }

    renderer.mesh_vertices.insert(renderer.mesh_vertices.end(), vertices, vertices + 4);
    renderer.mesh_indices.insert(renderer.mesh_indices.end(), indices, indices + 6);
}

Rect screen_rect_to_normalized(Rect& rect, glm::vec2 resolution) {
    auto normal_rect = Rect{.position = rect.position / resolution * 2.0f - 1.0f, .scale = rect.scale / resolution * 2.0f};
    normal_rect.position.y *= -1 + normal_rect.scale.y;
    return normal_rect;
}

void draw_world_rect(Renderer& renderer, Camera2D camera, Rect rect, RGBA rgba) {
    draw_rect(renderer, world_rect_to_normalized(camera, rect), rgba);
}

void draw_screen_rect(Renderer& renderer, Rect rect, RGBA rgba) {
    auto resolution = glm::vec2{renderer.window_width, renderer.window_height};
    draw_rect(renderer, screen_rect_to_normalized(rect, resolution), rgba);
}


void draw_world_textured_rect(Renderer& renderer, Camera2D camera, TextureID texture_id, std::optional<Rect> src_rect, Rect world_rect, RGBA rgba) {
    auto rect = renderer::world_rect_to_normalized(camera, world_rect);
    draw_textured_rect(renderer, texture_id, src_rect, rect, rgba);
}

void draw_textured_rect(Renderer& renderer, TextureID texture_id, std::optional<Rect> src_rect, Rect normalized_rect, RGBA rgba) {
    auto& texture = renderer.textures[(int)texture_id];

    auto normalized_texture_rect = Rect{{0,0}, {1,1}};
    if (src_rect.has_value()) {
        normalized_texture_rect = {
            .position = src_rect->position / glm::vec2{texture.w, texture.h},
            .scale = src_rect->scale / glm::vec2{texture.w, texture.h},
        };
    }

    PositionUvColorVertex vertices[4] = {
        {{-1.0, -1.0}, {0,1}},
        {{1.0, -1.0}, {1,1}},
        {{-1.0, 1.0}, {0,0}},
        {{1.0, 1.0}, {1,0}},
    };

    uint16_t indices[6]{
        0,
        1,
        2,
        2,
        1,
        3,
    };

    auto& draw_data = renderer.texture_draws[(int)texture_id];

    for (int i = 0; i < 6; i++) {
        indices[i] += draw_data.verts.size();
    }

    auto resolution = glm::vec2{renderer.window_width, renderer.window_height};
    // normalized_screen_rect.position.y *= -1;

    for (int i = 0; i < 4; i++) {
        auto& vert_pos = vertices[i].position;
        vert_pos = vert_pos * normalized_rect.scale + normalized_rect.position;

        auto& vert_uv = vertices[i].uv;
        vert_uv = vert_uv * normalized_texture_rect.scale + normalized_texture_rect.position;
        // vert_uv.y *= -1;
        vertices[i].color = rgba;
    }

    draw_data.verts.insert(draw_data.verts.end(), vertices, vertices + 4);
    draw_data.indices.insert(draw_data.indices.end(), indices, indices + 6);
}

// void render_geometry(Renderer& renderer, const Texture& texture, const PositionColorVertex* vertices, int num_vertices, const void*
// indices, int num_indices, int index_size) {
//     if (renderer.null_swapchain) {
//         return;
//     }
//
//     renderer.vertices_collect.insert(renderer.vertices_collect.end(), vertices, vertices + num_vertices);
//
//
//     memcpy(transfer_data, vertices, vertex_buffer_size);
//
//     auto index_data = (void*)&transfer_data[num_vertices];
//     memcpy(index_data, indices, index_buffer_size);
//
//
//
//
//     SDL_ReleaseGPUBuffer(renderer.device, vertex_buffer);
//     SDL_ReleaseGPUBuffer(renderer.device, index_buffer);
// }

void render_geometry_textured(
    Renderer& renderer,
    const Texture& texture,
    const PositionUvColorVertex* vertices,
    int num_vertices,
    const void* indices,
    int num_indices,
    int index_size
) {
    if (renderer.null_swapchain) {
        return;
    }

    auto vertex_buffer_size = (uint32_t)(sizeof(PositionUvColorVertex) * num_vertices);
    auto index_buffer_size = (uint32_t)(index_size * num_indices);

    auto vertex_buffer_create_info = SDL_GPUBufferCreateInfo{
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = vertex_buffer_size,
    };

    auto vertex_buffer = SDL_CreateGPUBuffer(renderer.device, &vertex_buffer_create_info);

    auto index_buffer_create_info = SDL_GPUBufferCreateInfo{
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = index_buffer_size,
    };

    auto index_buffer = SDL_CreateGPUBuffer(renderer.device, &index_buffer_create_info);

    auto transfer_buffer_create_info = SDL_GPUTransferBufferCreateInfo{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = vertex_buffer_size + index_buffer_size,
    };

    auto transfer_buffer = SDL_CreateGPUTransferBuffer(renderer.device, &transfer_buffer_create_info);

    auto transfer_data = (PositionUvColorVertex*)SDL_MapGPUTransferBuffer(renderer.device, transfer_buffer, false);

    memcpy(transfer_data, vertices, vertex_buffer_size);

    auto index_data = (void*)&transfer_data[num_vertices];
    memcpy(index_data, indices, index_buffer_size);

    SDL_UnmapGPUTransferBuffer(renderer.device, transfer_buffer);

    auto upload_command_buffer = SDL_AcquireGPUCommandBuffer(renderer.device);

    auto copy_pass = SDL_BeginGPUCopyPass(upload_command_buffer);

    {
        auto transfer_buffer_location = SDL_GPUTransferBufferLocation{
            .transfer_buffer = transfer_buffer,
            .offset = vertex_buffer_size,
        };

        auto gpu_buffer_region = SDL_GPUBufferRegion{
            .buffer = index_buffer,
            .offset = 0,
            .size = index_buffer_size,
        };

        SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &gpu_buffer_region, false);
    }

    {
        auto transfer_buffer_location = SDL_GPUTransferBufferLocation{
            .transfer_buffer = transfer_buffer,
            .offset = 0,
        };

        auto gpu_buffer_region = SDL_GPUBufferRegion{
            .buffer = vertex_buffer,
            .offset = 0,
            .size = vertex_buffer_size,
        };

        SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &gpu_buffer_region, false);
    }

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_command_buffer);
    SDL_ReleaseGPUTransferBuffer(renderer.device, transfer_buffer);

    SDL_BindGPUGraphicsPipeline(renderer.render_pass, renderer.textured_mesh_pipeline);

    auto texture_sampler_binding = SDL_GPUTextureSamplerBinding{.texture = texture.texture, .sampler = texture.sampler};
    SDL_BindGPUFragmentSamplers(renderer.render_pass, 0, &texture_sampler_binding, 1);

    auto buffer_binding = SDL_GPUBufferBinding{
        .buffer = vertex_buffer,
        .offset = 0,
    };
    SDL_BindGPUVertexBuffers(renderer.render_pass, 0, &buffer_binding, 1);

    buffer_binding = SDL_GPUBufferBinding{
        .buffer = index_buffer,
        .offset = 0,
    };

    auto index_element_size = SDL_GPU_INDEXELEMENTSIZE_16BIT;
    if (index_size == 4) {
        index_element_size = SDL_GPU_INDEXELEMENTSIZE_32BIT;
    }
    SDL_BindGPUIndexBuffer(renderer.render_pass, &buffer_binding, index_element_size);

    // SDL_DrawGPUPrimitives(renderer.render_pass, num_vertices, 1, 0, 0);
    SDL_DrawGPUIndexedPrimitives(renderer.render_pass, num_indices, 1, 0, 0, 0);

    SDL_ReleaseGPUBuffer(renderer.device, vertex_buffer);
    SDL_ReleaseGPUBuffer(renderer.device, index_buffer);
}

//
// SDL_RenderGeometryRaw(SDL_Renderer *renderer, SDL_Texture *texture, const float *xy, int xy_stride, const SDL_FColor *color, int
// color_stride, const float *uv, int uv_stride, int num_vertices, const void *indices, int num_indices, int size_indices) return
// SDL_RenderGeometryRaw(renderer, texture, xy, xy_stride, color3, sizeof(*color3), uv, uv_stride, num_vertices, indices, num_indices,
// size_indices);

} // namespace renderer
