Stream stream_create(Slice_u8 slice, StreamOperation operation) {
    return (Stream) {
        .slice = slice,
        .operation = operation,
    };
}

void stream_clear(Stream* stream) {
    slice_clear(&stream->slice);
    stream->current = 0;
    stream->bit_index = 0;
}

void stream_pos_reset(Stream* stream) {
    stream->current = 0;
    stream->bit_index = 0;
}

void serialize_player(Stream* s, Entity* player) {
    serialize_var(s, &player->position);
    serialize_var(s, &player->hotbar);
    serialize_var(s, &player->selected_spell);
}

// void serialize_init_entities(Stream* stream, Slice_Entity* entities) {
//     u32* count_ref = (u32*)slice_getp(stream->slice, stream->current);
//
//     u32 count = 0;
//     serialize_var(stream, &count);
//
//     if (stream->operation == Stream_Write) {
//         for (u32 i = 0; i < entities->length; i++) {
//             Entity* ent = slice_getp(*entities, i);
//
//             if (ent->active && ent->replication_type == ReplicationType_Predicted) {
//                 serialize_snapshot_entity(stream, ent);
//                 count += 1;
//             }
//         }
//
//         *count_ref = count;
//     }
//
//     if (stream->operation == Stream_Read) {
//         for (u32 i = 0; i < count; i++) {
//             slice_push(entities, (Entity){});
//             Entity* ent = slice_back(*entities);
//             serialize_init_entity(stream, ent);
//         }
//     }
// }

typedef enum EntitySerializationType {
    EntitySerializationType_Init,
    EntitySerializationType_Update,
} EntitySerializationType;

void serialize_entities(Stream* stream, Slice_Entity* entities, EntitySerializationType serialization_type) {
    u32* count_ref = (u32*)slice_getp(stream->slice, stream->current);

    u32 count = 0;
    serialize_var(stream, &count);

    if (stream->operation == Stream_Write) {
        for (u32 i = 0; i < entities->length; i++) {
            Entity* ent = slice_getp(*entities, i);

            bool serialize = false;
            if (ent->active) {
                serialize = true;
            }
            if (serialize) {
                if (serialization_type == EntitySerializationType_Init) {
                    serialize_init_entity(stream, ent);
                } else if (serialization_type == EntitySerializationType_Update) {
                    serialize_snapshot_entity(stream, ent);
                }
                count += 1;
            }
        }
        *count_ref = count;
    }

    if (stream->operation == Stream_Read) {
        for (u32 i = 0; i < count; i++) {
            slice_push(entities, (Entity){0});
            Entity* ent = slice_back(*entities);
            if (serialization_type == EntitySerializationType_Init) {
                serialize_init_entity(stream, ent);
            } else if (serialization_type == EntitySerializationType_Update) {
                serialize_snapshot_entity(stream, ent);
            }
        }
    }
}

// only serialize rendering and stuff to check for rollback
// position + linear velocity atm
void serialize_snapshot_entity(Stream* stream, Entity* ent) {
    serialize_var(stream, &ent->generation);
    serialize_var(stream, &ent->index);
    // serialize_var(stream, &ent->type);
    serialize_var(stream, &ent->flags);
    serialize_var(stream, &ent->replication_type);

    serialize_var(stream, &ent->sprite);
    serialize_var(stream, &ent->sprite_src);
    serialize_var(stream, &ent->flip_sprite);

    if (ent->flags & EntityFlags_physics) {
        serialize_var(stream, &ent->position);
        // serialize_var(stream, &ent->linear_velocity);
    }

    if (ent->flags & EntityFlags_hittable) {
        serialize_var(stream, &ent->health);
        serialize_var(stream, &ent->hit_flash_end_tick);
    }

    if (ent->flags & EntityFlags_expires) {
        serialize_var(stream, &ent->expire_tick);
    }
}

