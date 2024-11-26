#pragma once
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

class Snapshot : public yojimbo::Message {
  public:
    State state{};

    template <typename Stream> bool Serialize(Stream& stream) {
        for (int i = 0; i < state.players.size(); i++) {
            auto& pos = state.players[i];
            serialize_float(stream, pos.x);
            serialize_float(stream, pos.y);
        }
        return true;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
};


YOJIMBO_MESSAGE_FACTORY_START(GameMessageFactory, (int)GameMessageType::Count)
YOJIMBO_DECLARE_MESSAGE_TYPE((int)GameMessageType::Test, TestMessage)
YOJIMBO_MESSAGE_FACTORY_FINISH()

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
    void Update(float dt);
    void ProcessMessages();
    void ProcessMessage(int client_index, yojimbo::Message* message);
    void ProcessTestMessage(int client_index, TestMessage* message);
private:
    GameConnectConfig m_connection_config;
    GameAdapter m_adapter;
    yojimbo::Server m_server;

    double m_time;
    bool m_running = true;
};
