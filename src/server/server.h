#pragma once
#include "EASTL/bonus/fixed_ring_buffer.h"
#include "EASTL/fixed_vector.h"
#include "game.h"
#include "serialize.h"
#include "yojimbo.h"
#include "net_common.h"

#include <EASTL/bonus/ring_buffer.h>

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
    GameAdapter<GameServer> m_adapter;
    yojimbo::Server m_server;

    State m_state{};

    int m_frame = 0;

    double m_time;
    bool m_running = true;
    ring_buffer<PlayerInput, input_buffer_capacity> m_input_buffers[max_player_count]{};

    //send once only send again once client responds
    // bool throttle_sending[max_player_count]{};

};
