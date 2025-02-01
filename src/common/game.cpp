#include "../pch.h"

#include "game.h"
#include "panic.h"

const static int jhshfk = 31231;

void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line) {
    return malloc(size);
}

void* operator new[](
    size_t size,
    size_t alignment,
    size_t alignmentOffset,
    const char* pName,
    int flags,
    unsigned debugFlags,
    const char* file,
    int line
) {
    return malloc(size);
}

glm::vec2 b2vec_to_glmvec(b2Vec2 vec) {
    return glm::vec2{vec.x, vec.y};
}

b2Vec2 glmvec_to_b2vec(glm::vec2 vec) {
    return b2Vec2{vec.x, vec.y};
}

const static float bullet_speed = 10.0f;
const static int substep_count = 4;



void create_box(State& state, glm::vec2 position) {
    for (int i = 0; i < boxes_capacity; i++) {
        if (state.boxes_active[i] == false) {
            auto& box = state.boxes[i];

            auto body_def = b2DefaultBodyDef();
            body_def.type = b2_dynamicBody;
            body_def.position = glmvec_to_b2vec(position);
            body_def.linearDamping = 4;
            body_def.fixedRotation = true;
            auto body_id = b2CreateBody(state.world_id, &body_def);

            auto polygon = b2MakeBox(0.5, 0.5);
            auto shape_def = b2DefaultShapeDef();
            shape_def.userData = &box.sensor_ref;
            b2CreatePolygonShape(body_id, &shape_def, &polygon);

            state.boxes_active[i] = true;
            state.boxes[i] = {
                .sensor_ref =
                    {
                        .type = EntityType::Box,
                        .index = i,
                    },
                .body_id = body_id,
                .health = box_health,
            };

            return;
        }
    }
}

void destroy_box(State& state, int index) {
    b2DestroyBody(state.boxes[index].body_id);
    state.boxes_active[index] = false;
}

void init_state(State& state) {
    b2WorldDef world_def = b2DefaultWorldDef();
    world_def.gravity = b2Vec2{0.0f, 0.0f};
    state.world_id = b2CreateWorld(&world_def);

    {
        // auto body_def = b2DefaultBodyDef();
        // body_def.position = {0, -2};
        //
        // auto body_id = b2CreateBody(state.world_id, &body_def);
        //
        // auto polygon = b2MakeBox(8, 2);
        // auto shape_def = b2DefaultShapeDef();
        //
        // b2CreatePolygonShape(body_id, &shape_def, &polygon);
    }

    auto ground_body_def = b2DefaultBodyDef();
    state.ground_id = b2CreateBody(state.world_id, &ground_body_def);

    auto ground_box = b2MakeBox(2.0, 0.5);
    auto ground_shape_def = b2DefaultShapeDef();
    b2CreatePolygonShape(state.ground_id, &ground_shape_def, &ground_box);

    {
        create_box(state, {1, 1});
    }

    // auto body_def = b2DefaultBodyDef();
    // body_def.type = b2_dynamicBody;
    // body_def.position = b2Vec2{0.0, 0.0};
    //
    // state.body_id = b2CreateBody(state.world_id, &body_def);

    // auto dynamic_box = b2MakeBox(0.5f, 0.5f);
    // auto shape_def = b2DefaultShapeDef();
    // shape_def.density = 1.0;
    // shape_def.friction = 0.3;

    // b2CreatePolygonShape(state.body_id, &shape_def, &dynamic_box);
    //
    // b2Body_SetGravityScale(state.body_id, 0.0f);
}

void remove_bullet(State& state, int index) {
    state.bullets_active[index] = false;
    b2DestroyBody(state.bullets[index].body_id);
}

const float player_speed = 6;
const float dash_speed = 20;
const float dash_distance = 3;

const float dash_duration = dash_distance / dash_speed;

const float bullet_damage = 10;

