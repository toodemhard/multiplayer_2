#define DefaultCamera \
((Camera2D) {\
    {0, 0},\
    float2_scale((float2){1 * 4.0f/3.0f, 1}, 14.0f),\
})

const float bullet_speed = 10.0f;
const float bullet_expire_duration = 2;
const int substep_count = 4;
const float hit_flash_duration = 0.1;

const float bullet_damage = 10;

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

EntityHandle entity_ptr_to_handle(Entity* ent_ptr) {
    return (EntityHandle){ent_ptr->index , ent_ptr->generation};
}

EntityIndex entity_ptr_to_index(Slice_Entity ent_list, Entity* ent_ptr) {
    return (ent_ptr - ent_list.data);
}

bool entity_is_valid(Slice_Entity entity_list, EntityHandle handle) {
    Entity* ent = slice_getp(entity_list, handle.index);
    if (!ent->active) {
        return false;
    }

    if (ent->generation != handle.generation) {
        return false;
    }

    return true;
}

Entity* entity_list_get(Slice_Entity entity_list, EntityHandle handle) {
    Entity* ret = NULL;

    Entity* ent = slice_getp(entity_list, handle.index);
    if (ent->active && ent->generation == handle.generation) {
        ret = ent;
    }

    return ret;
}

// create_entity(Slice_Entity* entity_list, Entity) {
//
// }

void entity_add_physics_component(Entity* ent, PhysicsComponent physics) {
    ent->flags |= EntityFlags_physics;
    ent->physics = physics;
}

EntityHandle create_entity(GameState* s, Entity entity) {
    bool force_handle = false;
    if (entity.generation > 0) {
        force_handle = true;
    }
    entity.active = true;

    Slice_Entity* entity_list = &s->entities;

    Entity* ent = NULL;

    if (!force_handle) {
        bool found_hole = false;
        for (u32 i = 0; i < entity_list->length; i++) {
            Entity* existing_ent = slice_getp(*entity_list, i);
            if (existing_ent->active == false) {
                entity.generation = existing_ent->generation + 1;
                *existing_ent = entity;
                ent = existing_ent;
                found_hole = true;
                ent->index = i;
                break;
            }
        }

        if (!found_hole) {
            entity.generation = 1;
            entity.index = entity_list->length;
            slice_push(entity_list, entity);
            ent = slice_back(*entity_list);
        }
    }

    if (force_handle) {
        ent = slice_getp(*entity_list, entity.index);
        *ent = entity;

        entity_list->length = max(entity.index + 1, entity_list->length);
        ASSERT(entity_list->length <= entity_list->capacity);
    }

    // s->serial += 1;
    // ent->net_id = s->serial;

    // initialize out of struct components like physics
    if (ent->flags & EntityFlags_physics) {
        PhysicsComponent* phys = &ent->physics;
        b2BodyDef body_def = b2DefaultBodyDef();
        body_def.type = phys->body_type;
        body_def.position = ent->position.b2vec;
        body_def.linearVelocity = phys->linear_velocity.b2vec;
        body_def.fixedRotation = true;
        body_def.linearDamping = phys->linear_damping;
        ent->body_id = b2CreateBody(s->world_id, &body_def);
        ColliderDef collider = phys->collider;
        b2Polygon polygon = b2MakeBox(collider.half_width, collider.half_height);
        b2ShapeDef shape_def = b2DefaultShapeDef();
        shape_def.isSensor = phys->is_sensor;
        shape_def.friction = 0;
        shape_def.userData = ent;
        b2ShapeId shape_id = b2CreatePolygonShape(ent->body_id, &shape_def, &polygon);
    }

    return (EntityHandle) {
        .index = ent->index,
        .generation = ent->generation,
    };
}

// // initialize out of struct components like physics
// void entity_init(GameState* s, Entity* ent) {
//     if (ent->flags & EntityFlags_physics) {
//         b2BodyDef body_def = b2DefaultBodyDef();
//         body_def.type = ent->body_type;
//         body_def.position = ent->position.b2vec;
//         body_def.linearVelocity = ent->linear_velocity.b2vec;
//         body_def.fixedRotation = true;
//         b2CreateBody(s->world_id, &body_def);
//         ColliderDef collider = ent->collider;
//         b2Polygon polygon = b2MakeBox(collider.half_width, collider.half_height);
//     }
// }

