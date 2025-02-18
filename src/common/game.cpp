#include "../pch.h"

#include "game.h"
#include "panic.h"

glm::vec2 b2vec_to_glmvec(b2Vec2 vec) {
    return glm::vec2{vec.x, vec.y};
}

b2Vec2 glmvec_to_b2vec(glm::vec2 vec) {
    return b2Vec2{vec.x, vec.y};
}

// durations in seconds 
// multiply by tick rate to get duration in ticks

const static float bullet_speed = 10.0f;
const static float bullet_expire_duration = 2;
const static int substep_count = 4;
const static float hit_flash_duration = 0.1;


EntityIndex entity_ptr_to_index(const Slice<Entity>* ent_list, Entity* ent_ptr) {
    return (ent_ptr - ent_list->data);
}

bool entity_is_valid(const Slice<Entity>* entity_list, EntityHandle handle) {
    if (entity_list->data[handle.index].generation == handle.generation) {
        return true;
    }

    return false;
}

Entity* entity_list_get(const Slice<Entity>* entity_list, EntityHandle handle) {
    Entity* ret = NULL;

    Entity* ent = &(*entity_list)[handle.index];
    if (ent->generation ==  handle.generation) {
        ret = ent;
    }

    return ret;
}

EntityHandle entity_list_add(Slice<Entity>* entity_list, Entity entity) {
    entity.is_active = true;

    EntityIndex index;
    bool found_hole = false;
    for (u32 i = 0; i < entity_list->length; i++) {
        Entity* existing_ent = &(*entity_list)[i];
        if (existing_ent->is_active == false) {
            entity.generation = existing_ent->generation + 1;
            *existing_ent = entity;
            found_hole = true;
            index = i;
            break;
        }
    }

    if (!found_hole) {
        index = entity_list->length;
        entity.generation = 0;
        slice_push(entity_list, entity);
    }

    return EntityHandle{
        .index = index,
        .generation = (*entity_list)[index].generation,
    };
}


void create_box(GameState* state, glm::vec2 position) {
    auto body_def = b2DefaultBodyDef();
    body_def.type = b2_dynamicBody;
    body_def.position = glmvec_to_b2vec(position);
    body_def.linearDamping = 4;
    body_def.fixedRotation = true;
    auto body_id = b2CreateBody(state->world_id, &body_def);

    Entity box = {
        .entity_type = EntityType::Box,
        .flags = etbf(EntityComponent::hittable),
        .body_id = body_id,
        .health = box_health,
    };

    EntityHandle ent_handle = entity_list_add(&state->entities, box);

    auto polygon = b2MakeBox(0.5, 0.5);
    auto shape_def = b2DefaultShapeDef();
    shape_def.userData = &slice_get(&state->entities, ent_handle.index);
    b2CreatePolygonShape(body_id, &shape_def, &polygon);
}

void create_bullet(GameState* state, glm::vec2 position, glm::vec2 direction, u32 current_tick, i32 tick_rate) {
    auto body_def = b2DefaultBodyDef();
    body_def.type = b2_dynamicBody;
    body_def.position = glmvec_to_b2vec(position);
    body_def.linearVelocity = glmvec_to_b2vec(direction * bullet_speed);
    body_def.fixedRotation = true;

    auto body_id = b2CreateBody(state->world_id, &body_def);
    auto polygon = b2MakeBox(0.25, 0.25);

    Entity bullet = {
        .entity_type = EntityType::Bullet,
        .flags = etbf(EntityComponent::attack) | etbf(EntityComponent::expires),
        .body_id = body_id,
        .expire_tick = current_tick + (u32)(bullet_expire_duration * tick_rate),
    };

    EntityHandle handle = entity_list_add(&state->entities, bullet);

    auto shape_def = b2DefaultShapeDef();
    shape_def.isSensor = true;
    shape_def.friction = 0;
    shape_def.userData = &(state->entities[handle.index]);

    auto shape_id = b2CreatePolygonShape(body_id, &shape_def, &polygon);
}

void entity_list_remove(Slice<Entity>* entity_list, EntityIndex index) {
    Entity* ent = &(*entity_list)[index];
    if (ent->is_active) {
        ent->is_active = false;

        if (b2Body_IsValid(ent->body_id)) {
            b2DestroyBody(ent->body_id);
        }
    }
}


void state_init(GameState* state, Arena* arena) {
    slice_init(&state->entities, arena, 512);

    b2WorldDef world_def = b2DefaultWorldDef();
    world_def.gravity = b2Vec2{0.0f, 0.0f};
    state->world_id = b2CreateWorld(&world_def);

    auto ground_body_def = b2DefaultBodyDef();
    ground_body_def.type = b2_staticBody;
    state->ground_id = b2CreateBody(state->world_id, &ground_body_def);

    auto ground_box = b2MakeBox(2.0, 0.5);
    auto ground_shape_def = b2DefaultShapeDef();
    b2CreatePolygonShape(state->ground_id, &ground_shape_def, &ground_box);

    b2World_Step(state->world_id, 0.1, 4);

    {
        create_box(state, {1, 1});
    }
}

const float player_speed = 6;
const float dash_speed = 20;
const float dash_distance = 3;

