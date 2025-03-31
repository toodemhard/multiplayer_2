// durations in seconds 
// multiply by tick rate to get duration in ticks

const float bullet_speed = 10.0f;
const float bullet_expire_duration = 2;
const int substep_count = 4;
const float hit_flash_duration = 0.1;

typedef enum Dir8 {
    Dir8_Up,
    Dir8_UpRight,
    Dir8_Right,
    Dir8_DownRight,
    Dir8_Down,
    Dir8_DownLeft,
    Dir8_Left,
    Dir8_UpLeft,
} Dir8;

bool entity_handle_equals(EntityHandle a, EntityHandle b) {
    return a.index == b.index && a.generation == b.generation;
}

EntityHandle entity_ptr_to_handle(Slice_Entity ent_list, Entity* ent_ptr) {
    return (EntityHandle){ent_ptr - ent_list.data, ent_ptr->generation};
}

EntityIndex entity_ptr_to_index(Slice_Entity ent_list, Entity* ent_ptr) {
    return (ent_ptr - ent_list.data);
}

bool entity_is_valid(Slice_Entity entity_list, EntityHandle handle) {
    if (!entity_list.data[handle.index].is_active) {
        return false;
    }

    if (entity_list.data[handle.index].generation != handle.generation) {
        return false;
    }

    return true;
}

Entity* entity_list_get(Slice_Entity entity_list, EntityHandle handle) {
    Entity* ret = NULL;

    Entity* ent = slice_getp(entity_list, handle.index);
    if (ent->generation ==  handle.generation) {
        ret = ent;
    }

    return ret;
}

