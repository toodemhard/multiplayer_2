#pragma once
#include "EASTL/bonus/fixed_ring_buffer.h"
#include "EASTL/fixed_vector.h"
#include "game.h"
#include "serialize.h"
#include "yojimbo.h"

#include <EASTL/bonus/ring_buffer.h>
#include <algorithm>
#include <limits>


constexpr uint8_t default_private_key[yojimbo::KeyBytes] = {0};

constexpr int input_buffer_capacity = 10;
constexpr int target_input_buffer_size = 3;

constexpr int tick_rate = 128;
const inline yojimbo::Address server_address(127, 0, 0, 1, 5000);

enum class GameMessageType {
    Test,
    Snapshot,
    Input,
    Init,
    ThrottleCommand,
    ThrottleComplete,
    Count,
};

enum class GameChannel {
    Reliable,
    Unreliable,
    Count,
};

struct GameConnectConfig : yojimbo::ClientServerConfig {
    GameConnectConfig() {
        numChannels = 2;
        channel[(int)GameChannel::Reliable].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
        channel[(int)GameChannel::Unreliable].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    }
};

class TestMessage : public yojimbo::Message {
  public:
    int m_data;

    TestMessage() : m_data(0) {}

    template <typename Stream> bool Serialize(Stream& stream) {
        serialize_int(stream, m_data, 0, 512);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

class SnapshotMessage : public yojimbo::Message {
  public:
    State state{};
    // eastl::fixed_vector<glm::vec2, 16> players;

    template <typename Stream> bool Serialize(Stream& stream) {
        serialize_bytes(stream, (uint8_t*)&state.players, sizeof(Players));

        serialize_bytes(stream, (uint8_t*)&state.bullets, sizeof(Bullets));

        // serialize_float(stream, pos.x);
        // serialize_float(stream, pos.y);
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

class InputMessage : public yojimbo::Message {
  public:
    PlayerInput input;

    template <typename Stream> bool Serialize(Stream& stream) {
        serialize_bool(stream, input.up);
        serialize_bool(stream, input.down);
        serialize_bool(stream, input.left);
        serialize_bool(stream, input.right);
        serialize_bool(stream, input.fire);

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

class InitMessage : public yojimbo::Message {
public:
    
    int frame;
    template <typename Stream> bool Serialize(Stream& stream) {
        serialize_int(stream, frame, 0, std::numeric_limits<int>::max());

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};


class ThrottleCommand : public yojimbo::Message {
public:
    int ticks;
    template <typename Stream> bool Serialize(Stream& stream) {
        serialize_int(stream, ticks, -input_buffer_capacity, input_buffer_capacity);

        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

class ThrottleComplete : public yojimbo::Message {
public:
    
    template <typename Stream> bool Serialize(Stream& stream) {
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};

YOJIMBO_MESSAGE_FACTORY_START(GameMessageFactory, (int)GameMessageType::Count);
YOJIMBO_DECLARE_MESSAGE_TYPE((int)GameMessageType::Test, TestMessage);
YOJIMBO_DECLARE_MESSAGE_TYPE((int)GameMessageType::Snapshot, SnapshotMessage);
YOJIMBO_DECLARE_MESSAGE_TYPE((int)GameMessageType::Input, InputMessage);
YOJIMBO_DECLARE_MESSAGE_TYPE((int)GameMessageType::Init, InitMessage);
YOJIMBO_DECLARE_MESSAGE_TYPE((int)GameMessageType::ThrottleCommand, ThrottleCommand);
YOJIMBO_DECLARE_MESSAGE_TYPE((int)GameMessageType::ThrottleComplete, ThrottleComplete);
YOJIMBO_MESSAGE_FACTORY_FINISH();

template<class Receiver>
class GameAdapter : public yojimbo::Adapter {
  public:
    explicit GameAdapter(Receiver* receiver) : m_receiver(receiver){}

    yojimbo::MessageFactory* CreateMessageFactory(yojimbo::Allocator& allocator) override {
        return YOJIMBO_NEW(allocator, GameMessageFactory, allocator);
    }

    void OnServerClientConnected(int client_index) override {
        if (m_receiver != NULL) {
            m_receiver->ClientConnected(client_index);
        }
    }

    void OnServerClientDisconnected(int client_index) override {
        if (m_receiver != NULL) {
            m_receiver->ClientDisconnected(client_index);
        }
    }

  private:
    Receiver* m_receiver;
};


template<typename T, size_t N>
class ring_buffer {
public:

    void push_back(T data) {
        // if (m_size >= N) {
        //     return;
        // }

        if (m_size < N) {
            m_size++;
        }

        m_buffer[m_start] = data;
        m_start++;
        if (m_start >= N) {
            m_start = 0;
        }
    }

    void pop_front() {
        if (m_size == 0) {
            return;
        }


        m_size--;
        m_buffer[m_end] = {};

        m_end++;

        if (m_end >= N) {
            m_end = 0;

        }
    }

    T& front() {
        return m_buffer[m_end];
    }

    int size() {
        return m_size;
    }

    bool empty() {
        return m_size == 0;
    }

    bool full() {
        return m_size == N;
    }

    int m_start, m_end, m_size;
    T m_buffer[N];
};