// serialize fully with all init config stuff for recreating
void serialize_init_entity(Stream* stream, Entity* ent) {
    serialize_var(stream, &ent->type);
    serialize_var(stream, &ent->flags);

    serialize_var(stream, &ent->sprite);
    serialize_var(stream, &ent->sprite_src);
    serialize_var(stream, &ent->flip_sprite);

    if (ent->flags & EntityFlags_player) {
        serialize_bytes(stream, (u8*)ent->hotbar, sizeof(ent->hotbar));
        serialize_var(stream, &ent->player_state);
        serialize_var(stream, &ent->dash_direction);
        serialize_var(stream, &ent->dash_end_tick);
        serialize_var(stream, &ent->selected_spell);
    }

    if (ent->flags & EntityFlags_physics) {
        serialize_var(stream, &ent->position);
        serialize_var(stream, &ent->physics);
    }

    if (ent->flags & EntityFlags_hittable) {
        serialize_var(stream, &ent->health);
        serialize_var(stream, &ent->hit_flash_end_tick);
    }

    if (ent->flags & EntityFlags_expires) {
        serialize_var(stream, &ent->expire_tick);
    }
}

void serialize_state_init_message(Stream* stream, Slice_Entity* entities, ClientID* client_id, u32* tick) {
    MessageType message_type = MessageType_StateInit;
    serialize_var(stream, &message_type);
    serialize_var(stream, client_id);
    serialize_var(stream, tick);
    serialize_entities(stream, entities, EntitySerializationType_Init);
}

void serialize_snapshot_message(Stream* stream, SnapshotMessage* s) {
    MessageType message_type = MessageType_Snapshot;
    serialize_var(stream, &message_type);
    serialize_var(stream, &s->tick_index);
    serialize_var(stream, &s->input_buffer_size);

    serialize_entities(stream, &s->ents, EntitySerializationType_Update);

    // bool player_alive = false;
    //
    // // if (s->player != NULL) {
    // //     player_alive = true;
    // // }
    //
    // serialize_bool(stream, &player_alive);
    //
    // if (player_alive) {
    //     serialize_player(stream, s->player);
    // }
}

void serialize_input_message(Stream* stream, PlayerInput* input, u32* tick) {
    MessageType type = MessageType_Input;
    serialize_var(stream, &type);

    serialize_var(stream, tick);

    serialize_bool(stream, &input->up);
    serialize_bool(stream, &input->down);
    serialize_bool(stream, &input->left);
    serialize_bool(stream, &input->right);
    serialize_bool(stream, &input->fire);
    serialize_bool(stream, &input->dash);

    for (i32 i = 0; i < array_length(input->select_spell); i++) {
        serialize_bool(stream, &input->select_spell[i]);
    }

    serialize_var(stream, &input->cursor_world_pos);
    serialize_var(stream, &input->move_spell_src);
    serialize_var(stream, &input->move_spell_dst);
}

void serialize_bool(Stream* stream, bool* value) {
    if (stream->operation == Stream_Write) {
        if (stream->bit_index == 0) {
            slice_push(&stream->slice, (u8)0);
        }
        *slice_getp(stream->slice, stream->current) |= *value << stream->bit_index;
    }

    if (stream->operation == Stream_Read) {
        *value = slice_get(stream->slice, stream->current) & (1 << stream->bit_index);
    }

    stream->bit_index++;

    if (stream->bit_index >= 8) {
        stream->current++;
        stream->bit_index = 0;
    }
}

void serialize_bytes(Stream* stream, u8* bytes, u64 size) {
    if (stream->bit_index > 0) {
        stream->current++;
        stream->bit_index = 0;
    }
    if (stream->operation == Stream_Read) {
        slice_copy_range_to_buffer(bytes, &stream->slice, stream->current, size);
    }
    if (stream->operation == Stream_Write) {
        slice_push_buffer(&stream->slice, bytes, size);
    }
    stream->current += size;
}

void serialize_slice_string(Stream* stream, Arena* read_arena, Slice_u8* string) {
    serialize_bytes(stream, (u8*)&string->length, sizeof(string->length));
        
    u64 string_size = string->length * sizeof(*string->data);
    if (stream->operation == Stream_Read) {
        string->data = (u8*)arena_alloc(read_arena, string_size);
    }
    serialize_bytes(stream, (u8*)string->data, string_size);
}

// when reading from stream dynamically allocated data types like slice, string needs arena to alloc on to
// when write can be null
void serialize_test_message(Stream* stream, Arena* read_arena, TestMessage* message) {
    MessageType type = MessageType_Test;
    serialize_bytes(stream, (u8*)&type, sizeof(type));
    serialize_slice_string(stream, read_arena, &message->str);
}

void serialize_slice_raw(Stream* stream, void* data, u64 element_size, u64* length, u64 capacity) {
    serialize_var(stream, length);
    ASSERT(*length <= capacity);

    serialize_bytes(stream, (u8*)data, element_size * *length);
}
