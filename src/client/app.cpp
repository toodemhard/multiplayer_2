#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_mouse.h"
#include "client.h"
#include "codegen/shaders.h"
#include <SDL3/SDL.h>

#include <chrono>
#include <future>
#include <glm/glm.hpp>
#include <iostream>
#include <numeric>
#include <optional>

#include "color.h"
#include "font.h"
#include "game.h"
#include "glm/ext/quaternion_geometric.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "renderer.h"
#include "stb_image.h"

#include "input.h"
#include "assets.h"

namespace app {

int run() {
    using namespace renderer;

    constexpr int window_width = 1024;
    constexpr int window_height = 768;

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

    Camera2D camera{
        {0, 0},
        {1 * 4.0f/3.0f, 1},
    };
    camera.scale *= 5.0f;


    InitializeYojimbo();
    GameClient client(yojimbo::Address(127, 0, 0, 1, 5000));


    for (int i = 0; i < (int)ImageID::count; i++) {
        texture_futures[i].wait();
    }
    font_future.wait();


    auto last_frame_time = std::chrono::high_resolution_clock::now();
    double accumulator = 0.0;
    uint32_t frame = {};
    double time = {};

    constexpr double fixed_dt = 1.0 / tick_rate;
    double speed_up_dt = 0.0;
    constexpr double speed_up_fraction = 0.05;

    while (1) {
        bool quit = false;

        auto this_frame_time = std::chrono::high_resolution_clock::now();
        double delta_time = std::chrono::duration_cast<std::chrono::duration<double>>(this_frame_time - last_frame_time).count();
        last_frame_time = this_frame_time;

        accumulator += delta_time;


        auto& pot_texture = textures[(int)ImageID::pot_jpg];
        auto& amogus_texture = textures[(int)ImageID::amogus_png];
        auto& bullet_texture = textures[(int)ImageID::bullet_png];

        auto src = Rect{.position{0, 0}, .scale{pot_texture.w, pot_texture.h}};
        auto src_2 = Rect{.position{0, 0}, .scale{amogus_texture.w, amogus_texture.h}};

        auto rect = Rect{
            {300, 300},
            {200, 200},
        };
        auto rect_2 = Rect{
            {300, 300},
            {200, 200},
        };
        using namespace Input;



        while (accumulator >= fixed_dt) {
            accumulator = accumulator - fixed_dt + speed_up_dt;
            frame++;
            time += fixed_dt;
            // printf("frame:%d %fs\n", frame, time);

            input.begin_frame();

            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    quit = true;
                    break;
                }
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

            glm::vec2 move_input{};

            if (input.key_down(SDL_SCANCODE_R)) {
                accumulator += fixed_dt;
            }

            if (input.key_down(SDL_SCANCODE_E)) {
                accumulator -= fixed_dt;
            }

            PlayerInput player_input{};
            if (input.action_held(ActionID::move_up)) {
                player_input.up = true;
            }
            if (input.action_held(ActionID::move_down)) {
                player_input.down = true;
            }
            if (input.action_held(ActionID::move_left)) {
                player_input.left = true;
            }
            if (input.action_held(ActionID::move_right)) {
                player_input.right = true;
            }
            // if (glm::length(move_input) > 0) {
            //     player_position += glm::normalize(move_input) * (float)delta_time * 4.0f;
            // }

            if (input.key_down(SDL_SCANCODE_F)) {
                printf("asdf\n");
            }

            if (input.mouse_down(SDL_BUTTON_LEFT) || input.key_down(SDL_SCANCODE_F)) {
                player_input.fire = true;
            }

            // printf("upate\n");

            int throttle_ticks{};
            client.Update(fixed_dt, player_input, input, &throttle_ticks);

            speed_up_dt = throttle_ticks * fixed_dt * speed_up_fraction;

            // accumulator += fixed_dt * throttle_ticks;

            input.end_frame();
        }

        if (quit) {
            break;
        }




        begin_rendering(renderer, window);

        draw_textured_rect(renderer, src, rect, pot_texture);
        draw_wire_rect(renderer, rect_2, {0, 0, 255, 255});
        draw_textured_rect(renderer, {}, {{200, 300}, {200, 200}}, amogus_texture);


        const auto& state = client.m_state;

        for (int i = 0; i < max_player_count; i++) {
            if (state.players.alive[i]) {
                draw_world_textured_rect(renderer, camera, {}, {state.players.position[i], {1, 1}}, amogus_texture);
            }
        }

        for (int i = 0; i < bullets_capacity; i++) {
            auto& bullets = state.bullets;

            if (!bullets.alive[i]) {
                continue;
            }

            draw_world_textured_rect(renderer, camera, {}, {{bullets.position[i]}, {1, 1}}, bullet_texture);
        }
        // draw_world_textured_rect(renderer, camera, nullptr, {client.m_pos, {1, 1}}, amogus_texture);


        auto string = std::format("{} {}", player_position.x, player_position.y);
        font::draw_text(renderer, font, string.data(), 64, {20, 30});


        end_rendering(renderer);
        // printf("render\n");


    }

    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}

} // namespace app
