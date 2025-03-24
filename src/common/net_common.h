#pragma once

constexpr int input_buffer_capacity = 16;
constexpr int target_input_buffer_size = 3;


constexpr int tick_rate = 64;
constexpr f64 fixed_dt = 1.0 / tick_rate;
// const inline yojimbo::Address server_address(127, 0, 0, 1, 5000);


enum MessageType {
    MessageType_Test,
    MessageType_Input,
    MessageType_Snapshot,
};

enum Channel {
    Channel_Unreliable,
    Channel_Reliable,
};

struct Message {
    MessageType type;
    void* data;
};

enum StreamOperation {
    Stream_Write,
    Stream_Read,
};

// for initialization just allocate slice, zero rest
struct Stream {
    Slice<u8> slice;
    u64 current;
    u8 bit_index;
    StreamOperation operation;
};

Stream stream_create(Slice<u8> slice, StreamOperation operation);

void stream_clear(Stream* stream);
void stream_pos_reset(Stream* stream);

struct TestMessage {
    Slice<u8> str;
};

struct Snapshot {
    u8 input_buffer_size;
    Slice<Ghost> ghosts;
    Entity player;
};

#define serialize_var(stream, var_ptr) serialize_bytes(stream, (u8*)var_ptr, sizeof(*var_ptr))

template<typename T>
void serialize_slice(Stream* stream, Slice<T>* slice) {
    serialize_var(stream, &slice->length);
    ASSERT(slice->length <= slice->capacity);

    for (i32 i = 0; i < slice->length; i++) {
        serialize_var(stream, &(*slice)[i]);
    }
}

void serialize_bool(Stream* stream, bool* value);
void serialize_int(Stream* stream, i32* num);
void serialize_bytes(Stream* stream, u8* bytes, u64 size);
// void serialize_string(Stream* stream, Arena* read_arena, String8* string);
void serialize_test_message(Stream* stream, Arena* read_arena, TestMessage* message);

void serialize_input_message(Stream* stream, PlayerInput* input);
void serialize_snapshot(Stream* stream, u8* input_buffer_size, Slice<Ghost>* ghosts, Entity* player);
