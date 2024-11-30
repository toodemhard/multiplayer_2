#include "game.h"

void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line) {
    return malloc(size);
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line) {
    return malloc(size);
}

void update(State& state, eastl::fixed_vector<PlayerInput, 16>& inputs, double time, double dt) {
    auto& bullets = state.bullets;
    // for (int i = 0; i < bullets_capacity; i++) {
    //
    //
    // }

    for (int i = 0; i < bullets_capacity; i++) {
        if (time - 1.0 > bullets.create_time[i]) {
            bullets.alive[i] = false;
        }
    }
    
    for (int player_index = 0; player_index < inputs.size(); player_index++) {
        auto input = inputs[player_index];
        glm::vec2 move_input{};
        if (input.up) {
            move_input.y += 1;
        }
        if (input.down) {
            move_input.y -= 1;
        }
        if (input.left) {
            move_input.x -= 1;
        }
        if (input.right) {
            move_input.x += 1;
        }

        // move_input.x += 1;
        if (glm::length(move_input) > 0) {
            state.players[player_index] += glm::normalize(move_input) * (float)dt * 4.0f;
        }

        if (input.fire) {
            printf("fire input\n");

            for (int bullet_index = 0; bullet_index < bullets_capacity; bullet_index++) {
                if (!state.bullets.alive[bullet_index]) {
                    state.bullets.alive[bullet_index] = true;
                    state.bullets.position[bullet_index] = state.players[player_index];
                    bullets.create_time[bullet_index] = time;
                    break;
                }

            }

        }
    }
}
