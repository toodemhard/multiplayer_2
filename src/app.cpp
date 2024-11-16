#include <SDL3/SDL.h>
#include "SDL3/SDL_gpu.h"
#include "codegen/shaders.h"
#include "codegen/assets.h"

#include <glm/glm.hpp>
#include <optional>

#include "color.h"
#include "font.h"
#include "renderer.h"
#include "stb_image.h"

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
        if(init_renderer(&renderer, window) != 0) {
            return 1;
        }

        Font font;
        font::load_font(&font, renderer, assets::Avenir_LT_Std_95_Black_ttf, 512, 512, 64);




        int width, height, comp;
        unsigned char* image = stbi_load(assets::amogus_png, &width, &height, &comp, STBI_rgb_alpha);

        auto texture = load_texture(renderer, Image{.w=width,.h=height,.data=image});


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

            auto rect = Rect{
                {0, 0},
                {600, 600},
            };

            begin_rendering(renderer, window);


            auto src = Rect {
                .scale{width / 2, height}
            };
            draw_textured_rect(renderer, &src, rect, texture, {100, 255, 0});

            // font::draw_text(renderer, font, "sfhkjlashfk jlhsadljfh adf", 36, {20, 30});

            end_rendering(renderer);
        }

        SDL_DestroyWindow(window);

        SDL_Quit();

        return 0;
    }

} // namespace app
