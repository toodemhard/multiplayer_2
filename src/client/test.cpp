// // #include <glad/glad.h>
// #include <SDL3/SDL.h>
// #include <glm/vec2.hpp>
// #include "codegen/assets.h"
// #include "codegen/shaders.h"
// #include "stb_image.h"
//
// SDL_GPUShader* LoadShader(
//     SDL_GPUDevice* device,
//     const char* shaderPath,
//     Uint32 samplerCount,
//     Uint32 uniformBufferCount,
//     Uint32 storageBufferCount,
//     Uint32 storageTextureCount
// ) {
//     SDL_GPUShaderStage stage;
//     // Auto-detect the shader stage from the file name for convenience
//     if (SDL_strstr(shaderPath, ".vert")) {
//         stage = SDL_GPU_SHADERSTAGE_VERTEX;
//     } else if (SDL_strstr(shaderPath, ".frag")) {
//         stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
//     } else {
//         SDL_Log("Invalid shader stage!");
//         return NULL;
//     }
//
//     size_t codeSize;
//     auto code = (const unsigned char*)SDL_LoadFile(shaderPath, &codeSize);
//     if (code == NULL) {
//         SDL_Log("Failed to load shader from disk! %s", shaderPath);
//         return NULL;
//     }
//
//     SDL_GPUShaderCreateInfo shaderInfo = {
//         .code_size = codeSize,
//         .code = code,
//         .entrypoint = "main",
//         .format = SDL_GPU_SHADERFORMAT_SPIRV,
//         .stage = stage,
//         .num_samplers = samplerCount,
//         .num_storage_textures = storageTextureCount,
//         .num_storage_buffers = storageBufferCount,
//         .num_uniform_buffers = uniformBufferCount,
//     };
//     SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderInfo);
//     if (shader == NULL) {
//         SDL_Log("Failed to create shader!");
//         SDL_free((void*)code);
//         return NULL;
//     }
//
//     SDL_free((void*)code);
//     return shader;
// }
//
// int IDK() {
//     constexpr int window_width = 1024;
//     constexpr int window_height = 768;
//
//     auto flags = SDL_WINDOW_INPUT_FOCUS; // | SDL_WINDOW_FULLSCREEN;
//
//     auto window = SDL_CreateWindow("ye", window_width, window_height, flags);
//     if (window == NULL) {
//         SDL_Log("CreateWindow failed: %s", SDL_GetError());
//     }
//
//     auto device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
//     if (device == NULL) {
//         SDL_Log("CreateGPUDevice failed");
//         return 1;
//     }
//
//     if (!SDL_ClaimWindowForGPUDevice(device, window)) {
//         SDL_Log("ClaimWindow failed");
//         return 1;
//     }
//
//     auto vertex_shader = LoadShader(device, shaders::rect_vert, 0, 1, 0, 0);
//
//     auto fragment_shader = LoadShader(device, shaders::rect_frag, 1, 0, 0, 0);
//
//     int width, height, comp;
//     unsigned char* image = stbi_load(assets::amogus_png, &width, &height, &comp, STBI_rgb_alpha);
//
//     auto texture_create_info = SDL_GPUTextureCreateInfo{
//         .type = SDL_GPU_TEXTURETYPE_2D,
//         .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
//         .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
//         .width = (uint32_t)width,
//         .height = (uint32_t)height,
//         .layer_count_or_depth = 1,
//         .num_levels = 1,
//     };
//
//     auto texture = SDL_CreateGPUTexture(device, &texture_create_info);
//
//     auto sampler_create_info = SDL_GPUSamplerCreateInfo{
//         .min_filter = SDL_GPU_FILTER_NEAREST,
//         .mag_filter = SDL_GPU_FILTER_NEAREST,
//         .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
//         .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
//         .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
//         .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
//     };
//
//     auto sampler = SDL_CreateGPUSampler(device, &sampler_create_info);
//
//     auto upload_command_buffer = SDL_AcquireGPUCommandBuffer(device);
//     auto copy_pass = SDL_BeginGPUCopyPass(upload_command_buffer);
//
//     auto transfer_create_info = SDL_GPUTransferBufferCreateInfo{
//         .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
//         .size = (uint32_t)(width * height) * 4,
//     };
//
//     auto texture_transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_create_info);
//
//     auto texture_transfer_ptr = SDL_MapGPUTransferBuffer(device, texture_transfer_buffer, false);
//     memcpy(texture_transfer_ptr, image, width * height * comp);
//
//     SDL_UnmapGPUTransferBuffer(device, texture_transfer_buffer);
//
//     auto texture_transfer_info = SDL_GPUTextureTransferInfo{
//         .transfer_buffer = texture_transfer_buffer,
//     };
//     auto texture_region = SDL_GPUTextureRegion{
//         .texture = texture,
//         .w = (Uint32)width,
//         .h = (Uint32)height,
//         .d = 1,
//     };
//
//     SDL_UploadToGPUTexture(copy_pass, &texture_transfer_info, &texture_region, false);
//     SDL_EndGPUCopyPass(copy_pass);
//     SDL_SubmitGPUCommandBuffer(upload_command_buffer);
//     SDL_ReleaseGPUTransferBuffer(device, texture_transfer_buffer);
//
//     auto create_info = SDL_GPUGraphicsPipelineCreateInfo{
//         .vertex_shader = vertex_shader,
//         .fragment_shader = fragment_shader,
//         .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
//         .target_info =
//             {
//                 .color_target_descriptions =
//                     (SDL_GPUColorTargetDescription[]){{.format = SDL_GetGPUSwapchainTextureFormat(device, window)}},
//                 .num_color_targets = 1,
//             },
//     };
//     // create_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
//     auto pipeline = SDL_CreateGPUGraphicsPipeline(device, &create_info);
//
//     SDL_ReleaseGPUShader(device, vertex_shader);
//     SDL_ReleaseGPUShader(device, fragment_shader);
//
//     while (1) {
//         bool quit = false;
//
//         SDL_Event event;
//         while (SDL_PollEvent(&event)) {
//             if (event.type == SDL_EVENT_QUIT) {
//                 quit = true;
//                 break;
//             }
//         }
//
//         if (quit) {
//             break;
//         }
//
//         auto command_buffer = SDL_AcquireGPUCommandBuffer(device);
//
//         SDL_GPUTexture* swapchain_texture;
//         SDL_AcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, NULL, NULL);
//
//         struct Rect {
//             glm::vec2 position;
//             glm::vec2 scale;
//         };
//
//         auto rect = Rect{
//             {0, 0},
//             {0.5, 0.5},
//         };
//
//         if (swapchain_texture != NULL) {
//             SDL_GPUColorTargetInfo color_target_info = {0};
//             color_target_info.texture = swapchain_texture;
//             color_target_info.clear_color = (SDL_FColor){0.0f, 0.0f, 0.0f, 1.0f};
//             color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
//             color_target_info.store_op = SDL_GPU_STOREOP_STORE;
//
//             auto render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, NULL);
//
//             SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
//
//             auto texture_sampler_binding = SDL_GPUTextureSamplerBinding{.texture = texture, .sampler = sampler};
//             SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_binding, 1);
//
//             SDL_PushGPUVertexUniformData(command_buffer, 0, &rect, sizeof(Rect));
//             SDL_DrawGPUPrimitives(render_pass, 4, 1, 0, 0);
//
//             SDL_EndGPURenderPass(render_pass);
//         }
//
//         SDL_SubmitGPUCommandBuffer(command_buffer);
//     }
//
//     SDL_DestroyWindow(window);
//
//     SDL_Quit();
//
//     return 0;
// }

