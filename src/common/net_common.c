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

void serialize_snapshot(Stream* stream, u8* input_buffer_size, Slice_Ghost* ghosts, Entity* player) {
    MessageType message_type = MessageType_Snapshot;
    serialize_var(stream, &message_type);
    serialize_var(stream, input_buffer_size);
    serialize_slice(stream, ghosts);

    bool player_alive = false;

    if (player != NULL) {
        player_alive = true;
    }

    serialize_bool(stream, &player_alive);

    if (player_alive) {
        serialize_player(stream, player);
    }
}

void serialize_input_message(Stream* stream, PlayerInput* input) {
    MessageType type = MessageType_Input;
    serialize_var(stream, &type);
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
