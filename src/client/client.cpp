#include "client.h"
#include "game_server.h"
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

void OnlineGameScreen::Update(float dt, Input::Input input) {
    m_client.AdvanceTime(m_client.GetTime() + dt);
    m_client.ReceivePackets();

    if (m_client.IsConnected()) {
        ProcessMessages();

        if (input.key_down(SDL_SCANCODE_SPACE)) {
            TestMessage* message = (TestMessage*)m_client.CreateMessage((int)GameMessageType::Test);
            message->m_data = 42;
            m_client.SendMessage((int)GameChannel::Reliable, message);
        }
    }

    m_client.SendPackets();
}

void OnlineGameScreen::ProcessMessages() {
    for (int i = 0; i < m_connection_config.numChannels; i++) {
        yojimbo::Message* message = m_client.ReceiveMessage(i);
        while (message != NULL) {
            ProcessMessage(message);
            m_client.ReleaseMessage(message);
            message = m_client.ReceiveMessage(i);
        }
    }
}

void ProcessTestMessage(TestMessage* message) {
    std::cout << std::format("Test message received from server with data: {}\n", message->m_data);
}

void OnlineGameScreen::ProcessMessage(yojimbo::Message* message) {
    switch (message->GetType()) {
    case (int)GameMessageType::Test:
        ProcessTestMessage((TestMessage*)message);
        break;
    case (int)GameMessageType::Snapshot:
        m_state = ((Snapshot*)message)->state;
    default:
        break;
    }
}