const float dash_duration = dash_distance / dash_speed;

const float bullet_damage = 10;

void state_update(GameState* state, Arena* temp_arena, PlayerInput inputs[max_player_count], u32 current_tick, i32 tick_rate) {
    const f64 dt = 1.0 / tick_rate;

    b2World_Step(state->world_id, dt, substep_count);

    b2SensorEvents sensorEvents = b2World_GetSensorEvents(state->world_id);
    for (int i = 0; i < sensorEvents.beginCount; ++i) {
        b2SensorBeginTouchEvent* beginTouch = sensorEvents.beginEvents + i;
        auto sensor = (Entity*)b2Shape_GetUserData(beginTouch->sensorShapeId);
        auto visitor = (Entity*)b2Shape_GetUserData(beginTouch->visitorShapeId);

        if (visitor != NULL && visitor->flags & etbf(EntityComponent::hittable)) {
            auto inc_dir = glm::normalize(b2vec_to_glmvec(b2Body_GetLinearVelocity(sensor->body_id)));
            b2Body_ApplyLinearImpulse(visitor->body_id, glmvec_to_b2vec(inc_dir * 2.0f), b2Body_GetWorldCenterOfMass(visitor->body_id), true);

            visitor->hit_flash_end_tick = current_tick + hit_flash_duration * tick_rate;

            entity_list_remove(&state->entities, entity_ptr_to_index(&state->entities, sensor));

            visitor->health -= bullet_damage;
        }
    }

    Slice<EntityIndex> delete_list = slice_create<EntityIndex>(temp_arena, 512);

    for (u32 i = 0; i < state->entities.length; i++) {
        Entity* ent = &state->entities.data[i];

        if (!ent->is_active) {
            continue;
        }

        if (ent->flags & etbf(EntityComponent::expires) && current_tick >= ent->expire_tick) {
            slice_push(&delete_list, i);
        }

        if (ent->flags & etbf(EntityComponent::hittable) && ent->health <= 0) {
            slice_push(&delete_list, i);
        }

        if (ent->entity_type == EntityType::Player) {
            const PlayerInput* input = &inputs[ent->player_id];
            glm::vec2 move_input{};
            if (input->up) {
                move_input.y += 1;
            }
            if (input->down) {
                move_input.y -= 1;
            }
            if (input->left) {
                move_input.x -= 1;
                ent->flip_sprite = false;
            }
            if (input->right) {
                move_input.x += 1;
                ent->flip_sprite = true;
            }

            auto velocity = glm::vec2{0, 0};
            // move_input.x += 1;

            // auto print_vec2 = [current_tick](glm::vec2 vec) {
            //     // printf("%d, %f, %f\n", current_tick, vec.x, vec.y);
            // };

            if (ent->player_state == PlayerState::Neutral) {
                if (move_input.x == 0 && move_input.y == 0) {

                }
                if (glm::length(move_input) > 0) {
                    auto move_direction = glm::normalize(move_input);
                    velocity = move_direction * player_speed;
                    // printf("%f, %f\n", velocity.x, velocity.y);

                    if (input->dash) {
                        ent->player_state = PlayerState::Dashing;
                        ent->dash_direction = move_direction;
                        ent->dash_end_tick = current_tick + dash_duration * tick_rate;
                    }
                }

                b2Body_SetLinearVelocity(ent->body_id, b2Vec2{velocity.x, velocity.y});
            } else if (ent->player_state == PlayerState::Dashing) {
                velocity = ent->dash_direction * dash_speed;

                b2Body_SetLinearVelocity(ent->body_id, b2Vec2{velocity.x, velocity.y});

                if (current_tick >= ent->dash_end_tick) {
                    ent->player_state = PlayerState::Neutral;
                }
            }

            if (input->fire) {
                glm::vec2 player_pos = b2vec_to_glmvec(b2Body_GetPosition(ent->body_id));
                glm::vec2 direction = {1,0};
                if (glm::length(input->cursor_world_pos - player_pos) > 0) {
                    direction = glm::normalize(input->cursor_world_pos - player_pos);
                }
                create_bullet(state, player_pos, direction, current_tick, tick_rate);
            }
        }
    }

    for (u32 i = 0; i < delete_list.length; i++) {
        entity_list_remove(&state->entities, delete_list[i]);
    }
}

EntityHandle create_player(GameState* state) {
    auto body_def = b2DefaultBodyDef();
    body_def.type = b2_dynamicBody;
    body_def.position = b2Vec2{0.0, 1.0};
    body_def.fixedRotation = true;
    auto body_id = b2CreateBody(state->world_id, &body_def);
    
    Entity player = {
        .entity_type = EntityType::Player,
        // .flags = etbf(hittable),
        .body_id = body_id,
        .health = 100,
    };

    EntityHandle handle = entity_list_add(&state->entities, player);

    auto polygon = b2MakeBox(0.5, 0.5);
    auto shape_def = b2DefaultShapeDef();
    shape_def.friction = 0;
    shape_def.userData = &state->entities[handle.index];

    b2CreatePolygonShape(body_id, &shape_def, &polygon);

    return handle;
}
