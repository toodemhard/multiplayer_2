#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_mouse.h"
#include "client.h"
#include "codegen/shaders.h"
#include <SDL3/SDL.h>

#include "imgui.h"

#include <chrono>
#include <future>
#include <glm/glm.hpp>
#include <iostream>
#include <numeric>
#include <optional>

#include "color.h"
#include "font.h"
#include "game.h"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "net_common.h"
#include "panic.h"
#include "renderer.h"
#include "stb_image.h"

#include "input.h"
#include "assets.h"
#include "ui.h"

constexpr double fixed_dt = 1.0 / tick_rate;
constexpr double speed_up_fraction = 0.05;

constexpr int window_width = 1024;
constexpr int window_height = 768;

class LocalScene {

};

void poll_event() {

}

void accumulate_input_events(Input::Input& accumulator, Input::Input& new_frame) {
    for (int i = 0; i < SDL_SCANCODE_COUNT; i++) {
        accumulator.keyboard_up[i] |= new_frame.keyboard_up[i];
        accumulator.keyboard_down[i] |= new_frame.keyboard_down[i];
        accumulator.keyboard_repeat[i] |= new_frame.keyboard_repeat[i];

        accumulator.button_up_flags |= new_frame.button_up_flags;
        accumulator.button_down_flags |= new_frame.button_down_flags;
    }
}

// void begin_tick_input(input ) {
//
//     inpubutton_held_flags = SDL_GetMouseState(&mouse_pos.x, &mouse_pos.y);
//     mod_state = SDL_GetModState();
// }

class MultiplayerScene {
public:
    Input::Input& m_input;
    Font* m_font;

    double accumulator = 0.0;
    uint32_t frame = {};
    double time = {};
    int current_tick = 0;

    double speed_up_dt = 0.0;

    float idle_period = 0.2;
    int idle_count = 2;
    int idle_period_ticks = idle_period * tick_rate;
    int idle_cycle_period_ticks = idle_period * idle_count * tick_rate;

    UI ui;

    Input::Input m_tick_input;

    Camera2D m_camera{
        {0, 0},
        {1 * 4.0f/3.0f, 1},
    };

    GameClient client;

    MultiplayerScene(Input::Input& input, Font* font) : m_input(input), ui(font), m_font(font) {
        m_camera.scale *= 5.0f;

        m_tick_input.init_keybinds(Input::default_keybindings);
    }

    void connect() {
        client.connect(yojimbo::Address(127, 0, 0, 1, 5000));
    }

    void update(double delta_time) {
        using namespace Input;

        accumulator += delta_time;
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        accumulate_input_events(m_tick_input, m_input);
        
        while (accumulator >= fixed_dt) {
            m_tick_input.begin_frame();
            accumulator = accumulator - fixed_dt + speed_up_dt;
            frame++;
            time += fixed_dt;
            // printf("frame:%d %fs\n", frame, time);



            glm::vec2 move_input{};

            if (m_input.key_down(SDL_SCANCODE_R)) {
                accumulator += fixed_dt;
            }

            if (m_input.key_down(SDL_SCANCODE_E)) {
                accumulator -= fixed_dt;
            }

            PlayerInput player_input{};
            if (m_tick_input.action_held(ActionID::move_up)) {
                player_input.up = true;
            }
            if (m_tick_input.action_held(ActionID::move_down)) {
                player_input.down = true;
            }
            if (m_tick_input.action_held(ActionID::move_left)) {
                player_input.left = true;
            }
            if (m_tick_input.action_held(ActionID::move_right)) {
                player_input.right = true;
            }
            // if (glm::length(move_input) > 0) {
            //     player_position += glm::normalize(move_input) * (float)delta_time * 4.0f;
            // }

            if (m_tick_input.key_down(SDL_SCANCODE_F)) {
                printf("asdf\n");
            }

            if (m_tick_input.mouse_down(SDL_BUTTON_LEFT) || m_input.key_down(SDL_SCANCODE_F)) {
                player_input.fire = true;
            }

            player_input.cursor_world_pos = screen_to_world_pos(m_camera, m_tick_input.mouse_pos, window_width, window_height);
            // printf("upate\n");

            // ImGui_ImplSDLRenderer3_NewFrame();
            // ImGui_ImplSDL3_NewFrame();
            // ImGui::NewFrame();

            int throttle_ticks{};
            client.fixed_update(fixed_dt, player_input, m_input, &throttle_ticks);

            speed_up_dt = throttle_ticks * fixed_dt * speed_up_fraction;

            // accumulator += fixed_dt * throttle_ticks;

            current_tick++;

            m_tick_input.end_frame();
        }

        client.update();
    }

