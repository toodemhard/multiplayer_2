#include "client.h"
#include "EASTL/internal/move_help.h"
#include "game_server.h"
#include <chrono>
#include <format>
#include <iostream>

OnlineGameScreen::OnlineGameScreen(const yojimbo::Address& server_address)
    : m_client(yojimbo::GetDefaultAllocator(), yojimbo::Address("0.0.0.0"), m_connection_config, m_adapter, 0.0) {
    uint64_t client_id;
    yojimbo_random_bytes((uint8_t*)&client_id, 8);
    m_client.InsecureConnect(default_private_key, client_id, server_address);
}

OnlineGameScreen::~OnlineGameScreen() {
    m_client.Disconnect();
}

void OnlineGameScreen::Update(float dt, PlayerInput input, Input::Input& input_2) {
    m_client.AdvanceTime(m_client.GetTime() + dt);
    m_client.ReceivePackets();

    if (m_client.IsConnected()) {
        for (int i = 0; i < m_connection_config.numChannels; i++) {
            yojimbo::Message* message = m_client.ReceiveMessage(i);
            while (message != NULL) {
                ProcessMessage(message);
                m_client.ReleaseMessage(message);
                message = m_client.ReceiveMessage(i);
            }
        }
        // printf("%d %d %d %d\n", input.up, input.down, input.left, input.right);

        auto message = (InputMessage*)m_client.CreateMessage((int)GameMessageType::Input);
        message->input = input;
        m_client.SendMessage((int)GameChannel::Reliable, message);

        

        if (input_2.key_down(SDL_SCANCODE_SPACE)) {
            TestMessage* message = (TestMessage*)m_client.CreateMessage((int)GameMessageType::Test);
            message->m_data = 42;
            m_client.SendMessage((int)GameChannel::Unreliable, message);

            m_packet_sent_time = std::chrono::high_resolution_clock::now();
        }
    }

    m_client.SendPackets();
}

void OnlineGameScreen::ProcessMessages() {
}

void ProcessTestMessage(TestMessage* message) {
    std::cout << std::format("Test message received from server with data: {}\n", message->m_data);
}

void OnlineGameScreen::ProcessMessage(yojimbo::Message* message) {
    switch (message->GetType()) {
    case (int)GameMessageType::Test:
        std::cout << std::format("ping: {}\n", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_packet_sent_time).count());
        ProcessTestMessage((TestMessage*)message);
        break;
    case (int)GameMessageType::Snapshot:
        m_state = eastl::move(((SnapshotMessage*)message)->state);
        // printf("snapshot\n");
        // printf("snapshot: %f %f\n", m_pos.x, m_pos.y);
        // for (auto pos : m_state.players) {
        //     printf("%f %f", pos.x, pos.y);
        // }
    default:
        break;
    }
}
