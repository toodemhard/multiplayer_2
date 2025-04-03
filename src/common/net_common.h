#pragma once

const int input_buffer_capacity = 16;
const int target_input_buffer_size = 3;


#define TICK_RATE 64
#define FIXED_DT (1.0 / TICK_RATE)
// const inline yojimbo::Address server_address(127, 0, 0, 1, 5000);


typedef enum MessageType {
    MessageType_Test,
    MessageType_Input,
    MessageType_Snapshot,
} MessageType;

typedef enum Channel {
    Channel_Unreliable,
    Channel_Reliable,
} Channel;

typedef struct Message {
    MessageType type;
    void* data;
} Message;

typedef enum StreamOperation {
    Stream_Write,
    Stream_Read,
} StreamOperation;

// for initialization just allocate slice, zero rest
typedef struct Stream Stream;
struct Stream {
    Slice_u8 slice;
    u64 current;
    u8 bit_index;
    StreamOperation operation;
};

Stream stream_create(Slice_u8 slice, StreamOperation operation);

void stream_clear(Stream* stream);
void stream_pos_reset(Stream* stream);

typedef struct TestMessage TestMessage;
struct TestMessage {
    Slice_u8 str;
};

typedef struct SnapshotMessage SnapshotMessage;
struct SnapshotMessage {
    u8 input_buffer_size;
    Slice_Ghost ghosts;
    Entity player;
};

#define serialize_var(stream, var_ptr) serialize_bytes( stream, (u8*)(var_ptr), sizeof(*(var_ptr)) )


#define serialize_slice(stream, slice)\
    serialize_slice_raw( (stream), (slice)->data, sizeof((slice)->data[0]), &(slice)->length, (slice)->capacity )

void serialize_bool(Stream* stream, bool* value);
void serialize_int(Stream* stream, i32* num);
void serialize_bytes(Stream* stream, u8* bytes, u64 size);
// void serialize_string(Stream* stream, Arena* read_arena, String8* string);
void serialize_test_message(Stream* stream, Arena* read_arena, TestMessage* message);

void serialize_entity(Stream* stream, Entity* ent);
void serialize_input_message(Stream* stream, PlayerInput* input);
void serialize_snapshot(Stream* stream, u8* input_buffer_size, Slice_Ghost* ghosts, Entity* player);

void serialize_slice_raw(Stream* stream, void* data, u64 element_size, u64* length, u64 capacity);
