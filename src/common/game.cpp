#include "game.h"

void update(State state, eastl::fixed_vector<PlayerInput, 16> inputs, float dt) {
    for (int i = 0; i < state.players.size(); i++) {
        auto input = inputs[i];

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
        if (glm::length(move_input) > 0) {
            state.players[i] += glm::normalize(move_input) * (float)dt * 4.0f;
        }
    }
}