void create_box(Slice_Entity* create_list, float2 position) {
    Entity ent = {
        .sprite = TextureID_box_png,
        .type = EntityType_Box,
        .flags = EntityFlags_physics | EntityFlags_hittable,
        .replication_type = ReplicationType_Predicted,
        .max_health = box_health,
        .position = position,
    };
    ent.health = ent.max_health;

    entity_add_physics_component(&ent, (PhysicsComponent) {
        .collider = (ColliderDef) {0.5, 0.5},
        .body_type = b2_dynamicBody,
        .linear_damping = 4,
    });

    slice_push(create_list, ent);
}

void create_wall(Slice_Entity* create_list, float2 position, Dir8 direction) {
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

    Entity ent = {
        .replication_type = ReplicationType_Predicted,
        .sprite = TextureID_ice_wall_png,
        .sprite_src = src,
        .flip_sprite = flip_sprite,
        .flags = EntityFlags_physics | EntityFlags_hittable,
        .max_health = 50,
        .position = position,
    };
    ent.health = ent.max_health;

    entity_add_physics_component(&ent, (PhysicsComponent) {
        .collider = (ColliderDef) {1, 1},
    });

    slice_push(create_list, ent);
}

void create_bullet(Slice_Entity* create_list, EntityHandle owner, float2 position, float2 direction, u32 current_tick, i32 tick_rate) {
    Entity bullet = {
        .sprite = TextureID_fireball_png,
        // .mix_color = {255,0,0,0},
        // .t = 0.5,
        .replication_type = ReplicationType_Predicted,
        .type = EntityType_Bullet,
        .flags = EntityFlags_physics | EntityFlags_attack | EntityFlags_expires,
        .expire_tick = current_tick + (u32)(bullet_expire_duration * tick_rate),
        .owner = owner,
        .position = position,
        .damage = 20,
    };

    entity_add_physics_component(&bullet, (PhysicsComponent) {
        .collider = (ColliderDef) {0.25, 0.25},
        .linear_velocity = float2_scale(direction, 14),
        .body_type = b2_dynamicBody,
        .is_sensor = true,
    });

    slice_push(create_list, bullet);
}

void create_bolt(Slice_Entity* create_list, EntityHandle owner, float2 position, float2 direction, u32 current_tick, i32 tick_rate) {
    Entity bullet = {
        .sprite = TextureID_bullet_png,
        .replication_type = ReplicationType_Predicted,
        .type = EntityType_Bullet,
        .flags = EntityFlags_physics | EntityFlags_attack | EntityFlags_expires,
        .expire_tick = current_tick + (u32)(bullet_expire_duration * tick_rate),
        .owner = owner,
        .position = position,
        .damage = 10,
    };

    entity_add_physics_component(&bullet, (PhysicsComponent) {
        .collider = (ColliderDef) {0.25, 0.25},
        .linear_velocity = float2_scale(direction, 8),
        .body_type = b2_dynamicBody,
        .is_sensor = true,
    });

    slice_push(create_list, bullet);
}

void delete_entity(Slice_Entity* entity_list, EntityIndex index) {
    Entity* ent = slice_getp(*entity_list, index);
    if (ent->active) {
        ent->active = false;

        if (ent->flags & EntityFlags_physics && b2Body_IsValid(ent->body_id)) {
            b2DestroyBody(ent->body_id);
        }
    }
}

