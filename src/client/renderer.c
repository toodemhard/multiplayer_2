global Renderer* renderer;

float2 texture_dimensions(TextureID texture_id) {
    Texture* tex = &renderer->textures[texture_id];
    return (float2){(f32)tex->w, (f32)tex->h};
}

i32 round_to_i32(f32 value) {
    return (i32)(value + 0.5f);
}

i32 truncate_to_i32(f32 value) {
    return (i32)value;
}

float2 snap_pos(float2 pos) {
    return (float2){truncate_to_i32(pos.x * 16.0f) / 16.0f, truncate_to_i32(pos.y * 16.0f) / 16.0f};
}

void renderer_set_ctx(Renderer* _renderer) {
    renderer = _renderer;
}

Rect render_target_rect(i32 window_width, i32 window_height, i32 render_width, i32 render_height) {
    Axis2 fit_axis = Axis2_X;
    if (window_width / (f32)window_height > render_width / (f32)render_height) {
        fit_axis = Axis2_Y;
    }

    float2 window_size = {window_width, window_height};
    float2 render_size = {render_width, render_height};

    Rect rect = {0};
    for (Axis2 i = 0; i < Axis2_Count; i++) {
        if (i == fit_axis) {
            rect.size.v[i] = window_size.v[i];
        } else {
            i32 other_axis = (i+1) % Axis2_Count;
            rect.size.v[i] = render_size.v[i] * (window_size.v[other_axis] / (f32)render_size.v[other_axis]);
            rect.position.v[i] = (window_size.v[i] - rect.size.v[i]) / 2;
        }
    }

    return rect;
}

const global int window_width = 1024;
const global int window_height = 768;

