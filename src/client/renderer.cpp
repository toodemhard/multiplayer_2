#include "../pch.h"

#include "renderer.h"
#include "assets.h"
#include "codegen/shaders.h"
#include "color.h"
#include "panic.h"
#include "common/types.h"

i32 round_to_i32(f32 value) {
    return (i32)(value + 0.5f);
}

i32 truncate_to_i32(f32 value) {
    return (i32)value;
}

glm::vec2 snap_pos(glm::vec2 pos) {
    return {truncate_to_i32(pos.x * 16.0f) / 16.0f, truncate_to_i32(pos.y * 16.0f) / 16.0f};
}

namespace renderer {

int init_renderer(Renderer* renderer, SDL_Window* window) {
    ZoneScoped;

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
        .blend_state = {
            .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
            .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .color_blend_op = SDL_GPU_BLENDOP_ADD,
            .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
            .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
            .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
            .enable_blend = true,
        }
    };
    auto depth_stencil_state = SDL_GPUDepthStencilState {
        .compare_op = SDL_GPU_COMPAREOP_LESS,
        .enable_depth_test = true,
        .enable_depth_write = true,
    };
    auto target_info = SDL_GPUGraphicsPipelineTargetInfo {
        .color_target_descriptions = &color_target_description,
        .num_color_targets = 1,
        .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
        .has_depth_stencil_target = false,
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
            .depth_stencil_state = depth_stencil_state,
            .target_info = target_info,
        };
        // create_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        renderer->line_pipeline = SDL_CreateGPUGraphicsPipeline(device, &create_info);

        SDL_ReleaseGPUShader(device, vertex_shader);
        SDL_ReleaseGPUShader(device, fragment_shader);
    }

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
            .depth_stencil_state = depth_stencil_state,
            .target_info = target_info,
        };

        // create_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        renderer->solid_mesh_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_create_info);
        if (renderer->solid_mesh_pipeline == NULL) {
            panic("{}", SDL_GetError());
        }

        SDL_ReleaseGPUShader(device, vertex_shader);
        SDL_ReleaseGPUShader(device, fragment_shader);
    }

    // reusable buffers
    {
        u32 index_buffer_size = megabytes(0.5);
        u32 vertex_buffer_size = megabytes(0.5);

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
        float x = 1.0f;
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
                .pitch = sizeof(SpriteVertex),
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
            .depth_stencil_state = depth_stencil_state,
            .target_info = target_info,
        };

        renderer->sprite_pipeline = SDL_CreateGPUGraphicsPipeline(renderer->device, &createinfo);
    }

    {
        SDL_GPUTextureCreateInfo create_info = {
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
            .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
            .width = (u32)renderer->window_width,
            .height = (u32)renderer->window_height,
            .layer_count_or_depth = 1,
            .num_levels = 1,
        };
        renderer->depth_texture = SDL_CreateGPUTexture(renderer->device, &create_info);
    }

    return 0;
}

