#pragma once

typedef struct Texture {
    SDL_GPUTexture* texture;
    SDL_GPUSampler* sampler;
    int w, h;
} Texture;

typedef struct PositionColorVertex {
    float2 position;
    float4 color;
} PositionColorVertex;

typedef struct PositionUvColorVertex {
    float2 position;
    float2 uv;
    RGBA color;
} PositionUvColorVertex;

typedef struct MixColor {
    float4 color;
    float t;
} MixColor;

typedef struct Camera2D {
    float2 position;
    float2 size;
} Camera2D;

typedef struct RectVertex {
    float2 position;
    float2 uv;
} RectVertex;

typedef struct SpriteVertex {
    float2 position;
    float2 size;
    float2 uv_position;
    float2 uv_size;
    
    RGBA mult_color;
    RGBA mix_color;
    float t;
} SpriteVertex;

typedef struct SpriteProperties {
    TextureID texture_id;
    Rect src_rect;
    Opt_RGBA mult_color;
    RGBA mix_color;
    float t;
} SpriteProperties;

typedef enum PipelineType {
    PipelineType_Sprite,
    PipelineType_Mesh,
    PipelineType_Line,
} PipelineType;

typedef struct DrawItem {
    PipelineType type;

    TextureID texture_id;
    u32 vertex_buffer_offset;
    u32 vertices_size;

    u32 index_buffer_offset;
    u32 indices_size;
} DrawItem;
slice_def(DrawItem);

typedef struct Renderer {
    i32 window_width;
    i32 window_height;

    i32 res_width;
    i32 res_height;

    Rect dst_rect;

    Camera2D* active_camera;

    SDL_GPUDevice* device;

    Texture textures[TextureID_Count];

    SDL_GPUTexture* swapchain_texture;
    u32 swapchain_w;
    u32 swapchain_h;

    SDL_GPUTexture* depth_texture;
    SDL_GPUCommandBuffer* draw_command_buffer;
    SDL_GPURenderPass* render_pass;

    SDL_GPUTexture* render_target;

    Slice_DrawItem draw_list;
    Slice_u8 vertex_data;
    Slice_u8 index_data;
    SDL_GPUBuffer* dynamic_vertex_buffer;
    SDL_GPUBuffer* dynamic_index_buffer;
    SDL_GPUTransferBuffer* transfer_buffer;

    SDL_GPUGraphicsPipeline* line_pipeline;
    SDL_GPUGraphicsPipeline* solid_mesh_pipeline;
    SDL_GPUGraphicsPipeline* sprite_pipeline;
    SDL_GPUBuffer* rect_vertex_buffer;
    SDL_GPUBuffer* rect_index_buffer;

    bool null_swapchain;
} Renderer;


typedef struct Image {
    int w, h;
    void* data;
} Image;


inline float2 screen_to_world_pos(Camera2D cam, float2 screen_pos, int s_w, int s_h) {
    screen_pos.y = s_h - screen_pos.y;
    float2 a = float2_sub(float2_div( screen_pos, (float2){(float)s_w, (float)s_h}), float2_scale(float2_one, 0.5));
    return float2_add(cam.position, float2_mult(a, cam.size));
}

float2 snap_pos(float2 pos);

float2 texture_dimensions(TextureID texture_id);


SDL_GPUShader* load_shader( SDL_GPUDevice* device, ShaderID shader_id, Uint32 samplerCount, Uint32 uniformBufferCount, Uint32 storageBufferCount, Uint32 storageTextureCount);

void renderer_set_ctx(Renderer* _renderer);
int init_renderer(Renderer* renderer_out, SDL_Window* window);
void begin_rendering(SDL_Window* window, Arena* temp_arena);
void end_rendering();

float2 world_to_normalized(Camera2D camera, float2 world_pos);
Rect world_rect_to_normalized(Camera2D camera, Rect world_rect);
Rect screen_rect_to_normalized(Rect rect, float2 resolution);

void draw_sprite_world(Camera2D camera, Rect world_rect, SpriteProperties properties);
void draw_sprite_screen(Rect screen_rect, SpriteProperties properties);
void draw_world_lines(Camera2D camera, float2* vertices, int vert_count, float4 color);
void draw_world_rect(Camera2D camera, Rect rect, float4 rgba);
void draw_screen_rect(Rect rect, float4 rgba);

Texture load_texture(TextureID texture_id, Image image);

void draw_world_polygon(Camera2D camera, float2* verts, int vert_count, float4 color);