int renderer_init(Renderer* renderer, SDL_Window* window) {
    renderer->window_width = window_width;
    renderer->window_height = window_height;
    renderer->res_width = window_width;
    renderer->res_height = window_height;
    SDL_GPUDevice* device;
    {
        device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
        if (device == NULL) {
            SDL_Log("CreateGPUDevice failed: %s", SDL_GetError());
            return 1;
        }
        renderer->device = device;
    }

    {
        if (!SDL_ClaimWindowForGPUDevice(device, window)) {
            SDL_Log("ClaimWindow failed");
            return 1;
        }
    }

    renderer->render_target = SDL_CreateGPUTexture(renderer->device, (SDL_GPUTextureCreateInfo[]){{
        .format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = renderer->res_width,
        .height = renderer->res_height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    }});



    SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_IMMEDIATE);

    SDL_GPUColorTargetDescription color_target_description = {
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
    SDL_GPUDepthStencilState depth_stencil_state = {
        .compare_op = SDL_GPU_COMPAREOP_LESS,
        .enable_depth_test = true,
        .enable_depth_write = true,
    };
    SDL_GPUGraphicsPipelineTargetInfo target_info = {
        .color_target_descriptions = &color_target_description,
        .num_color_targets = 1,
        .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
        .has_depth_stencil_target = false,
    };

    // lines pipeline
    {
        SDL_GPUShader* vertex_shader = load_shader(device, ShaderID_raw_vertex_buffer_vert_spv, 0, 0, 0, 0);
        SDL_GPUShader* fragment_shader = load_shader(device, ShaderID_solid_color_frag_spv, 0, 0, 0, 0);

        SDL_GPUVertexBufferDescription descriptions[] = {{
            .slot = 0,
            .pitch = sizeof(PositionColorVertex),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            .instance_step_rate = 0,
        }};

        SDL_GPUVertexAttribute attributes[] = {
            {
                .location = 0,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                .offset = 0,
            },
            {
                .location = 1,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                .offset = sizeof(float2),
            },
        };

        SDL_GPUGraphicsPipelineCreateInfo create_info = {
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .vertex_input_state =
                {
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
        SDL_GPUShader* vertex_shader = load_shader(device, ShaderID_raw_vertex_buffer_vert_spv, 0, 0, 0, 0);
        SDL_GPUShader* fragment_shader = load_shader(device, ShaderID_solid_color_frag_spv, 0, 0, 0, 0);

        SDL_GPUVertexBufferDescription descriptions[] = {{
            .slot = 0,
            .pitch = sizeof(PositionColorVertex),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            .instance_step_rate = 0,
        }};

        SDL_GPUVertexAttribute attributes[] = {
            {
                .location = 0,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                .offset = 0,
            },
            {
                .location = 1,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                .offset = sizeof(float2),
            },
        };

        SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info = {
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .vertex_input_state =
                {
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

        ASSERT(renderer->solid_mesh_pipeline != NULL);

        SDL_ReleaseGPUShader(device, vertex_shader);
        SDL_ReleaseGPUShader(device, fragment_shader);
    }

    // reusable buffers
    {
        u32 index_buffer_size = megabytes(0.5);
        u32 vertex_buffer_size = megabytes(0.5);

        SDL_GPUBufferCreateInfo vertex_buffer_create_info = {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = vertex_buffer_size,
        };

        renderer->dynamic_vertex_buffer = SDL_CreateGPUBuffer(renderer->device, &vertex_buffer_create_info);

        SDL_GPUBufferCreateInfo index_buffer_create_info = {
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = index_buffer_size,
        };

        renderer->dynamic_index_buffer = SDL_CreateGPUBuffer(renderer->device, &index_buffer_create_info);

        SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info = {
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
            SDL_GPUBufferCreateInfo createinfo = {
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

        void* map_pointer = SDL_MapGPUTransferBuffer(renderer->device, renderer->transfer_buffer, false);

        memcpy(map_pointer, verts, verts_size);
        memcpy((u8*)map_pointer + verts_size, indices, indices_size);

        SDL_UnmapGPUTransferBuffer(renderer->device, renderer->transfer_buffer);

        SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(renderer->device);
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);

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
        SDL_GPUShader* vertex_shader = load_shader(device, ShaderID_sprite_vert_spv, 0, 0, 0, 0);
        SDL_GPUShader* fragment_shader = load_shader(device, ShaderID_sprite_frag_spv, 1, 0, 0, 0);

        SDL_GPUVertexBufferDescription descriptions[] = {
            {
                .slot = 0,
                .pitch = sizeof(RectVertex),
                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            },
            {
                .slot = 1,
                .pitch = sizeof(SpriteVertex),
                .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
                .instance_step_rate = 1,
            },
        };

        SDL_GPUVertexAttribute attributes[] = {
            {
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                .offset = 0,
            },
            {
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                .offset = sizeof(float2),
            },
            {
                .buffer_slot = 1,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            },
            {
                .buffer_slot = 1,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            },
            {
                .buffer_slot = 1,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            },
            {
                .buffer_slot = 1,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            },
            {
                .buffer_slot = 1,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
            },
            {
                .buffer_slot = 1,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
            },
            {
                .buffer_slot = 1,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
            },
        };

        for (int i = 0; i < 9; i++) {
            attributes[i].location = i;
        };

        int sizes[] = { 8, 8, 8, 8, 16, 4, 4 };
        int offset = 0;
        for (int i = 0; i < 7; i++) {
            attributes[i + 2].offset = offset;
            offset += sizes[i];
        }

        SDL_GPUGraphicsPipelineCreateInfo createinfo = {
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .vertex_input_state =
                {
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

void draw_sprite(Rect normalized_rect, SpriteProperties properties);

void draw_sprite_world(Camera2D camera, Rect world_rect, SpriteProperties properties) {
    ASSERT(properties.texture_id != TextureID_NULL);
    camera.position = snap_pos(camera.position);
    world_rect.position = snap_pos(world_rect.position);

    Rect normalized_rect = world_rect_to_normalized(camera, world_rect);
    draw_sprite(normalized_rect, properties);
}

void draw_sprite_screen(Rect screen_rect, SpriteProperties properties) {
    float2 resolution = {(f32)renderer->window_width, (f32)renderer->window_height};
    Rect norm_rect = screen_rect_to_normalized(screen_rect, resolution);

    draw_sprite(norm_rect, properties);
}


void draw_sprite(Rect normalized_rect, SpriteProperties properties) {

    Rect normalized_texture_rect = {(float2){0,0}, {1,1}};
    Rect src_rect = properties.src_rect;
    if (!(src_rect.w == 0 && src_rect.h == 0)) {
        Texture texture = renderer->textures[(int)properties.texture_id];
        normalized_texture_rect = (Rect) {
            .position = float2_div(src_rect.position, (float2){(f32)texture.w, (f32)texture.h}),
            .size = float2_div(src_rect.size, (float2){(f32)texture.w, (f32)texture.h}),
        };
    }

    if (!properties.mult_color.has_value) {
        opt_set(&properties.mult_color, (float4){1,1,1,1});
    }

    SpriteVertex sprite_vertex = {
        .position = normalized_rect.position,
        .size = normalized_rect.size,
        .uv_position = normalized_texture_rect.position,
        .uv_size = normalized_texture_rect.size,
        .mult_color = properties.mult_color.value,
        .mix_color = properties.mix_color,
        .t = properties.t,
    };

    bool add_new_draw = true;
    Slice_DrawItem* draw_list = &renderer->draw_list;
    if (draw_list->length > 0) {
        DrawItem* last_draw = slice_getp(*draw_list, draw_list->length - 1);
        if (last_draw->type == PipelineType_Sprite && last_draw->texture_id == properties.texture_id) {
            last_draw->vertices_size += sizeof(sprite_vertex);
            add_new_draw = false;
        } 
    }

    if (add_new_draw) {
        slice_push(draw_list, (DrawItem){
            .type = PipelineType_Sprite,
            .texture_id = properties.texture_id,
            .vertex_buffer_offset = (u32)renderer->vertex_data.length,
            .vertices_size = sizeof(sprite_vertex),
        });
    }

    slice_push_buffer(&renderer->vertex_data, (u8*)&sprite_vertex, sizeof(sprite_vertex));
}

SDL_GPUShader* load_shader(
    SDL_GPUDevice* device,
    ShaderID shader_id,
    Uint32 samplerCount,
    Uint32 uniformBufferCount,
    Uint32 storageBufferCount,
    Uint32 storageTextureCount
) {
    const char* shaderPath = shader_paths[shader_id];

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
    const u8* code = SDL_LoadFile(shaderPath, &codeSize);
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

float2 world_to_normalized(Camera2D camera, float2 world_pos) {
    float2 res = float2_sub(world_pos, camera.position);
    res = float2_div(res, camera.size);
    res =  float2_scale(res, 2.0);
    return res;
}

Rect world_rect_to_normalized(Camera2D camera, Rect world_rect) {
    return (Rect){
        float2_scale(float2_div(float2_sub(world_rect.position, camera.position), camera.size), 2.0f),
        float2_div(world_rect.size, camera.size),
    };
}

void begin_rendering(SDL_Window* window, Arena* temp_arena) {
    slice_init(&renderer->draw_list, temp_arena, 512);
    slice_init(&renderer->vertex_data, temp_arena, megabytes(0.5));
    slice_init(&renderer->index_data, temp_arena, megabytes(0.5));

    renderer->draw_command_buffer = SDL_AcquireGPUCommandBuffer(renderer->device);


    SDL_AcquireGPUSwapchainTexture(renderer->draw_command_buffer, window, &renderer->swapchain_texture, &renderer->swapchain_w, &renderer->swapchain_h);

    if (renderer->swapchain_texture == NULL) {
        renderer->null_swapchain = true;
        return;
    } else {
        renderer->null_swapchain = false;
    }

    SDL_GPUColorTargetInfo color_target_info = {0};
    color_target_info.texture = renderer->render_target;
    color_target_info.clear_color = (SDL_FColor){0.0f, 0.0f, 0.0f, 1.0f};
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

void end_rendering() {

    if (renderer->null_swapchain) {
        return;
    }

    // SDL_SetGPUViewport(renderer->render_pass, (SDL_GPUViewport[]){{0,0,window_width*1.5,window_height*1.5}});

    void* transfer_ptr = SDL_MapGPUTransferBuffer(renderer->device, renderer->transfer_buffer, true);

    uint32_t vertex_buffer_offset = 0;
    push_to_transfer_buffer(&transfer_ptr, renderer->vertex_data.data, renderer->vertex_data.length);
    uint32_t index_buffer_offset = renderer->vertex_data.length;
    push_to_transfer_buffer(&transfer_ptr, renderer->index_data.data, renderer->index_data.length);


    SDL_UnmapGPUTransferBuffer(renderer->device, renderer->transfer_buffer);

    SDL_GPUCommandBuffer* upload_command_buffer = SDL_AcquireGPUCommandBuffer(renderer->device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_command_buffer);

    //upload to buffers
    if (renderer->vertex_data.length > 0) {
        SDL_GPUTransferBufferLocation transfer_buffer_location = {
            .transfer_buffer = renderer->transfer_buffer,
            .offset = vertex_buffer_offset,
        };

        SDL_GPUBufferRegion gpu_buffer_region = {
            .buffer = renderer->dynamic_vertex_buffer,
            .offset = 0,
            .size = (u32)renderer->vertex_data.length,
        };
        SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &gpu_buffer_region, true);
    }
    if (renderer->index_data.length > 0 ) {
        SDL_GPUTransferBufferLocation transfer_buffer_location = {
            .transfer_buffer = renderer->transfer_buffer,
            .offset = index_buffer_offset,
        };
        SDL_GPUBufferRegion gpu_buffer_region = {
            .buffer = renderer->dynamic_index_buffer,
            .offset = 0,
            .size = (u32)renderer->index_data.length,
        };
        SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &gpu_buffer_region, true);
    }

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_command_buffer);

    Slice_DrawItem* draw_list = &renderer->draw_list;
    for (i32 i = 0; i < draw_list->length; i++) {
        DrawItem* draw_item = slice_getp(*draw_list, i);
        
        switch (draw_item->type) {
        case PipelineType_Sprite: {
            SDL_BindGPUGraphicsPipeline(renderer->render_pass, renderer->sprite_pipeline);
            SDL_GPUBufferBinding bindings[] = {
                {
                    .buffer = renderer->rect_vertex_buffer,
                    .offset = 0,
                },
                {
                    .buffer = renderer->dynamic_vertex_buffer,
                    .offset = draw_item->vertex_buffer_offset,
                },
            };
            SDL_BindGPUVertexBuffers(renderer->render_pass, 0, bindings, 2);

            SDL_GPUBufferBinding binding = {
                .buffer = renderer->rect_index_buffer,
                .offset = 0,
            };
            SDL_BindGPUIndexBuffer(renderer->render_pass, &binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

            Texture* texture = &renderer->textures[draw_item->texture_id];
            SDL_GPUTextureSamplerBinding sampler_binding = {.texture=texture->texture, .sampler=texture->sampler};
            SDL_BindGPUFragmentSamplers(renderer->render_pass, 0, &sampler_binding, 1);

            SDL_DrawGPUIndexedPrimitives(renderer->render_pass, 6, draw_item->vertices_size / sizeof(SpriteVertex), 0, 0, 0);

        } break;
        case PipelineType_Line: {
            SDL_BindGPUGraphicsPipeline(renderer->render_pass, renderer->line_pipeline);

            SDL_GPUBufferBinding buffer_binding = {
                .buffer = renderer->dynamic_vertex_buffer,
                .offset = draw_item->vertex_buffer_offset,
            };
            SDL_BindGPUVertexBuffers(renderer->render_pass, 0, &buffer_binding, 1);

            buffer_binding = (SDL_GPUBufferBinding){
                .buffer = renderer->dynamic_index_buffer,
                .offset = draw_item->index_buffer_offset,
            };

            SDL_GPUIndexElementSize index_element_size = SDL_GPU_INDEXELEMENTSIZE_16BIT;

            SDL_BindGPUIndexBuffer(renderer->render_pass, &buffer_binding, index_element_size);

            SDL_DrawGPUIndexedPrimitives(renderer->render_pass, draw_item->indices_size / sizeof(u16), 1, 0, 0, 0);

        } break;
        case PipelineType_Mesh: {
            SDL_BindGPUGraphicsPipeline(renderer->render_pass, renderer->solid_mesh_pipeline);

            SDL_GPUBufferBinding buffer_binding = {
                .buffer = renderer->dynamic_vertex_buffer,
                .offset = draw_item->vertex_buffer_offset,
            };
            SDL_BindGPUVertexBuffers(renderer->render_pass, 0, &buffer_binding, 1);

            buffer_binding = (SDL_GPUBufferBinding) {
                .buffer = renderer->dynamic_index_buffer,
                .offset = draw_item->index_buffer_offset,
            };

            SDL_GPUIndexElementSize index_element_size = SDL_GPU_INDEXELEMENTSIZE_16BIT;

            SDL_BindGPUIndexBuffer(renderer->render_pass, &buffer_binding, index_element_size);
            SDL_DrawGPUIndexedPrimitives(renderer->render_pass, draw_item->indices_size / sizeof(u16), 1, 0, 0, 0);

        } break;

        }

    }

    SDL_EndGPURenderPass(renderer->render_pass);

    Rect dst_rect = render_target_rect(renderer->swapchain_w, renderer->swapchain_h, renderer->res_width, renderer->res_height);
    renderer->dst_rect = dst_rect;

    SDL_BlitGPUTexture(renderer->draw_command_buffer, (SDL_GPUBlitInfo[]){{
        .source = {
            .texture = renderer->render_target,
            .w = renderer->res_width,
            .h = renderer->res_height,
        },
        .destination = {
            .texture = renderer->swapchain_texture,
            .x = dst_rect.x,
            .y = dst_rect.y,
            .w = dst_rect.w,
            .h = dst_rect.h,
        },
        // .filter = SDL_GPU_FILTER_LINEAR,
    }});


    SDL_SubmitGPUCommandBuffer(renderer->draw_command_buffer);
}

// very retarded api
// renderer can store texture refs for known stuff
// other stuff user has to keep it
// texture id only for renderer managed texture
// renderer needs to know about textures for bucketing draw calls
Texture load_texture(TextureID texture_id, Image image) {
    const int comp = 4;

    SDL_GPUTextureCreateInfo texture_create_info = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = (uint32_t)image.w,
        .height = (uint32_t)image.h,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };

    SDL_GPUTexture* texture = SDL_CreateGPUTexture(renderer->device, &texture_create_info);

    SDL_GPUSamplerCreateInfo sampler_create_info = {
        .min_filter = SDL_GPU_FILTER_NEAREST,
        .mag_filter = SDL_GPU_FILTER_NEAREST,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    };

    SDL_GPUSampler* sampler = SDL_CreateGPUSampler(renderer->device, &sampler_create_info);

    SDL_GPUCommandBuffer* upload_command_buffer = SDL_AcquireGPUCommandBuffer(renderer->device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_command_buffer);

    SDL_GPUTransferBufferCreateInfo transfer_create_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = (uint32_t)(image.w * image.h) * comp,
    };

    SDL_GPUTransferBuffer* texture_transfer_buffer = SDL_CreateGPUTransferBuffer(renderer->device, &transfer_create_info);

    void* texture_transfer_ptr = SDL_MapGPUTransferBuffer(renderer->device, texture_transfer_buffer, false);
    memcpy(texture_transfer_ptr, image.data, image.w * image.h * comp);

    SDL_UnmapGPUTransferBuffer(renderer->device, texture_transfer_buffer);

    SDL_GPUTextureTransferInfo texture_transfer_info = {
        .transfer_buffer = texture_transfer_buffer,
    };
    SDL_GPUTextureRegion texture_region = {
        .texture = texture,
        .w = (Uint32)image.w,
        .h = (Uint32)image.h,
        .d = 1,
    };

    SDL_UploadToGPUTexture(copy_pass, &texture_transfer_info, &texture_region, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_command_buffer);
    SDL_ReleaseGPUTransferBuffer(renderer->device, texture_transfer_buffer);

    Texture texture_ref = {.texture = texture, .sampler = sampler, .w = image.w, .h = image.h};
    if (texture_id) {
        renderer->textures[texture_id] = texture_ref;
    }
    return texture_ref;
}

// void draw_lines(Renderer& renderer, float2* vertices, int vert_count, RGBA color) {
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

RGBA rgban_to_rgba(float4 rgba) {
    return (RGBA) {
        .r = (u8)(rgba.r * 255),
        .g = (u8)(rgba.b * 255),
        .b = (u8)(rgba.g * 255),
        .a = (u8)(rgba.a * 255),
    };
}

void draw_world_lines(Camera2D camera, float2* vertices, int vert_count, float4 color) {
    camera.position = snap_pos(camera.position);

    int start_index = 0;
    u32 verts_size = sizeof(PositionColorVertex) * vert_count;
    u32 indices_size = sizeof(u16) * 2 * (vert_count - 1);

    bool add_new_draw = true;
    Slice_DrawItem* draw_list = &renderer->draw_list;
    if (draw_list->length > 0) {
        DrawItem* last_draw = slice_getp(*draw_list, draw_list->length - 1);
        if (last_draw->type == PipelineType_Line) {
            start_index = last_draw->vertices_size / sizeof(PositionColorVertex);

            last_draw->vertices_size += verts_size;
            last_draw->indices_size += indices_size;

            add_new_draw = false;
        } 
    }

    if (add_new_draw) {
        slice_push(draw_list, (DrawItem){
            .type = PipelineType_Line,
            .vertex_buffer_offset = (u32)renderer->vertex_data.length,
            .vertices_size = verts_size,
            .index_buffer_offset = (u32)renderer->index_data.length,
            .indices_size = indices_size,
        });
    }

    for (int i = 0; i < vert_count; i++) {
        PositionColorVertex vertex = {
                .position = world_to_normalized(camera, snap_pos(vertices[i])),
                .color = color,
        };

        slice_push_buffer(&renderer->vertex_data, (u8*)&vertex, sizeof(vertex));
    }

    for (int i = start_index; i < start_index + vert_count - 1; i++) {
        u16 indices[] = {(u16)i, (u16)(i+1)};
        slice_push_buffer(&renderer->index_data, (u8*)indices, sizeof(indices));
    }
}

void draw_world_polygon(Camera2D camera, float2* verts, int vert_count, float4 color) {
    ASSERT(vert_count >= 3);

    bool push_new_draw = true;
    u32 verts_size = sizeof(PositionColorVertex) * vert_count;
    u32 indxs_size = sizeof(u16) * (vert_count - 2) * 3;
    u16 start_index = 0;
    Slice_DrawItem* draw_list = &renderer->draw_list;
    if (draw_list->length > 0) {
        DrawItem* last_draw = slice_getp(*draw_list, draw_list->length - 1);

        if (last_draw->type == PipelineType_Mesh) {
            push_new_draw = false;
            start_index = last_draw->vertices_size / sizeof(PositionColorVertex); //GET PREVIOUS VERTEX SIZE BEFORE APPEND SIZE PLS

            last_draw->vertices_size += verts_size;
            last_draw->indices_size += indxs_size;
        }
    }

    if (push_new_draw) {
        slice_push(draw_list, (DrawItem){
            .type = PipelineType_Mesh,
            .vertex_buffer_offset = (u32)renderer->vertex_data.length,
            .vertices_size = verts_size,
            .index_buffer_offset = (u32)renderer->index_data.length,
            .indices_size = indxs_size,
        });
    }

    for (u16 i = 1; i < vert_count - 1; i++) {
        u16 indices[] = {(u16)(start_index + 0), (u16)(start_index + i), (u16)(start_index + i + 1)};
        slice_push_buffer(&renderer->index_data, (u8*)indices, sizeof(indices));
    }

    for (int i = 0; i < vert_count; i++) {
        PositionColorVertex vert = {
            .position = world_to_normalized(camera, verts[i]),
            .color = color,
        };

        slice_push_buffer(&renderer->vertex_data, (u8*)&vert, sizeof(vert));
    }
}

void draw_rect(Rect normalized_rect, float4 rgba) {
    PositionColorVertex vertices[4] = {
        {{-1.0, -1.0}},
        {{1.0, -1.0}},
        {{-1.0, 1.0}},
        {{1.0, 1.0}},
    };

    u16 indices[6] = {
        0,
        1,
        2,
        2,
        1,
        3,
    };


    bool push_new_draw = true;
    u16 start_index = 0;
    Slice_DrawItem* draw_list = &renderer->draw_list;
    if (draw_list->length > 0) {
        DrawItem* last_draw = slice_getp(*draw_list, draw_list->length - 1);

        if (last_draw->type == PipelineType_Mesh) {
            push_new_draw = false;
            start_index = last_draw->vertices_size / sizeof(PositionColorVertex); //GET PREVIOUS VERTEX SIZE BEFORE APPEND SIZE PLS

            last_draw->vertices_size += sizeof(vertices);
            last_draw->indices_size += sizeof(indices);
        }
    }

    if (push_new_draw) {
        slice_push(draw_list, (DrawItem){
            .type = PipelineType_Mesh,
            .vertex_buffer_offset = (u32)renderer->vertex_data.length,
            .vertices_size = sizeof(vertices),
            .index_buffer_offset = (u32)renderer->index_data.length,
            .indices_size = sizeof(indices),
        });
    }

    for (int i = 0; i < 6; i++) {
        indices[i] += start_index;
    }

    for (int i = 0; i < 4; i++) {
        float2* vert = &vertices[i].position;
        *vert = float2_add(float2_mult(*vert, normalized_rect.size), normalized_rect.position);
        vertices[i].color = rgba;
    }

    slice_push_buffer(&renderer->vertex_data, (u8*)vertices, sizeof(vertices));
    slice_push_buffer(&renderer->index_data, (u8*)indices, sizeof(indices));
}

Rect screen_rect_to_normalized(Rect rect, float2 resolution) {
    rect.position = float2_add(rect.position, float2_scale(rect.size, 1 / 2.0));
    Rect normal_rect = {
        .position = float2_sub(float2_scale(float2_div(rect.position, resolution), 2.0), float2_scale(float2_one, 1.0)),
        .size = float2_div(rect.size, resolution),
    };
    normal_rect.position.y *= -1;
    
    return normal_rect;
}

void draw_world_rect(Camera2D camera, Rect rect, float4 rgba) {
    draw_rect(world_rect_to_normalized(camera, rect), rgba);
}

void draw_screen_rect(Rect rect, float4 rgba) {
    float2 resolution = {(f32)renderer->window_width, (f32)renderer->window_height};
    draw_rect(screen_rect_to_normalized(rect, resolution), rgba);
}