void draw_world_sprite(Renderer* renderer, Camera2D camera, Rect world_rect, const SpriteProperties& properties) {
    camera.position = snap_pos(camera.position);
    world_rect.position = snap_pos(world_rect.position);

    auto normalized_rect = world_rect_to_normalized(camera, world_rect);

    auto normalized_texture_rect = Rect{{0,0}, {1,1}};
    if (properties.src_rect.has_value()) {
        auto& texture = renderer->textures[(int)properties.texture_id];
        auto& src_rect = properties.src_rect.value();
        normalized_texture_rect = {
            .position = src_rect.position / glm::vec2{texture.w, texture.h},
            .scale = src_rect.scale / glm::vec2{texture.w, texture.h},
        };
    }

    SpriteVertex sprite_vertex = {
        .position = normalized_rect.position,
        .size = normalized_rect.scale,
        .uv_position = normalized_texture_rect.position,
        .uv_size = normalized_texture_rect.scale,
        .mult_color = properties.mult_color,
        .mix_color = properties.mix_color,
        .t = properties.t,
    };

    bool add_new_draw = true;
    auto& draw_list = renderer->draw_list;
    if (draw_list.length > 0) {
        auto& last_draw = draw_list[draw_list.length - 1];
        if (last_draw.type == PipelineType::Sprite && last_draw.texture_id == properties.texture_id) {
            last_draw.vertices_size += sizeof(sprite_vertex);
            add_new_draw = false;
        } 
    }

    if (add_new_draw) {
        slice_push(&draw_list, DrawItem{
            .type = PipelineType::Sprite,
            .texture_id = properties.texture_id,
            .vertex_buffer_offset = renderer->vertex_data.length,
            .vertices_size = sizeof(sprite_vertex),
        });
    }


    slice_push_range(&renderer->vertex_data, (u8*)&sprite_vertex, sizeof(sprite_vertex));
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

glm::vec2 world_to_normalized(Camera2D camera, glm::vec2 world_pos) {
    return ((world_pos - camera.position) / camera.scale * 2.0f);
}

Rect world_rect_to_normalized(Camera2D camera, Rect world_rect) {
    return Rect{
        (world_rect.position - camera.position) / camera.scale * 2.0f,
        world_rect.scale / camera.scale,
    };
}

void begin_rendering(Renderer* renderer, SDL_Window* window, Arena* temp_arena) {

    slice_init(&renderer->draw_list, temp_arena, 100);
    slice_init(&renderer->vertex_data, temp_arena, megabytes(0.5));
    slice_init(&renderer->index_data, temp_arena, megabytes(0.5));

    renderer->draw_command_buffer = SDL_AcquireGPUCommandBuffer(renderer->device);

    SDL_GPUTexture* swapchain_texture;
    SDL_AcquireGPUSwapchainTexture(renderer->draw_command_buffer, window, &swapchain_texture, NULL, NULL);

    if (swapchain_texture == NULL) {
        renderer->null_swapchain = true;
        return;
    } else {
        renderer->null_swapchain = false;
    }

    SDL_GPUColorTargetInfo color_target_info = {0};
    color_target_info.texture = swapchain_texture;
    color_target_info.clear_color = SDL_FColor{0.0f, 0.0f, 0.0f, 1.0f};
    color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target_info.store_op = SDL_GPU_STOREOP_STORE;

    renderer->render_pass = SDL_BeginGPURenderPass(renderer->draw_command_buffer, &color_target_info, 1, NULL);
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

void push_to_transfer_buffer(void** transfer_ptr, void* data, u32 size) {
    memcpy(*transfer_ptr, data, size);
    *transfer_ptr = (uint8_t*)*transfer_ptr + size;
};

void end_rendering(Renderer* renderer) {
    ZoneScoped;

    if (renderer->null_swapchain) {
        return;
    }

    void* transfer_ptr = SDL_MapGPUTransferBuffer(renderer->device, renderer->transfer_buffer, true);

    uint32_t vertex_buffer_offset = 0;
    push_to_transfer_buffer(&transfer_ptr, renderer->vertex_data.data, renderer->vertex_data.length);
    uint32_t index_buffer_offset = renderer->vertex_data.length;
    push_to_transfer_buffer(&transfer_ptr, renderer->index_data.data, renderer->index_data.length);


    SDL_UnmapGPUTransferBuffer(renderer->device, renderer->transfer_buffer);

    auto upload_command_buffer = SDL_AcquireGPUCommandBuffer(renderer->device);
    auto copy_pass = SDL_BeginGPUCopyPass(upload_command_buffer);

    //upload to buffers
    if (renderer->vertex_data.length > 0) {
        auto transfer_buffer_location = SDL_GPUTransferBufferLocation{
            .transfer_buffer = renderer->transfer_buffer,
            .offset = vertex_buffer_offset,
        };

        auto gpu_buffer_region = SDL_GPUBufferRegion{
            .buffer = renderer->dynamic_vertex_buffer,
            .offset = 0,
            .size = renderer->vertex_data.length,
        };
        SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &gpu_buffer_region, true);
    }
    if (renderer->index_data.length > 0 ) {
        auto transfer_buffer_location = SDL_GPUTransferBufferLocation{
            .transfer_buffer = renderer->transfer_buffer,
            .offset = index_buffer_offset,
        };
        auto gpu_buffer_region = SDL_GPUBufferRegion{
            .buffer = renderer->dynamic_index_buffer,
            .offset = 0,
            .size = renderer->index_data.length,
        };
        SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &gpu_buffer_region, true);
    }

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_command_buffer);

    auto& draw_list = renderer->draw_list;
    for (i32 i = 0; i < draw_list.length; i++) {
        DrawItem& draw_item = draw_list[i];
        
        switch (draw_item.type) {
        case PipelineType::Sprite: {
            SDL_BindGPUGraphicsPipeline(renderer->render_pass, renderer->sprite_pipeline);
            SDL_GPUBufferBinding bindings[] = {
                SDL_GPUBufferBinding {
                    .buffer = renderer->rect_vertex_buffer,
                    .offset = 0,
                },
                SDL_GPUBufferBinding {
                    .buffer = renderer->dynamic_vertex_buffer,
                    .offset = draw_item.vertex_buffer_offset,
                },
            };
            SDL_BindGPUVertexBuffers(renderer->render_pass, 0, bindings, 2);

            SDL_GPUBufferBinding binding = {
                .buffer = renderer->rect_index_buffer,
                .offset = 0,
            };
            SDL_BindGPUIndexBuffer(renderer->render_pass, &binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

            auto& texture = renderer->textures[(int)draw_item.texture_id];
            auto sampler_binding = SDL_GPUTextureSamplerBinding{.texture=texture.texture, .sampler=texture.sampler};
            SDL_BindGPUFragmentSamplers(renderer->render_pass, 0, &sampler_binding, 1);

            SDL_DrawGPUIndexedPrimitives(renderer->render_pass, 6, draw_item.vertices_size / sizeof(SpriteVertex), 0, 0, 0);

        } break;
        case PipelineType::Line: {
            SDL_BindGPUGraphicsPipeline(renderer->render_pass, renderer->line_pipeline);

            auto buffer_binding = SDL_GPUBufferBinding{
                .buffer = renderer->dynamic_vertex_buffer,
                .offset = draw_item.vertex_buffer_offset,
            };
            SDL_BindGPUVertexBuffers(renderer->render_pass, 0, &buffer_binding, 1);

            buffer_binding = SDL_GPUBufferBinding{
                .buffer = renderer->dynamic_index_buffer,
                .offset = draw_item.index_buffer_offset,
            };

            auto index_element_size = SDL_GPU_INDEXELEMENTSIZE_16BIT;

            SDL_BindGPUIndexBuffer(renderer->render_pass, &buffer_binding, index_element_size);

            SDL_DrawGPUIndexedPrimitives(renderer->render_pass, draw_item.indices_size / sizeof(u16), 1, 0, 0, 0);

        } break;
        case PipelineType::Mesh: {
            SDL_BindGPUGraphicsPipeline(renderer->render_pass, renderer->solid_mesh_pipeline);

            auto buffer_binding = SDL_GPUBufferBinding{
                .buffer = renderer->dynamic_vertex_buffer,
                .offset = draw_item.vertex_buffer_offset,
            };
            SDL_BindGPUVertexBuffers(renderer->render_pass, 0, &buffer_binding, 1);

            buffer_binding = SDL_GPUBufferBinding{
                .buffer = renderer->dynamic_index_buffer,
                .offset = draw_item.index_buffer_offset,
            };

            auto index_element_size = SDL_GPU_INDEXELEMENTSIZE_16BIT;

            SDL_BindGPUIndexBuffer(renderer->render_pass, &buffer_binding, index_element_size);
            SDL_DrawGPUIndexedPrimitives(renderer->render_pass, draw_item.indices_size / sizeof(u16), 1, 0, 0, 0);

        } break;

        }

    }

    SDL_EndGPURenderPass(renderer->render_pass);
    SDL_SubmitGPUCommandBuffer(renderer->draw_command_buffer);
}