void state_init(GameState* state, Arena* arena) {
    state->entities = slice_create_fixed(Entity, arena, MaxEntities);

    b2WorldDef world_def = b2DefaultWorldDef();
    world_def.gravity = (b2Vec2){0.0f, 0.0f};
    state->world_id = b2CreateWorld(&world_def);
    state->create_list = slice_create(Entity, arena, MaxEntities);
    state->delete_list = slice_create(EntityIndex, arena, MaxEntities);



    Camera2D cam = DefaultCamera;

    b2BodyDef body_def = b2DefaultBodyDef();
    // body_def.type = phys->body_type;
    body_def.position = (b2Vec2) {0, cam.size.y / 2 + 0.5};
    b2BodyId id0 = b2CreateBody(state->world_id, &body_def);
    // body_def.position = (b2Vec2) {4,0};
    body_def.position = (b2Vec2) {0, -(cam.size.y / 2 + 0.5)};
    b2BodyId id1 = b2CreateBody(state->world_id, &body_def);
    body_def.position = (b2Vec2) {cam.size.x / 2 + 0.5, 0};
    b2BodyId id2 = b2CreateBody(state->world_id, &body_def);
    body_def.position = (b2Vec2) {-(cam.size.x / 2 + 0.5), 0};
    b2BodyId id3 = b2CreateBody(state->world_id, &body_def);

    b2Polygon polygon = b2MakeBox(30, 0.5);
    b2ShapeDef shape_def = b2DefaultShapeDef();
    // shape_def.friction = 0;
    b2CreatePolygonShape(id0, &shape_def, &polygon);
    b2CreatePolygonShape(id1, &shape_def, &polygon);

    polygon = b2MakeBox(0.5, 20);
    b2CreatePolygonShape(id2, &shape_def, &polygon);
    b2CreatePolygonShape(id3, &shape_def, &polygon);
}

void mod_lists_clear(GameState* state) {
    slice_clear(&state->delete_list);
    slice_clear(&state->create_list);
}

const float player_speed = 7;
const float dash_speed = 20;
const float dash_distance = 3;

#define dash_duration (dash_distance / dash_speed)


float2 rotate(float2 vector, f32 angle) {
    float2x2 rotation_matrix = {
        (f32)cos(angle), (f32)-sin(angle),
        (f32)sin(angle), (f32)cos(angle)
    };

    return float2x2_mult_float2(rotation_matrix, vector);
}

const double pi = 3.141592653589793238462643383279502884197;
#define deg_in_rads  (pi / 180);

float deg_to_rad(float deg) {
    return deg * deg_in_rads;
}

typedef struct ModLists {
    Slice_EntityIndex* delete_list;
    Slice_Entity* create_list;
} ModLists;