EntityHandle entity_list_add(Slice_Entity* entity_list, Entity entity) {
    entity.is_active = true;

    EntityIndex index;
    bool found_hole = false;
    for (u32 i = 0; i < entity_list->length; i++) {
        Entity* existing_ent = slice_getp(*entity_list, i);
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

    return (EntityHandle) {
        .index = index,
        .generation = slice_get(*entity_list, index).generation,
    };
}


void create_box(GameState* state, float2 position) {
    b2BodyDef body_def = b2DefaultBodyDef();
    body_def.type = b2_dynamicBody;
    body_def.position = position.b2vec;
    body_def.linearDamping = 4;
    body_def.fixedRotation = true;
    b2BodyId body_id = b2CreateBody(state->world_id, &body_def);

    Entity box = {
        .sprite = TextureID_box_png,
        .type = EntityType_Box,
        .flags = EntityFlags_hittable,
        .body_id = body_id,
        .health = box_health,
    };

    EntityHandle ent_handle = entity_list_add(&state->entities, box);

    b2Polygon polygon = b2MakeBox(0.5, 0.5);
    b2ShapeDef shape_def = b2DefaultShapeDef();
    shape_def.userData = slice_getp(state->entities, ent_handle.index);
    b2CreatePolygonShape(body_id, &shape_def, &polygon);
}

void create_wall(GameState* state, float2 position, Dir8 direction) {
    b2BodyDef body_def = b2DefaultBodyDef();
    // body_def.type = b2_staticBody;
    body_def.position = position.b2vec;
    // body_def.fixedRotation = true;

    b2BodyId body_id = b2CreateBody(state->world_id, &body_def);

    Rect src = {0,0,64,64};
    bool flip_sprite = false;
    if (direction == Dir8_Up || direction == Dir8_Down) {
        src.position = (float2){0,64};
    }
    if (direction == Dir8_UpLeft || direction == Dir8_UpRight || direction == Dir8_DownRight || direction == Dir8_DownLeft) {
        src.position = (float2){64,0};
    }
    if (direction == Dir8_UpLeft || direction == Dir8_DownRight) {
        flip_sprite = true;
    }

    Entity wall = {
        .sprite = TextureID_ice_wall_png,
        .sprite_src = src,
        .flip_sprite = flip_sprite,
        .flags = EntityFlags_hittable,
        .body_id = body_id,
        .health = 50,
    };

    EntityHandle handle = entity_list_add(&state->entities, wall);

    b2ShapeDef shape_def = b2DefaultShapeDef();
    // shape_def.isSensor = true;
    // shape_def.friction = 0;
    // shape_def.userData = &(state->entities[handle.index]);
    b2Polygon polygon = b2MakeBox(0.25, 0.25);

    b2ShapeId shape_id = b2CreatePolygonShape(body_id, &shape_def, &polygon);
}

void create_bullet(GameState* state, EntityHandle owner, float2 position, float2 direction, u32 current_tick, i32 tick_rate) {
    b2BodyDef body_def = b2DefaultBodyDef();
    body_def.type = b2_dynamicBody;
    body_def.position = position.b2vec;
    body_def.linearVelocity = float2_scale(direction, bullet_speed).b2vec;
    body_def.fixedRotation = true;

    b2BodyId body_id = b2CreateBody(state->world_id, &body_def);

    Entity bullet = {
        .sprite = TextureID_bullet_png,
        .type = EntityType_Bullet,
        .flags = EntityFlags_attack | EntityFlags_expires,
        .body_id = body_id,
        .expire_tick = current_tick + (u32)(bullet_expire_duration * tick_rate),
        .owner = owner,
    };

    EntityHandle handle = entity_list_add(&state->entities, bullet);

    b2ShapeDef shape_def = b2DefaultShapeDef();
    shape_def.isSensor = true;
    shape_def.friction = 0;
    shape_def.userData = slice_getp(state->entities, handle.index);
    b2Polygon polygon = b2MakeBox(0.25, 0.25);

    b2ShapeId shape_id = b2CreatePolygonShape(body_id, &shape_def, &polygon);
}

void entity_list_remove(Slice_Entity* entity_list, EntityIndex index) {
    Entity* ent = slice_getp(*entity_list, index);
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
    world_def.gravity = (b2Vec2){0.0f, 0.0f};
    state->world_id = b2CreateWorld(&world_def);

    // b2BodyDef groundBodyDef = b2DefaultBodyDef();
    // groundBodyDef.position = (b2Vec2){0.0f, -10.0f};
    // b2BodyId groundId = b2CreateBody(state->world_id, &groundBodyDef);
    // b2Polygon groundBox = b2MakeBox(50.0f, 10.0f);
    // b2ShapeDef groundShapeDef = b2DefaultShapeDef();
    // b2CreatePolygonShape(groundId, &groundShapeDef, &groundBox);

    b2World_Step(state->world_id, 0.1, 4);

    {
        create_box(state, (float2){1, 1});
    }
}

const float player_speed = 6;
const float dash_speed = 20;
const float dash_distance = 3;

#define dash_duration (dash_distance / dash_speed)

const float bullet_damage = 10;

float2 rotate(float2 vector, f32 angle) {
    float2x2 rotation_matrix = {
        (f32)cos(angle), (f32)-sin(angle),
        (f32)sin(angle), (f32)cos(angle)
    };

    return float2x2_mult_float2(rotation_matrix, vector);
}

float lerp(float v0, float v1, float t) {
    return (1 - t) * v0 + t * v1;
}

const double pi = 3.141592653589793238462643383279502884197;
#define deg_in_rads  (pi / 180);

float deg_to_rad(float deg) {
    return deg * deg_in_rads;
}

void state_update(GameState* state, Arena* temp_arena, Inputs inputs, u32 current_tick, i32 tick_rate) {
    const f64 dt = 1.0 / tick_rate;

    b2World_Step(state->world_id, dt, substep_count);

    Slice_EntityIndex delete_list = slice_create(EntityIndex, temp_arena, 512);

    b2SensorEvents sensorEvents = b2World_GetSensorEvents(state->world_id);
    for (int i = 0; i < sensorEvents.beginCount; ++i) {
        b2SensorBeginTouchEvent* beginTouch = sensorEvents.beginEvents + i;
        Entity* sensor = (Entity*)b2Shape_GetUserData(beginTouch->sensorShapeId);
        Entity* visitor = (Entity*)b2Shape_GetUserData(beginTouch->visitorShapeId);

        if (visitor == NULL) {
            continue;
        }

        bool skip_owner = !(sensor->flags & EntityFlags_damage_owner) && entity_handle_equals(sensor->owner, entity_ptr_to_handle(state->entities, visitor));

        if (!skip_owner && visitor != NULL && visitor->flags & EntityFlags_hittable) {
            float2 inc_dir = normalize((float2){.b2vec=b2Body_GetLinearVelocity(sensor->body_id)});
            b2Body_ApplyLinearImpulse(visitor->body_id, float2_scale(inc_dir, 2.0f).b2vec, b2Body_GetWorldCenterOfMass(visitor->body_id), true);

            visitor->hit_flash_end_tick = current_tick + hit_flash_duration * tick_rate;

            slice_push(&delete_list, entity_ptr_to_index(state->entities, sensor));
            // entity_list_remove(&state->entities, entity_ptr_to_index(&state->entities, sensor));

            visitor->health -= bullet_damage;
        }
    }


    for (u32 i = 0; i < state->entities.length; i++) {
        Entity* ent = &state->entities.data[i];

        if (!ent->is_active) {
            continue;
        }

        if (ent->flags & EntityFlags_expires && current_tick >= ent->expire_tick) {
            slice_push(&delete_list, i);
        }

        if (ent->flags & EntityFlags_hittable && ent->health <= 0) {
            slice_push(&delete_list, i);
        }

        if (ent->type == EntityType_Player) {
            const PlayerInput* input = NULL;
            for (u32 input_idx = 0; input_idx < inputs.ids.length; input_idx++){
                if (slice_get(inputs.ids, input_idx) == ent->client_id) {
                    input = slice_getp(inputs.inputs, input_idx);
                    break;
                }
            }
            PlayerInput idk = {0};

            if (!input) {
                input = &idk;
            }

            float2 move_input = {0};
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

            for (i32 i = 0; i < array_length(input->select_spell); i++) {
                if (input->select_spell[i]) {
                    ent->selected_spell = i;
                }
            }

            if (input->move_spell_dst != input->move_spell_src) {
                SpellType b = ent->hotbar[input->move_spell_dst];
                ent->hotbar[input->move_spell_dst] = ent->hotbar[input->move_spell_src];
                ent->hotbar[input->move_spell_src] = b;
            }


            float2 velocity = {0, 0};
            // move_input.x += 1;

            // auto print_vec2 = [current_tick](float2 vec) {
            //     // printf("%d, %f, %f\n", current_tick, vec.x, vec.y);
            // };

            if (ent->player_state == PlayerState_Neutral) {
                if (move_input.x == 0 && move_input.y == 0) {

                }
                if (magnitude(move_input) > 0) {
                    float2 move_direction = normalize(move_input);
                    velocity = float2_scale(move_direction, player_speed);
                    // printf("%f, %f\n", velocity.x, velocity.y);

                    if (input->dash) {
                        ent->player_state = PlayerState_Dashing;
                        ent->dash_direction = move_direction;
                        ent->dash_end_tick = current_tick + dash_duration * tick_rate;
                    }
                }

                b2Body_SetLinearVelocity(ent->body_id, (b2Vec2){velocity.x, velocity.y});
            } else if (ent->player_state == PlayerState_Dashing) {
                velocity = float2_scale(ent->dash_direction, dash_speed);

                b2Body_SetLinearVelocity(ent->body_id, (b2Vec2){velocity.x, velocity.y});

                if (current_tick >= ent->dash_end_tick) {
                    ent->player_state = PlayerState_Neutral;
                }
            }

            EntityHandle handle = {i, ent->generation};

            if (input->fire) {
                float2 player_pos = {.b2vec=b2Body_GetPosition(ent->body_id)};
                float2 direction = {1,0};
                if (magnitude(float2_sub(input->cursor_world_pos, player_pos)) > 0) {
                    direction = normalize(float2_sub(input->cursor_world_pos, player_pos));
                }

                SpellType selected_spell = SpellType_NULL;
                if (ent->selected_spell >= 0 && ent->selected_spell < hotbar_length) {
                    selected_spell = ent->hotbar[ent->selected_spell];
                }

                if (selected_spell == SpellType_Fireball) {
                    create_bullet(state, handle, player_pos, direction, current_tick, tick_rate);
                }

                if (selected_spell == SpellType_SpreadBolt) {

                    i32 projectile_count = 4;

                    f32 range_start = deg_to_rad(-20);
                    f32 range_end = deg_to_rad(20);

                    for (i32 i = 0; i < projectile_count; i++) {
                        f32 dir_offset = lerp(range_start, range_end, (f32)i / (projectile_count - 1));
                        create_bullet(state, handle, player_pos, rotate(direction, dir_offset), current_tick, tick_rate);
                    }
                }

                if (selected_spell == SpellType_IceWall) {
                    f32 d = 1 - fmod(atan2f(direction.y, direction.x) / (2*pi) + 0.5 + 0.1875, 1);
                    // printf("%f %f\n", direction.x, direction.y);
                    printf("%f\n", d);
                    // i32 idk = (1 - fmod(d + 0.5, 1)) * 8;
                    Dir8 dir = (Dir8) (d * 8);
                    printf("%d\n", dir);
                    // i32 x = (pi - d - 1/8.0 * pi) / (2*pi) * 8;
                    // printf("%x\n", x);
                    create_wall(state, float2_add(player_pos, float2_scale(direction, 2)), dir);
                }

                // if (selected)
            }
        }
    }
    for (u32 i = 0; i < delete_list.length; i++) {
        entity_list_remove(&state->entities, slice_get(delete_list, i));
    }
}

EntityHandle create_player(GameState* state, ClientID client_id) {
    b2BodyDef body_def = b2DefaultBodyDef();
    body_def.type = b2_dynamicBody;
    body_def.position = (b2Vec2){0.0, 1.0};
    body_def.fixedRotation = true;
    b2BodyId body_id = b2CreateBody(state->world_id, &body_def);
    
    Entity player = {
        .sprite = TextureID_player_png,
        .type = EntityType_Player,
        .flags = EntityFlags_hittable,
        .body_id = body_id,
        .hotbar = {SpellType_Fireball, SpellType_SpreadBolt, SpellType_IceWall, SpellType_SniperRifle},
        .client_id = client_id,
        // .hotbar = {SpellType_Bolt, SpellType_SpreadBolt},
        .health = 100,
    };

    EntityHandle handle = entity_list_add(&state->entities, player);

    b2Polygon polygon = b2MakeBox(0.5, 0.5);
    b2ShapeDef shape_def = b2DefaultShapeDef();
    shape_def.friction = 0;
    shape_def.userData = slice_getp(state->entities, handle.index);

    b2CreatePolygonShape(body_id, &shape_def, &polygon);

    return handle;
}

Slice_Ghost entities_to_snapshot(Arena* tick_arena, const Slice_Entity ents, u64 current_tick, Slice_pEntity* players) {
    Slice_Ghost ghosts = slice_create(Ghost, tick_arena, ents.length);

    for (i32 i = 0; i < ents.length; i++) {
        Entity* ent = slice_getp(ents, i);
        if (ent->is_active) {
            slice_push(&ghosts, (Ghost){
                .sprite = ent->sprite,
                .sprite_src = ent->sprite_src,
                .flip_sprite = ent->flip_sprite,
                .position = {.b2vec=b2Body_GetPosition(ent->body_id)},
                .health = (u16)ent->health,
                .show_health = (bool)(ent->flags & EntityFlags_hittable),
                .hit_flash = ent->hit_flash_end_tick > current_tick,
            });

            if (ent->type == EntityType_Player) {
                ent->position.b2vec = b2Body_GetPosition(ent->body_id);
                slice_push(players, ent);
            }
        }
    }

    return ghosts;
}