// very retarded api
// renderer can store texture refs for known stuff
// other stuff user has to keep it
// texture id only for renderer managed texture
// renderer needs to know about textures for bucketing draw calls
Texture load_texture(Renderer* renderer, std::optional<TextureID> texture_id, const Image& image) {
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

    auto texture = SDL_CreateGPUTexture(renderer->device, &texture_create_info);

    auto sampler_create_info = SDL_GPUSamplerCreateInfo{
        .min_filter = SDL_GPU_FILTER_NEAREST,
        .mag_filter = SDL_GPU_FILTER_NEAREST,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    };

    auto sampler = SDL_CreateGPUSampler(renderer->device, &sampler_create_info);

    auto upload_command_buffer = SDL_AcquireGPUCommandBuffer(renderer->device);
    auto copy_pass = SDL_BeginGPUCopyPass(upload_command_buffer);

    auto transfer_create_info = SDL_GPUTransferBufferCreateInfo{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = (uint32_t)(image.w * image.h) * comp,
    };

    auto texture_transfer_buffer = SDL_CreateGPUTransferBuffer(renderer->device, &transfer_create_info);

    auto texture_transfer_ptr = SDL_MapGPUTransferBuffer(renderer->device, texture_transfer_buffer, false);
    memcpy(texture_transfer_ptr, image.data, image.w * image.h * comp);

    SDL_UnmapGPUTransferBuffer(renderer->device, texture_transfer_buffer);

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
    SDL_ReleaseGPUTransferBuffer(renderer->device, texture_transfer_buffer);

    auto texture_ref = Texture{.texture = texture, .sampler = sampler, .w = image.w, .h = image.h};
    if (texture_id.has_value()) {
        renderer->textures[(int)texture_id.value()] = texture_ref;
    }
    return texture_ref;
}

