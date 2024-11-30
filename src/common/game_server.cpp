#include "game_server.h"
#include "EASTL/fixed_vector.h"
#include "game.h"
#include "yojimbo_adapter.h"
#include "yojimbo_allocator.h"
#include "yojimbo_client.h"
#include "yojimbo_config.h"
#include "yojimbo_utils.h"

#include <chrono>
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

    auto message = (SnapshotMessage*)m_server.CreateMessage(client_index, (int)GameMessageType::Snapshot);
    message->state.players = {{0.5, 0.5}, {0, 0}, {-1, 0}};
    m_server.SendMessage(client_index, (int)GameChannel::Unreliable, message);
    m_server.SendPackets();

    // m_state.players.push_back({1,0});
}

void GameServer::ClientDisconnected(int client_index) {
    std::cout << std::format("client {} disconnected\n", client_index);
}

void GameServer::Run() {
    double fixed_dt = 1.0 / 128.0;
    auto last_frame_time = std::chrono::high_resolution_clock::now();
    double accumulator = 0.0;

    uint32_t frame = 0;
    double time = 0.0;

    while (1) {
        auto this_frame_time = std::chrono::high_resolution_clock::now();
        double delta_time = std::chrono::duration_cast<std::chrono::duration<double>>(this_frame_time - last_frame_time).count();
        // double delta_time = duration.count();
        accumulator += delta_time;
        last_frame_time = this_frame_time;
        printf("frame:%d %fs\n", frame, time);

        while (accumulator >= fixed_dt) {
            frame++;
            accumulator -= fixed_dt;
            time += fixed_dt;
            printf("frame:%d %fs\n", frame, time);
            // printf("update\n");

            Update(time, fixed_dt);
            m_time += fixed_dt;
        }

    }
}

int frame = 0;

void GameServer::Update(double time, double dt) {
    // if (!m_server.IsRunning()) {
    //     m_running = false;
    //     return;
    // }

    m_server.AdvanceTime(m_time);
    m_server.ReceivePackets();

    eastl::fixed_vector<PlayerInput, 16> inputs(m_server.GetNumConnectedClients(), {});
    // ProcessMessages();
    for (int client_index = 0; client_index < max_players; client_index++) {
        if (m_server.IsClientConnected(client_index)) {
            for (int channel = 0; channel < m_connection_config.numChannels; channel++) {
                yojimbo::Message* message = m_server.ReceiveMessage(client_index, channel);
                while (message != NULL) {
                    switch (message->GetType()) {
                    case (int)GameMessageType::Test: {
                        auto test_message = (TestMessage*)message;
                        std::cout << std::format(
                            "Test message received from the client {} with data: {}\n", client_index, test_message->m_data
                        );
                        auto return_test_message = (TestMessage*)m_server.CreateMessage(client_index, (int)GameMessageType::Test);
                        return_test_message->m_data = test_message->m_data;
                        m_server.SendMessage(client_index, (int)GameChannel::Reliable, return_test_message);
                    } break;
                    case (int)GameMessageType::Input: {
                        inputs[client_index] = ((InputMessage*)message)->input;
                        auto& input = inputs[client_index];
                        printf("frame %d: %d %d %d %d %d\n", frame, input.up, input.down, input.left, input.right, input.fire);
                        if (input.fire) {
                            printf("askdlfh\n");
                            printf("askdlfh\n");
                            printf("askdlfh\n");
                            printf("askdlfh\n");
                            printf("askdlfh\n");
                            printf("askdlfh\n");
                        }
                        // auto& input = inputs[client_index];
                        // printf("%d %d %d %d\n", input.up, input.down, input.left, input.right);
                    } break;
                    default:
                        break;
                    }
                    m_server.ReleaseMessage(client_index, message);
                    message = m_server.ReceiveMessage(client_index, channel);
                }
            }

            auto message = (SnapshotMessage*)m_server.CreateMessage(client_index, (int)GameMessageType::Snapshot);
            message->state = m_state;
            m_server.SendMessage(client_index, (int)GameChannel::Unreliable, message);
            m_server.SendPackets();
        }

        // update(m_state, inputs, dt);

        frame++;
    }
    // m_server.ReceiveMessage(i,)

    update(m_state, inputs, time, dt);

    // for (int i = 0; i < max_players; i++) {
    //     if (m_server.IsClientConnected(i)) {
    //         auto snapshot = (SnapshotMessage*)m_server.CreateMessage(i, (int)GameMessageType::Snapshot);
    //         snapshot->state = m_state;
    //         m_server.SendMessage(i, (int)GameChannel::Unreliable, snapshot);
    //         m_server.ReleaseMessage(i, snapshot);
    //     }
    // }

    m_server.SendPackets();
}

void GameServer::ProcessMessages() {
    for (int i = 0; i < max_players; i++) {
        if (m_server.IsClientConnected(i)) {
            for (int j = 0; j < m_connection_config.numChannels; j++) {
                yojimbo::Message* message = m_server.ReceiveMessage(i, j);
                while (message != NULL) {
                    ProcessMessage(i, message);
                    m_server.ReleaseMessage(i, message);
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
        // case (int)GameMessageType::Input:
        //     auto input = ((InputMessage*)message)->input

    default:
        break;
    }
}

void GameServer::ProcessTestMessage(int client_index, TestMessage* message) {
    std::cout << std::format("Test message received from the client {} with data: {}\n", client_index, message->m_data);
    auto test_message = (TestMessage*)m_server.CreateMessage(client_index, (int)GameMessageType::Test);
    test_message->m_data = message->m_data;
    m_server.SendMessage(client_index, (int)GameChannel::Reliable, test_message);
}