// int opengl(){ 
//     constexpr int window_width = 1024;
//     constexpr int window_height = 768;
//
//     if (!SDL_Init(SDL_INIT_VIDEO)) {
//         SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
//         return -1;
//     }
//
//     auto flags = SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE; // | SDL_WINDOW_FULLSCREEN;
//
//     auto window = SDL_CreateWindow("ye", window_width, window_height, flags);
//     if (window == NULL) {
//         SDL_Log("CreateWindow failed: %s", SDL_GetError());
//         return -1;
//     }
//
//     auto gl_context = SDL_GL_CreateContext(window);
//     if (gl_context == NULL) {
//         SDL_Log("Failed to create OpenGL Context: %s", SDL_GetError());
//         return -1;
//     }
//
//     SDL_GL_MakeCurrent(window, gl_context);
//
//     if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
//         SDL_Log("Failed to initialize GLAD");
//         return -1;
//     }
//
//     glViewport(0, 0, window_width, window_height);
//
//
//     int width, height, comp;
//     unsigned char* image = stbi_load(assets::amogus_png, &width, &height, &comp, STBI_rgb_alpha);
//
//     float vertices[] = {
//         -0.5f, -0.5f, 0.0f,
//          0.5f, -0.5f, 0.0f,
//          0.0f,  0.5f, 0.0f
//     };
//
//     unsigned int vbo;
//     glGenBuffers(1, &vbo);
//
//     glBindBuffer(GL_ARRAY_BUFFER, vbo);
//     glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
//
//
//
//     while (1) {
//         bool quit = false;
//
//         SDL_Event event;
//         while (SDL_PollEvent(&event)) {
//             if (event.type == SDL_EVENT_QUIT) {
//                 quit = true;
//                 break;
//             }
//
//             switch (event.type) {
//             case SDL_EVENT_WINDOW_RESIZED:
//                 int width, height;
//                 SDL_GetWindowSize(window, &width, &height);
//                 glViewport(0, 0, width, height);
//                 break;
//             }
//
//         }
//
//         if (quit) {
//             break;
//         }
//         glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
//         glClear(GL_COLOR_BUFFER_BIT);
//
//         SDL_GL_SwapWindow(window);
//     }
//
//     SDL_DestroyWindow(window);
//
//     SDL_Quit();
//
//     return 0;
// }