// void draw_lines(Renderer& renderer, glm::vec2* vertices, int vert_count, RGBA color) {
//     int start_index = renderer.line_vertices.size();
//     for (int i = start_index; i < start_index + vert_count - 1; i++) {
//         renderer.line_indices.emplace_back(i);
//         renderer.line_indices.emplace_back(i + 1);
//     }
//
//     for (int i = 0; i < vert_count; i++) {
//         renderer.line_vertices.emplace_back(PositionColorVertex{
//             .position = vertices[i],
//             .color = color,
//         });
//     }
// }

void draw_world_lines(Renderer* renderer, Camera2D camera, glm::vec2* vertices, int vert_count, RGBA color) {
    camera.position = snap_pos(camera.position);

    int start_index = 0;
    u32 verts_size = sizeof(PositionColorVertex) * vert_count;
    u32 indices_size = sizeof(u16) * 2 * (vert_count - 1);

    bool add_new_draw = true;
    auto& draw_list = renderer->draw_list;
    if (draw_list.length > 0) {
        auto& last_draw = draw_list[draw_list.length - 1];
        if (last_draw.type == PipelineType::Line) {
            start_index = last_draw.vertices_size / sizeof(PositionColorVertex);

            last_draw.vertices_size += verts_size;
            last_draw.indices_size += indices_size;

            add_new_draw = false;
        } 
    }

    if (add_new_draw) {
        slice_push(&draw_list, DrawItem{
            .type = PipelineType::Line,
            .vertex_buffer_offset = renderer->vertex_data.length,
            .vertices_size = verts_size,
            .index_buffer_offset = renderer->index_data.length,
            .indices_size = indices_size,
        });
    }

    for (int i = 0; i < vert_count; i++) {
        auto vertex = PositionColorVertex{
                .position = world_to_normalized(camera, snap_pos(vertices[i])),
                .color = color,
        };

        slice_push_range(&renderer->vertex_data, (u8*)&vertex, sizeof(vertex));
    }

    for (int i = start_index; i < start_index + vert_count - 1; i++) {
        u16 indices[] = {(u16)i, (u16)(i+1)};
        slice_push_range(&renderer->index_data, (u8*)indices, sizeof(indices));
    }
}

