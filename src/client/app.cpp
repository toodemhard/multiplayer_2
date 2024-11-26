#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "client.h"
#include "codegen/assets.h"
#include "codegen/shaders.h"
#include <SDL3/SDL.h>

#include <chrono>
#include <glm/glm.hpp>
#include <iostream>
#include <optional>

#include "color.h"
#include "font.h"
#include "glm/ext/quaternion_geometric.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "renderer.h"
#include "stb_image.h"

#include "input.h"

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

    Font font;
    font::load_font(&font, renderer, assets::Avenir_LT_Std_95_Black_ttf, 512, 512, 64);

    Texture amogus_texture;
    {
        int width, height, comp;
        unsigned char* image = stbi_load(assets::amogus_png, &width, &height, &comp, STBI_rgb_alpha);
        amogus_texture = load_texture(renderer, Image{.w = width, .h = height, .data = image});
    }

    Texture pot_texture;
    {
        int width, height, comp;
        unsigned char* image = stbi_load(assets::pot_jpg, &width, &height, &comp, STBI_rgb_alpha);
        pot_texture = load_texture(renderer, Image{.w = width, .h = height, .data = image});
    }

    Input::Input input;
    input.init_keybinds(Input::default_keybindings);

    glm::vec2 player_position(0, 0);

    Camera2D camera{
        {0, 0},
        {1 * 4.0f/3.0f, 1},
    };
    camera.scale *= 5.0f;

    auto last_frame_time = std::chrono::high_resolution_clock::now();

    InitializeYojimbo();
    OnlineGameScreen client(yojimbo::Address(127, 0, 0, 1, 5000));

    while (1) {
        auto this_frame_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = (this_frame_time - last_frame_time);
        double delta_time = duration.count();
        bool quit = false;

        input.begin_frame();

        SDL_Event event;
        float total_wheel{};

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                quit = true;
                break;
            }

            switch (event.type) {
            case SDL_EVENT_MOUSE_WHEEL:
                total_wheel += event.wheel.y;
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
            default:
                break;
            }

            input.wheel = total_wheel;
        }

        if (quit) {
            break;
        }

        client.Update(delta_time, input);

        begin_rendering(renderer, window);

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

        glm::vec2 move_input{};
        if (input.action_held(ActionID::move_up)) {
            move_input.y += 1;
        }
        if (input.action_held(ActionID::move_down)) {
            move_input.y -= 1;
        }
        if (input.action_held(ActionID::move_left)) {
            move_input.x -= 1;
        }
        if (input.action_held(ActionID::move_right)) {
            move_input.x += 1;
        }
        if (glm::length(move_input) > 0) {
            player_position += glm::normalize(move_input) * (float)delta_time * 4.0f;
        }

        draw_textured_rect(renderer, &src, rect, pot_texture);
        draw_wire_rect(renderer, rect_2, {0, 0, 255, 255});
        draw_textured_rect(renderer, nullptr, {{200, 300}, {200, 200}}, amogus_texture);

        draw_world_textured_rect(renderer, camera, nullptr, {player_position, {1, 1}}, amogus_texture);

        auto string = std::format("{} {}", player_position.x, player_position.y);
        font::draw_text(renderer, font, string.data(), 64, {20, 30});


        end_rendering(renderer);

        input.end_frame();

        last_frame_time = this_frame_time;
    }

    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}

} // namespace app
