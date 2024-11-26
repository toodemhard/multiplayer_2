#include "game_server.h"
#include "yojimbo_adapter.h"
#include "yojimbo_allocator.h"
#include "yojimbo_client.h"
#include "yojimbo_config.h"
#include "yojimbo_utils.h"

#include <cstdint>
#include <format>
#include <iostream>
#include <string>

void GameAdapter::OnServerClientConnected(int client_index) {
    if (m_server != NULL) {
        m_server->ClientConnected(client_index);
    }
}

void GameAdapter::OnServerClientDisconnected(int client_index) {
    if (m_server != NULL) {
        m_server->ClientDisconnected(client_index);
    }
}

GameServer::GameServer(const yojimbo::Address& address)
    : m_adapter(this), m_server(yojimbo::GetDefaultAllocator(), default_private_key, address, m_connection_config, m_adapter, 0.0) {
    m_server.Start(max_players);
    if (!m_server.IsRunning()) {
        throw std::runtime_error("could not start server at port " + std::to_string(address.GetPort()));
    }

    char buffer[256];
    m_server.GetAddress().ToString(buffer, sizeof(buffer));
    std::cout << "server address is " << buffer << '\n';
}

void GameServer::ClientConnected(int client_index) {
    std::cout << std::format("client {} connected\n", client_index);
}

void GameServer::ClientDisconnected(int client_index) {
    std::cout << std::format("client {} disconnected\n", client_index);
}

void GameServer::Run() {
    float fixed_dt = 1.0f / 60.0f;
    while (m_running) {
        double current_time = yojimbo_time();
        if (m_time <= current_time) {
            Update(fixed_dt);
            m_time += fixed_dt;
        } else {
            yojimbo_sleep(m_time - current_time);
        }
    }
}

void GameServer::Update(float dt) {
    if (!m_server.IsRunning()) {
        m_running = false;
        return;
    }

    m_server.AdvanceTime(m_time);
    m_server.ReceivePackets();
    ProcessMessages();

    m_server.GetNumConnectedClients()

    for (int i = 0; i < max_players; i++) {
        if (m_server.IsClientConnected(i)) {
            m_server.CreateMessage(i, GameMessageType::Snapshot);


    m_server.SendPackets();
}

void GameServer::ProcessMessages() {
    for (int i = 0; i < max_players; i++) {
        if (m_server.IsClientConnected(i)) {
            for (int j = 0; j < m_connection_config.numChannels; j++) {
                yojimbo::Message* message = m_server.ReceiveMessage(i, j);
                while (message != NULL) {
                    ProcessMessage(i, message);
                    m_server.ReleaseMessage(i, message,
                    message = m_server.ReceiveMessage(i, j);
                }
            }
        }
    }
}

void GameServer::ProcessMessage(int client_index, yojimbo::Message* message) {
    switch (message->GetType()) {
    case (int)GameMessageType::Test:
        ProcessTestMessage(client_index, (TestMessage*)message);
        break;
    default:
        break;
    }
}

void GameServer::ProcessTestMessage(int client_index, TestMessage* message) {
    std::cout << std::format("Test message received from the client {} with data: {}\n", client_index, message->m_data);
    auto test_message = (TestMessage*) m_server.CreateMessage(client_index, (int)GameMessageType::Test);
    test_message->m_data = message->m_data;
    m_server.SendMessage(client_index, (int)GameChannel::Reliable, test_message);
}