// dt is duration between ticks
void update_state(State& state, PlayerInput inputs[max_player_count], u32 current_tick, double dt) {
    b2World_Step(state.world_id, dt, substep_count);

    b2SensorEvents sensorEvents = b2World_GetSensorEvents(state.world_id);
    for (int i = 0; i < sensorEvents.beginCount; ++i) {
        b2SensorBeginTouchEvent* beginTouch = sensorEvents.beginEvents + i;
        auto sensor_ref = (EntityRef*)b2Shape_GetUserData(beginTouch->sensorShapeId);
        auto visitor_ref = (EntityRef*)b2Shape_GetUserData(beginTouch->visitorShapeId);

        if (visitor_ref != NULL && visitor_ref->type == EntityType::Box) {
            auto& box = state.boxes[visitor_ref->index];
            auto& bullet = state.bullets[sensor_ref->index];
            auto inc_dir = glm::normalize(b2vec_to_glmvec(b2Body_GetLinearVelocity(bullet.body_id)));
            b2Body_ApplyLinearImpulse(box.body_id, glmvec_to_b2vec(inc_dir * 2.0f), b2Body_GetWorldCenterOfMass(box.body_id), true);

            box.last_hit_tick = current_tick;

            remove_bullet(state, sensor_ref->index);

            box.health -= bullet_damage;

            if (box.health <= 0) {
                destroy_box(state, visitor_ref->index);
            }
        }
        //      if (ref != NULL) {
        // switch (((SensorRef*)ref)->type) {
        // 	case EntityType::Bullet: {
        // 		//remove_bullet(state, ref.index);
        // 	}
        // 	break;
        // 	case EntityType::Box: {
        // 		//remove_bullet(state, ref.index);
        // 	}
        // 	break;
        // }
        //      }

        // process begin event
    }

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

        if ((current_tick - bullet.create_tick) * dt > 1) {
            remove_bullet(state, i);
        }
        // else {
        //     bullet.position += bullet.direction_normalized * bullet_speed * (float)dt;
        // }
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

        auto velocity = glm::vec2{0, 0};
        // move_input.x += 1;

        if (player.state == PlayerState::Neutral) {
            auto move_direction = glm::normalize(move_input);

            if (input.dash) {
                player.state = PlayerState::Dashing;
                player.dash_direction = move_direction;
                player.dash_start_tick = current_tick;
            }


            if (glm::length(move_input) > 0) {
                velocity = move_direction * player_speed;
                // printf("%f, %f\n", velocity.x, velocity.y);
            }
            b2Body_SetLinearVelocity(player.body_id, b2Vec2{velocity.x, velocity.y});
        }

        if (player.state == PlayerState::Dashing) {
            velocity = player.dash_direction * dash_speed;
            b2Body_SetLinearVelocity(player.body_id, b2Vec2{velocity.x, velocity.y});

            if ((current_tick - player.dash_start_tick) * dt > dash_duration) {
                player.state = PlayerState::Neutral;
            }
        }

        auto player_pos = b2vec_to_glmvec(b2Body_GetPosition(player.body_id));

        auto count = 0;
        for (int bullet_index = 0; bullet_index < bullets_capacity; bullet_index++) {
            if (state.bullets_active[bullet_index]) {
                count++;
            }
        }
        // printf("%d\n", count);

        if (input.fire) {
            for (int bullet_index = 0; bullet_index < bullets_capacity; bullet_index++) {
                if (!state.bullets_active[bullet_index]) {
                    auto body_def = b2DefaultBodyDef();
                    body_def.type = b2_dynamicBody;
                    body_def.position = glmvec_to_b2vec(player_pos);
                    body_def.linearVelocity = glmvec_to_b2vec(glm::normalize(input.cursor_world_pos - player_pos) * bullet_speed);
                    body_def.fixedRotation = true;

                    auto body_id = b2CreateBody(state.world_id, &body_def);

                    auto polygon = b2MakeBox(0.25, 0.25);
                    auto shape_def = b2DefaultShapeDef();
                    shape_def.isSensor = true;
                    shape_def.friction = 0;
                    shape_def.userData = &state.bullets[bullet_index].sensor_ref;

                    auto shape_id = b2CreatePolygonShape(body_id, &shape_def, &polygon);

                    state.bullets_active[bullet_index] = true;
                    auto& bullet = state.bullets[bullet_index];

                    bullet = Bullet{
                        .sensor_ref =
                            {
                                .type = EntityType::Bullet,
                                .index = bullet_index,
                            },
                        .body_id = body_id,
                        .create_tick = current_tick,
                    };

                    void* stuff = b2Shape_GetUserData(shape_id);
                    // bullet.direction_normalized = glm::normalize(input.cursor_world_pos - player_pos);

                    break;
                }
            }
        }
    }
}

PlayerID create_player(State& state) {
    for (int i = 0; i < max_player_count; i++) {
        if (!state.players_active[i]) {
            state.players_active[i] = true;

            auto body_def = b2DefaultBodyDef();
            body_def.type = b2_dynamicBody;
            body_def.position = b2Vec2{0.0, 1.0};
            body_def.fixedRotation = true;

            auto body_id = b2CreateBody(state.world_id, &body_def);
            state.players[i] = {
                .body_id = body_id,
            };

            auto polygon = b2MakeBox(0.5, 0.5);
            auto shape_def = b2DefaultShapeDef();
            shape_def.friction = 0;

            b2CreatePolygonShape(body_id, &shape_def, &polygon);

            return i;
        }
    }

    panic("players array exceeded\n");
}

void remove_player(State& state, int player_index) {
    state.players_active[player_index] = false;
}
