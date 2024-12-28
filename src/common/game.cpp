#include "game.h"

void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line) {
    return malloc(size);
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line) {
    return malloc(size);
}

const static float bullet_speed = 10.0f;

void update_state(State& state, PlayerInput inputs[max_player_count], double time, double dt) {
    auto& bullets = state.bullets;
    // for (int i = 0; i < bullets_capacity; i++) {
    //
    //
    // }

    for (int i = 0; i < bullets_capacity; i++) {
        if (state.bullets_active[i] == false) {
            continue;
        }

        auto& bullet = state.bullets[i];

        if (time - 1.0 > bullet.create_time) {
            state.bullets_active[i] = false;
        } else {
            bullet.position += bullet.direction_normalized * bullet_speed * (float)dt;
        }
    }
    
    for (int player_index = 0; player_index < max_player_count; player_index++) {
        if (!state.players_active[player_index]) {
            continue;
        }

        auto& player = state.players[player_index];

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
            player.position += glm::normalize(move_input) * (float)dt * 4.0f;
        }

        if (input.fire) {
            for (int bullet_index = 0; bullet_index < bullets_capacity; bullet_index++) {
                if (!state.bullets_active[bullet_index]) {
                    state.bullets_active[bullet_index] = true;
                    auto& bullet = state.bullets[bullet_index];

                    bullet.position = player.position;
                    bullet.direction_normalized = glm::normalize(input.cursor_world_pos - player.position);
                    bullet.create_time = time;

                    break;
                }
            }
        }
    }
}

void create_player(State& state) {
    for (int i = 0; i < max_player_count; i++) {
        if (!state.players_active[i]) {
            state.players_active[i] = true;
            state.players[i] = {};
            return;
        }
    }
}

void remove_player(State& state, int player_index) {
    state.players_active[player_index] = false;
}
