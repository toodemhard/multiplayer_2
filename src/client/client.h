#pragma once
#include <chrono>
#include <yojimbo.h>
#include "EASTL/fixed_vector.h"
#include "game.h"
#include "input.h"
#include "game_server.h"

class OnlineGameScreen {
  public:
    OnlineGameScreen(const yojimbo::Address& server_address);
    ~OnlineGameScreen();
    void Update(float dt, PlayerInput input, Input::Input& input_2);
    void ProcessMessages();
    void ProcessMessage(yojimbo::Message* message);

    State m_state;

    std::chrono::time_point<std::chrono::steady_clock> m_packet_sent_time;

    // glm::vec2 m_pos;
    eastl::fixed_vector<glm::vec2, 16> m_players;

  private:
    GameConnectConfig m_connection_config;
    GameAdapter m_adapter;
    yojimbo::Client m_client;
};

