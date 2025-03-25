#pragma once

struct Texture {
    SDL_GPUTexture* texture;
    SDL_GPUSampler* sampler;
    int w, h;
};

struct PositionColorVertex {
    float2 position;
    float4 color;
};

struct PositionUvColorVertex {
    float2 position;
    float2 uv;
    RGBA color;
};

struct MixColor {
    float4 color;
    float t;
};

struct Camera2D {
    float2 position;
    float2 scale;
};

struct RectVertex {
    float2 position;
    float2 uv;
};

struct SpriteVertex {
    float2 position;
    float2 size;
    float2 uv_position;
    float2 uv_size;
    
    RGBA mult_color;
    RGBA mix_color;
    float t;
};

struct SpriteProperties {
    TextureID texture_id;
    Rect src_rect;
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
    i32 window_width = 1024;
    i32 window_height = 768;

    Camera2D* active_camera;

    SDL_GPUDevice* device;

    Texture textures[TextureID_Count];

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


inline float2 screen_to_world_pos(const Camera2D& cam, float2 screen_pos, int s_w, int s_h) {
    screen_pos.y = s_h - screen_pos.y;

    return cam.position + (screen_pos / float2{(float)s_w, (float)s_h} - 0.5f * float2_one) * cam.scale;
}

float2 snap_pos(float2 pos);

float2 texture_dimensions(TextureID texture_id);


SDL_GPUShader* load_shader( SDL_GPUDevice* device, const char* shaderPath, Uint32 samplerCount, Uint32 uniformBufferCount, Uint32 storageBufferCount, Uint32 storageTextureCount);

void renderer_set_ctx(Renderer* _renderer);
int init_renderer(Renderer* renderer_out, SDL_Window* window);
void begin_rendering(SDL_Window* window, Arena* temp_arena);
void end_rendering();

float2 world_to_normalized(Camera2D camera, float2 world_pos);
Rect world_rect_to_normalized(Camera2D camera, Rect world_rect);
Rect screen_rect_to_normalized(Rect rect, float2 resolution);

void draw_sprite_world(Camera2D camera, Rect world_rect, const SpriteProperties& properties);
void draw_sprite_screen(Rect screen_rect, const SpriteProperties& properties);
void draw_world_lines(Camera2D camera, float2* vertices, int vert_count, float4 color);
void draw_world_rect(Camera2D camera, Rect rect, float4 rgba);
void draw_screen_rect(Rect rect, float4 rgba);

Texture load_texture(TextureID texture_id, const Image& image);

void draw_world_polygon(const Camera2D& camera, float2* verts, int vert_count, float4 color);