// modified entities is out parameter so server can send mod messages to client for replication
void state_update(GameState* state, Inputs inputs, u32 current_tick, i32 tick_rate, bool modify_existences, int aaa) {
    // ArenaTemp scratch = scratch_get(0,0);
    // defer_loop((void)0, scratch_release(scratch)) {

    const f64 dt = 1.0 / tick_rate;

    b2World_Step(state->world_id, dt, substep_count);

    Slice_EntityIndex* delete_list = &state->delete_list;
    Slice_Entity* create_list = &state->create_list;
    // Slice_EntityIndex delete_list = slice_create(EntityIndex, mod_lists_arena, 512);
    // Slice_Entity create_list = slice_create(Entity, mod_lists_arena, 128);

    b2SensorEvents sensorEvents = b2World_GetSensorEvents(state->world_id);
    for (int i = 0; i < sensorEvents.beginCount; ++i) {
        b2SensorBeginTouchEvent* beginTouch = sensorEvents.beginEvents + i;
        Entity* sensor = (Entity*)b2Shape_GetUserData(beginTouch->sensorShapeId);
        Entity* visitor = (Entity*)b2Shape_GetUserData(beginTouch->visitorShapeId);

        if (visitor == NULL) {
            continue;
        }

        bool skip = !(sensor->flags & EntityFlags_damage_owner) && entity_handle_equals(sensor->owner, entity_ptr_to_handle(visitor));
        if (!skip) {
            if (!visitor->active) {
                skip = true;
            }
            if (visitor->flags & EntityFlags_player && visitor->player_state == PlayerState_Dashing) {
                skip = true;
            }
        }

        if (!skip && visitor != NULL && visitor->flags & EntityFlags_hittable) {
            float2 inc_dir = normalize((float2){.b2vec=b2Body_GetLinearVelocity(sensor->body_id)});
            b2Body_ApplyLinearImpulse(visitor->body_id, float2_scale(inc_dir, 2.0f).b2vec, b2Body_GetWorldCenterOfMass(visitor->body_id), true);

            visitor->hit_flash_end_tick = current_tick + hit_flash_duration * tick_rate;

            slice_push(delete_list, entity_ptr_to_index(state->entities, sensor));
            // entity_list_remove(&state->entities, entity_ptr_to_index(&state->entities, sensor));

            visitor->health -= sensor->damage;
        }
    }


    for (u32 i = 0; i < state->entities.length; i++) {
        Entity* ent = &state->entities.data[i];

        if (!ent->active) {
            continue;
        }

        if (ent->flags & EntityFlags_expires && current_tick >= ent->expire_tick) {
            slice_push(delete_list, i);
        }

        if (ent->flags & EntityFlags_hittable && ent->health <= 0) {
            slice_push(delete_list, i);
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

            // if (modify_existences && ent->client_id == 1){
            //     printf("input: %d\n",input->left);
            // }

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
            // };b2Body_GetPosition(ent->body_id)
            float2 current_pos = (float2) {.b2vec = b2Body_GetPosition(ent->body_id)};
            if (current_tick < ent->dash_end_tick + 0.1 * TICK_RATE) {
                ent->dash_trail = float2_add(ent->dash_trail, float2_scale(float2_sub(current_pos, ent->dash_trail), 20 * dt));
            } else {
                ent->dash_trail = current_pos;
            }

            if (ent->player_state == PlayerState_Neutral) {
                ent->t = 0;
                // if (move_input.x == 0 && move_input.y == 0) {
                //
                // }
                // ent->dash_trail = ent->position;
                if (magnitude(move_input) > 0) {
                    float2 move_direction = normalize(move_input);
                    velocity = float2_scale(move_direction, player_speed);
                    // printf("%f, %f\n", velocity.x, velocity.y);

                    if (input->dash && (current_tick > ent->dash_end_tick + 0.2 * tick_rate || !ent->not_first_dash)) {
                        ent->not_first_dash = true;
                        ent->player_state = PlayerState_Dashing;
                        ent->dash_direction = move_direction;
                        ent->dash_end_tick = current_tick + dash_duration * tick_rate;
                    }
                }


                b2Body_SetLinearVelocity(ent->body_id, (b2Vec2){velocity.x, velocity.y});
            } else if (ent->player_state == PlayerState_Dashing) {
                // float dash_gap = 0.5;

                ent->mix_color = (RGBA){255,255,255,255};
                ent->t = 0.75;
                velocity = float2_scale(ent->dash_direction, dash_speed);

                b2Body_SetLinearVelocity(ent->body_id, (b2Vec2){velocity.x, velocity.y});

                if (current_tick >= ent->dash_end_tick) {
                    ent->player_state = PlayerState_Neutral;
                }
            }

            EntityHandle handle = {i, ent->generation};

            ent->mana_regen_tick_acc += 1;

            const i32 mana_per_second = 50;
            const i32 ticks_per_mana = TICK_RATE / mana_per_second;
            while (ent->mana_regen_tick_acc > ticks_per_mana) {
                ent->mana += 1;
                ent->mana_regen_tick_acc -= ticks_per_mana;
            }

            // ent->mana += 1;
            ent->mana = min(ent->mana, ent->max_mana);


            SpellType selected_spell = SpellType_NULL;
            if (ent->selected_spell >= 0 && ent->selected_spell < hotbar_length) {
                selected_spell = ent->hotbar[ent->selected_spell];
            }
            Spell spell = spell_table[selected_spell];
            bool sufficient_mana = ent->mana >= spell.mana_cost;

            float2 player_pos = {.b2vec=b2Body_GetPosition(ent->body_id)};
            float2 aim_direction = {1,0};
            if (magnitude(float2_sub(input->cursor_world_pos, player_pos)) > 0) {
                aim_direction = normalize(float2_sub(input->cursor_world_pos, player_pos));
            }
            f32 d = 1 - fmod(atan2f(aim_direction.y, aim_direction.x) / (2*pi) + 0.5 + 0.1875, 1);
            Dir8 dir = (Dir8) (d * 8);

            ent->sprite_src.size = (float2){16,32};
            ent->flip_sprite = (dir > Dir8_Down) ? true : false;
            switch(dir) {
                case Dir8_Up:
                ent->sprite_src.position = (float2){0,32};
                break;
                case Dir8_UpRight:
                case Dir8_UpLeft:
                ent->sprite_src.position = (float2){48,0};
                break;
                case Dir8_Right:
                case Dir8_Left:
                ent->sprite_src.position = (float2){32,0};
                break;
                case Dir8_DownRight:
                case Dir8_DownLeft:
                ent->sprite_src.position = (float2){16,0};
                break;
                case Dir8_Down:
                ent->sprite_src.position = (float2){0,0};
                break;
            }
            if (input->fire && sufficient_mana) {

                ent->mana -= spell.mana_cost;

                if (selected_spell == SpellType_Fireball) {
                    create_bullet(create_list, handle, player_pos, aim_direction, current_tick, tick_rate);
                }

                if (selected_spell == SpellType_SpreadBolt) {
                    i32 projectile_count = 5;

                    f32 range_start = deg_to_rad(-25);
                    f32 range_end = deg_to_rad(25);

                    for (i32 i = 0; i < projectile_count; i++) {
                        f32 dir_offset = lerp(range_start, range_end, (f32)i / (projectile_count - 1));
                        create_bolt(create_list, handle, player_pos, rotate(aim_direction, dir_offset), current_tick, tick_rate);
                    }
                }

                if (selected_spell == SpellType_IceWall) {
                    create_wall(create_list, float2_add(player_pos, float2_scale(aim_direction, 2)), dir);
                }
            }
        }

        if (ent->flags & EntityFlags_physics) {
            ent->position.b2vec = b2Body_GetPosition(ent->body_id);
            ent->physics.linear_velocity.b2vec = b2Body_GetLinearVelocity(ent->body_id);

            if (!ent->sensor_added_this_tick) {
                ent->last_tick_sensor = (EntityHandle){0};
            }
            ent->sensor_added_this_tick = false;
        }
    }

    if (modify_existences) {
        for (u32 i = 0; i < delete_list->length; i++) {
            delete_entity(&state->entities, slice_get(*delete_list, i));
        }

        for (u32 i = 0; i < create_list->length; i++) {
            EntityHandle handle = create_entity(state, slice_get(*create_list, i));
            *slice_getp(*create_list, i) = state->entities.data[handle.index];
        }
    }
}

