#include "SDL3/SDL_gpu.h"
#include "codegen/shaders.h"
#include "font.h"
#include "glm/ext/vector_float2.hpp"
#include <SDL3/SDL.h>
#include <iostream>
#include <tracy/Tracy.hpp>

#include <vector>
#include <fstream>

constexpr int window_width = 1024;
constexpr int window_height = 768;

// std::vector<char> read_file(const char* file_name) {
//     std::string path = "data/" + std::string(file_name);
//     std::ifstream file{path, std::ios::binary | std::ios::ate};
//     if (!file) {
//         exit(1);
//     }
//
//     auto size = file.tellg();
//     std::vector<char> data(size, '\0');
//     file.seekg(0);
//     file.read(data.data(), size);
//
//     return data;
// }

SDL_GPUShader* LoadShader(
	SDL_GPUDevice* device,
	const char* shaderPath,
	Uint32 samplerCount,
	Uint32 uniformBufferCount,
	Uint32 storageBufferCount,
	Uint32 storageTextureCount
) {
    SDL_GPUShaderStage stage;
	// Auto-detect the shader stage from the file name for convenience
	if (SDL_strstr(shaderPath, ".vert"))
	{
		stage = SDL_GPU_SHADERSTAGE_VERTEX;
	}
	else if (SDL_strstr(shaderPath, ".frag"))
	{
		stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	}
	else
	{
		SDL_Log("Invalid shader stage!");
		return NULL;
	}

	size_t codeSize;
	auto code = (const unsigned char*)SDL_LoadFile(shaderPath, &codeSize);
	if (code == NULL)
	{
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
	if (shader == NULL)
	{
		SDL_Log("Failed to create shader!");
		SDL_free((void*)code);
		return NULL;
	}

	SDL_free((void*)code);
	return shader;
}


int main() {
    // Font::save_font_atlas_image();
    // return 0;



    auto flags = SDL_WINDOW_INPUT_FOCUS; // | SDL_WINDOW_FULLSCREEN;

    auto window = SDL_CreateWindow("ye", window_width, window_height, flags);
    if (window == NULL) {
        SDL_Log("CreateWindow failed: %s", SDL_GetError());
    }

    auto device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    if (device == NULL) {
        SDL_Log("CreateGPUDevice failed");
        return 1;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        SDL_Log("ClaimWindow failed");
		return 1;
    }

    auto vertex_shader = LoadShader(device, shaders::rect_vert, 0, 1, 0, 0);

    auto fragment_shader = LoadShader(device, shaders::rect_frag, 0, 0, 0, 0);

    auto create_info = SDL_GPUGraphicsPipelineCreateInfo {
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
        .target_info = {
            .color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
                .format = SDL_GetGPUSwapchainTextureFormat(device, window)
            }},
			.num_color_targets = 1,
		},
    };
    create_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    auto pipeline = SDL_CreateGPUGraphicsPipeline(device, &create_info);

    std::vector<unsigned char> bitmap;
    Font::generate_text_font_atlas(bitmap);

    auto texture_create_info = SDL_GPUTextureCreateInfo {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = 512,
        .height = 512,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };

    auto texture = SDL_CreateGPUTexture(device, &texture_create_info);

    SDL_ReleaseGPUShader(device, vertex_shader);
    SDL_ReleaseGPUShader(device, fragment_shader);

    while (1) {
        bool quit = false;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                quit = true;
                break;
            }
        }

        if (quit) {
            break;
        }


        auto command_buffer = SDL_AcquireGPUCommandBuffer(device);

        SDL_GPUTexture* swapchain_texture;
        SDL_AcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, NULL, NULL);
            
        struct Rect {
            glm::vec2 position;
            glm::vec2 scale;
        };

        auto rect = Rect {
            {0, 0},
            {0.5, 0.5},
        };

        if (swapchain_texture != NULL) {
            SDL_GPUColorTargetInfo color_target_info = { 0 };
            color_target_info.texture = swapchain_texture;
            color_target_info.clear_color = (SDL_FColor){ 0.0f, 0.0f, 0.0f, 1.0f };
            color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            color_target_info.store_op = SDL_GPU_STOREOP_STORE;

            auto render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, NULL);

            SDL_BindGPUGraphicsPipeline(render_pass, pipeline);


            SDL_PushGPUVertexUniformData(command_buffer, 0, &rect, sizeof(Rect));
            SDL_DrawGPUPrimitives(render_pass, 4, 1, 0, 0);

            SDL_EndGPURenderPass(render_pass);
        }

        SDL_SubmitGPUCommandBuffer(command_buffer);
    }

    SDL_DestroyWindow(window);

    SDL_Quit();
}
