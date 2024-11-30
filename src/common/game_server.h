#pragma once
#include "EASTL/fixed_vector.h"
#include "game.h"
#include "serialize.h"
#include "yojimbo.h"

constexpr uint8_t default_private_key[yojimbo::KeyBytes] = {0};
constexpr int max_players = 64;

enum class GameMessageType {
    Test,
    Snapshot,
    Input,
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
        auto& players = state.players;
        if (Stream::IsWriting) {
            write_int(stream, players.size(), 0, 16);
        }

        if (Stream::IsReading) {
            int size;
            read_int(stream, size, 0, 16);
            players.reserve(size);
            for (int i = 0; i < size; i++) {
                players.push_back({});
            }
        }

        for (int i = 0; i < players.size(); i++) {
            auto& pos = players[i];
            serialize_float(stream, pos.x);
            serialize_float(stream, pos.y);
        }

        for (int i = 0; i < bullets_capacity; i++) {
            serialize_bytes(stream, (uint8_t*)&state.bullets, sizeof(Bullets));
        }

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

YOJIMBO_MESSAGE_FACTORY_START(GameMessageFactory, (int)GameMessageType::Count);
YOJIMBO_DECLARE_MESSAGE_TYPE((int)GameMessageType::Test, TestMessage);
YOJIMBO_DECLARE_MESSAGE_TYPE((int)GameMessageType::Snapshot, SnapshotMessage);
YOJIMBO_DECLARE_MESSAGE_TYPE((int)GameMessageType::Input, InputMessage);
YOJIMBO_MESSAGE_FACTORY_FINISH();

// class GameMessageFactory : public yojimbo ::MessageFactory {
//   public:
//     GameMessageFactory(yojimbo ::Allocator& allocator) : MessageFactory(allocator, (int)GameMessageType ::Count) {}
//     yojimbo ::Message* CreateMessageInternal(int type) {
//         yojimbo ::Message* message;
//         yojimbo ::Allocator& allocator = GetAllocator();
//         (void)allocator;
//         switch (type) {
//             ;
//         case (int)GameMessageType ::Test:
//             message = (new ((allocator).Allocate(
//                 sizeof(TestMessage), "C:\\Users\\toodemhard\\projects\\games\\multiplayer_2\\src\\common\\game_server.h", 102
//             )) TestMessage());
//             if (!message)
//                 return 0;
//             SetMessageType(message, (int)GameMessageType ::Test);
//             return message;
//             ;
//         case (int)GameMessageType ::Snapshot:
//             message = (new ((allocator).Allocate(
//                 sizeof(SnapshotMessage), "C:\\Users\\toodemhard\\projects\\games\\multiplayer_2\\src\\common\\game_server.h", 111
//             )) SnapshotMessage());
//             if (!message)
//                 return 0;
//             SetMessageType(message, (int)GameMessageType ::Snapshot);
//             return message;
//             ;
//         case (int)GameMessageType ::Input:
//             message = (new ((allocator).Allocate(
//                 sizeof(InputMessage), "C:\\Users\\toodemhard\\projects\\games\\multiplayer_2\\src\\common\\game_server.h", 120
//             )) InputMessage());
//             if (!message)
//                 return 0;
//             SetMessageType(message, (int)GameMessageType ::Input);
//             return message;
//             ;
//         default:
//             return 0;
//         }
//     }
// };
// ;

class GameServer;

class GameAdapter : public yojimbo::Adapter {
  public:
    explicit GameAdapter(GameServer* server = NULL) : m_server(server) {}

    yojimbo::MessageFactory* CreateMessageFactory(yojimbo::Allocator& allocator) override {
        return YOJIMBO_NEW(allocator, GameMessageFactory, allocator);
    }

    void OnServerClientConnected(int client_index) override;

    void OnServerClientDisconnected(int client_index) override;

  private:
    GameServer* m_server;
};

class GameServer {
  public:
    GameServer(const yojimbo::Address& address);
    void ClientConnected(int client_index);
    void ClientDisconnected(int client_index);
    void Run();
    void Update(double time, double dt);
    void ProcessMessages();
    void ProcessMessage(int client_index, yojimbo::Message* message);
    void ProcessTestMessage(int client_index, TestMessage* message);

  private:
    GameConnectConfig m_connection_config;
    GameAdapter m_adapter;
    yojimbo::Server m_server;

    State m_state{ {{0.5, 0.5}, {0, 0}, {-1, 0}} };

    double m_time;
    bool m_running = true;
};