void create_player(Slice_Entity* create_list, ClientID client_id, float2 position) {
    // b2BodyDef body_def = b2DefaultBodyDef();
    // body_def.fixedRotation = true;
    // b2BodyId body_id = b2CreateBody(state->world_id, &body_def);
    
    Entity ent = {
        .sprite = TextureID_player_2_png,
        .sprite_src.size = (float2){16,32},
        .type = EntityType_Player,
        .flags = EntityFlags_player | EntityFlags_physics | EntityFlags_hittable | EntityFlags_has_mana,
        .replication_type = ReplicationType_Conditional,
        .hotbar = {SpellType_Fireball, SpellType_SpreadBolt, SpellType_IceWall, SpellType_SniperRifle},
        .client_id = client_id,
        .max_health = 100,
        .max_mana = 100,
        .position = position,
    };
    ent.health = ent.max_health;
    ent.mana = ent.max_mana;

    entity_add_physics_component(&ent, (PhysicsComponent) {
        .collider = (ColliderDef){0.35, 0.65},
        .body_type = b2_dynamicBody,
    });

    slice_push(create_list, ent);
}

// out params
// void entities_to_snapshot(Slice_Ghost* ghosts, Slice_Entity ents, u64 current_tick, Slice_pEntity* players) {
//     // Slice_Ghost ghosts = slice_create(Ghost, tick_arena, ents.length);
//
//     for (i32 i = 0; i < ents.length; i++) {
//         Entity* ent = slice_getp(ents, i);
//         if (ent->active) {
//             slice_push(ghosts, (Ghost){
//                 .replication_type = ent->replication_type,
//                 .sprite = ent->sprite,
//                 .sprite_src = ent->sprite_src,
//                 .flip_sprite = ent->flip_sprite,
//                 .position = {.b2vec=b2Body_GetPosition(ent->body_id)},
//                 .health = (u16)ent->health,
//                 .show_health = (bool)(ent->flags & EntityFlags_hittable),
//                 .hit_flash = ent->hit_flash_end_tick > current_tick,
//             });
//
//             if (players != NULL && ent->type == EntityType_Player) {
//                 ent->physics.position.b2vec = b2Body_GetPosition(ent->body_id);
//                 slice_push(players, ent);
//             }
//         }
//     }
// }
