#pragma once
#include <chrono>
#include <yojimbo.h>
#include "EASTL/fixed_vector.h"
#include "game.h"
#include "input.h"
#include "net_common.h"


class ClientAdapter : public yojimbo::Adapter {
  public:
    yojimbo::MessageFactory* CreateMessageFactory(yojimbo::Allocator& allocator) override {
        return YOJIMBO_NEW(allocator, GameMessageFactory, allocator);
    }
};

class GameClient {
  public:
    GameClient(const yojimbo::Address& server_address);
    ~GameClient();
    void Update(float dt, PlayerInput input, Input::Input& input_2);
    void ProcessMessages();
    void ProcessMessage(yojimbo::Message* message);
    void ClientConnected(int client_index);
    void ClientDisconnected(int client_index);

    State m_state;

    std::chrono::time_point<std::chrono::steady_clock> m_packet_sent_time;

    // glm::vec2 m_pos;
    eastl::fixed_vector<glm::vec2, 16> m_players;

  private:
    GameConnectConfig m_connection_config;
    GameAdapter<GameClient> m_adapter;
    yojimbo::Client m_client;
};