void draw_world_polygon(Renderer* renderer, const Camera2D& camera, glm::vec2* verts, int vert_count, RGBA color) {
    ASSERT(vert_count >= 3)

    bool push_new_draw = true;
    u32 verts_size = sizeof(PositionColorVertex) * vert_count;
    u32 indxs_size = sizeof(u16) * (vert_count - 2) * 3;
    u16 start_index = 0;
    auto& draw_list = renderer->draw_list;
    if (draw_list.length > 0) {
        auto& last_draw = draw_list[draw_list.length - 1];

        if (last_draw.type == PipelineType::Mesh) {
            push_new_draw = false;
            start_index = last_draw.vertices_size / sizeof(PositionColorVertex); //GET PREVIOUS VERTEX SIZE BEFORE APPEND SIZE PLS

            last_draw.vertices_size += verts_size;
            last_draw.indices_size += indxs_size;
        }
    }

    if (push_new_draw) {
        slice_push(&draw_list, DrawItem{
            .type = PipelineType::Mesh,
            .vertex_buffer_offset = renderer->vertex_data.length,
            .vertices_size = verts_size,
            .index_buffer_offset = renderer->index_data.length,
            .indices_size = indxs_size,
        });
    }

    for (u16 i = 1; i < vert_count - 1; i++) {
        u16 indices[] = {(u16)(start_index + 0), (u16)(start_index + i), (u16)(start_index + i + 1)};
        slice_push_range(&renderer->index_data, (u8*)indices, sizeof(indices));
    }

    for (int i = 0; i < vert_count; i++) {
        PositionColorVertex vert = {
            .position = world_to_normalized(camera, verts[i]),
            .color = color,
        };

        slice_push_range(&renderer->vertex_data, (u8*)&vert, sizeof(vert));
    }
}

void draw_rect(Renderer* renderer, Rect normalized_rect, RGBA rgba) {
    PositionColorVertex vertices[4] = {
        {{-1.0, -1.0}},
        {{1.0, -1.0}},
        {{-1.0, 1.0}},
        {{1.0, 1.0}},
    };

    u16 indices[6]{
        0,
        1,
        2,
        2,
        1,
        3,
    };


    bool push_new_draw = true;
    u16 start_index = 0;
    auto& draw_list = renderer->draw_list;
    if (draw_list.length > 0) {
        auto& last_draw = draw_list[draw_list.length - 1];

        if (last_draw.type == PipelineType::Mesh) {
            push_new_draw = false;
            start_index = last_draw.vertices_size / sizeof(PositionColorVertex); //GET PREVIOUS VERTEX SIZE BEFORE APPEND SIZE PLS

            last_draw.vertices_size += sizeof(vertices);
            last_draw.indices_size += sizeof(indices);
        }
    }

    if (push_new_draw) {
        slice_push(&draw_list, DrawItem{
            .type = PipelineType::Mesh,
            .vertex_buffer_offset = renderer->vertex_data.length,
            .vertices_size = sizeof(vertices),
            .index_buffer_offset = renderer->index_data.length,
            .indices_size = sizeof(indices),
        });
    }

    for (int i = 0; i < 6; i++) {
        indices[i] += start_index;
    }

    for (int i = 0; i < 4; i++) {
        auto& vert = vertices[i].position;
        vert = vert * normalized_rect.scale + normalized_rect.position;
        vertices[i].color = rgba;
    }

    slice_push_range(&renderer->vertex_data, (u8*)vertices, sizeof(vertices));
    slice_push_range(&renderer->index_data, (u8*)indices, sizeof(indices));
}

Rect screen_rect_to_normalized(Rect rect, glm::vec2 resolution) {
    auto normal_rect = Rect{.position = rect.position / resolution * 2.0f - 1.0f, .scale = rect.scale / resolution * 2.0f};
    normal_rect.position.y *= -1 + normal_rect.scale.y;
    return normal_rect;
}

void draw_world_rect(Renderer* renderer, Camera2D camera, Rect rect, RGBA rgba) {
    draw_rect(renderer, world_rect_to_normalized(camera, rect), rgba);
}

void draw_screen_rect(Renderer* renderer, Rect rect, RGBA rgba) {
    auto resolution = glm::vec2{renderer->window_width, renderer->window_height};
    draw_rect(renderer, screen_rect_to_normalized(rect, resolution), rgba);
}

} // namespace renderer