    void render(Renderer& renderer, SDL_Window* window, Texture textures[(int)ImageID::count]) {
        using namespace renderer;

        auto& char_sheet = textures[(int)ImageID::char_sheet_png];
        auto& bullet_texture = textures[(int)ImageID::bullet_png];


        // ui.draw(renderer);



        const auto& state = client.m_state;

        int idle_frame = current_tick % idle_cycle_period_ticks / idle_period_ticks;



        for (int i = 0; i < max_player_count; i++) {
            if (state.players_active[i]) {
                auto& player = state.players[i];
                // draw_world_textured_rect(renderer, camera, {}, {state.players.position[i], {1, 1}}, amogus_texture);
                draw_world_textured_rect(renderer, m_camera, Rect{{16 * idle_frame + 2,0}, {16,16}}, {player.position, {1, 1}}, char_sheet);
            }
        }

        for (int i = 0; i < bullets_capacity; i++) {

            if (!state.bullets_active[i]) {
                continue;
            }
            auto& bullet = state.bullets[i];

            draw_world_textured_rect(renderer, m_camera, {}, {{bullet.position}, {0.5, 0.5}}, bullet_texture);
        }

        // draw_world_textured_rect(renderer, camera, nullptr, {client.m_pos, {1, 1}}, amogus_texture);
        // auto string = std::format("render: {:.3f}ms", last_render_duration * 1000.0);
        // font::draw_text(renderer, m_font, string.data(), 64, {20, 30});


        // auto viewport = SDL_GPUViewport {
        //     .x = 0,
        //     .y = 0,
        //     .w = (float)renderer.window_width,
        //     .h = (float)renderer.window_height,
        // };
        // SDL_SetGPUViewport(renderer.render_pass, &viewport);
        // renderer::render_geometry_raw(renderer, font.atlas_texture, vertices, 4, indices, 6, sizeof(uint16_t));


        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), &renderer);



        // last_render_duration = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - render_begin_time).count();
    }
};

namespace app {

int run() {
    using namespace renderer;


    // panic("asasdf {} {}", 234, 65345);

    auto flags = SDL_WINDOW_INPUT_FOCUS; // | SDL_WINDOW_FULLSCREEN;

    auto window = SDL_CreateWindow("ye", window_width, window_height, flags);
    if (window == NULL) {
        SDL_Log("CreateWindow failed: %s", SDL_GetError());
    }

    Renderer renderer;
    if (init_renderer(&renderer, window) != 0) {
        return 1;
    }

    Texture textures[(int)ImageID::count];
    std::future<void> texture_futures[(int)ImageID::count];

    auto image_to_texture = [](Renderer& renderer, Texture textures[], ImageID id) {
        int width, height, comp;
        unsigned char* image = stbi_load(image_paths[(int)id], &width, &height, &comp, STBI_rgb_alpha);
        textures[(int)id] = load_texture(renderer, Image{.w = width, .h = height, .data = image});
    };

    for (int i = 0; i < (int)ImageID::count; i++) {
        texture_futures[i] = std::async(std::launch::async, image_to_texture, std::ref(renderer), textures, (ImageID)i);
    }

    Font font;
    auto font_future =  std::async(std::launch::async, font::load_font, &font, std::ref(renderer), font_paths[(int)FontID::Avenir_LT_Std_95_Black_ttf], 512, 512, 64);

    Input::Input input;
    input.init_keybinds(Input::default_keybindings);

    glm::vec2 player_position(0, 0);

    // __debugbreak();


    InitializeYojimbo();


    for (int i = 0; i < (int)ImageID::count; i++) {
        texture_futures[i].wait();
    }
    font_future.wait();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui_ImplSDL3_InitForSDLRenderer(window, &renderer);
    ImGui_ImplSDLRenderer3_Init(&renderer);



    auto last_frame_time = std::chrono::high_resolution_clock::now();
    double last_render_duration = 0.0;
    double accumulator = 0.0;
    uint32_t frame = {};
    double time = {};


    // tick is update
    // frame is animation keyframe


    MultiplayerScene multi_scene(input, &font);
    multi_scene.connect();



    while (1) {
        bool quit = false;

        auto this_frame_time = std::chrono::high_resolution_clock::now();
        double delta_time = std::chrono::duration_cast<std::chrono::duration<double>>(this_frame_time - last_frame_time).count();
        last_frame_time = this_frame_time;

        input.begin_frame();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                quit = true;
                break;
            }

            ImGui_ImplSDL3_ProcessEvent(&event);
            switch (event.type) {
                case SDL_EVENT_MOUSE_WHEEL:
                    input.wheel += event.wheel.y;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    input.keyboard_repeat[event.key.scancode] = true;

                    if (!event.key.repeat) {
                        input.keyboard_down[event.key.scancode] = true;
                    }
                    break;
                case SDL_EVENT_KEY_UP:
                    input.keyboard_up[event.key.scancode] = true;
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    input.button_down_flags |= SDL_BUTTON_MASK(event.button.button);
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    if (event.button.down) {
                        break;
                    }
                    input.button_up_flags |= SDL_BUTTON_MASK(event.button.button);
                default:
                    break;
            }
        }

        if (quit) {
            break;
        }

        multi_scene.update(delta_time);

        auto render_begin_time = std::chrono::high_resolution_clock::now();
        begin_rendering(renderer, window);
        multi_scene.render(renderer, window, textures);

        auto string = std::format("render: {:.3f}ms", last_render_duration * 1000.0);
        font::draw_text(renderer, font, string.data(), 24, {20, 30});
        end_rendering(renderer);

        last_render_duration = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - render_begin_time).count();

        input.end_frame();
    }

    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}

} // namespace app
