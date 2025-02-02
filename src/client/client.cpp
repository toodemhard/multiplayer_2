#include "../pch.h"

#include "imgui.h"

#include "client.h"

GameClient::GameClient()
    : m_adapter(this), m_client(yojimbo::GetDefaultAllocator(), yojimbo::Address("0.0.0.0"), m_connection_config, m_adapter, 0.0) {

    // m_client.SetLatency(100);
    // m_client.SetJitter(5);
    // m_client.SetPacketLoss(1);
}

void GameClient::connect(const yojimbo::Address& server_address) {
    uint64_t client_id;
    yojimbo_random_bytes((uint8_t*)&client_id, 8);

    m_client.InsecureConnect(default_private_key, client_id, server_address);
}

GameClient::~GameClient() {
    m_client.Disconnect();
}

void GameClient::ClientConnected(int client_index) {}

void GameClient::ClientDisconnected(int client_index) {}

void GameClient::update() {
    yojimbo::NetworkInfo net_info;
    m_client.GetNetworkInfo(net_info);
    ImGui::Begin("net debug");
    ImGui::Text("RTT: %fms", net_info.RTT);
    ImGui::Text("Jitter: %f", net_info.averageJitter);
    ImGui::Text("Loss: %f%%", net_info.packetLoss);
    ImGui::Text("Up Bandwidth: %fkbps", net_info.sentBandwidth);
    ImGui::Text("Down Bandwidth: %fkbps", net_info.receivedBandwidth);

    ImGui::Text("Current Tick: %d", m_current_tick);

    ImGui::End();
}

void GameClient::fixed_update(float dt, PlayerInput input, Input::Input& input_2, int* throttle_ticks) {
    m_client.AdvanceTime(m_client.GetTime() + dt);
    m_client.ReceivePackets();

    if (m_client.IsConnected()) {
        for (int i = 0; i < m_connection_config.numChannels; i++) {
            yojimbo::Message* message = m_client.ReceiveMessage(i);
            while (message != NULL) {
                switch (message->GetType()) {
                case (int)GameMessageType::Init:
                    m_current_tick = ((InitMessage*)message)->current_tick;
                    break;
                case (int)GameMessageType::Test:
                    // std::cout << std::format("ping: {}\n",
                    // std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() -
                    // m_packet_sent_time).count());
                    std::cout << std::format("Test message received from server with data: {}\n", ((TestMessage*)message)->m_data);
                    break;
                case (int)GameMessageType::Snapshot:
                    m_state = std::move(((SnapshotMessage*)message)->state);
                    // printf("snapshot\n");
                    // printf("snapshot: %f %f\n", m_pos.x, m_pos.y);
                    // for (auto pos : m_state.players) {
                    //     printf("%f %f", pos.x, pos.y);
                    // }
                    break;
                case (int)GameMessageType::ThrottleInfo: {
                    auto throttle_info = (ThrottleInfo*)message;
                    // printf("thing: %d\n", throttle_info->input_buffer_size);
                    *throttle_ticks = target_input_buffer_size - throttle_info->input_buffer_size;

                } break;
                default:
                        break;
                }
                m_client.ReleaseMessage(message);
                message = m_client.ReceiveMessage(i);
            }
        }
        // printf("%d %d %d %d\n", input.up, input.down, input.left, input.right);


        auto message = (InputMessage*)m_client.CreateMessage((int)GameMessageType::Input);
        message->input = input;
        message->tick = m_current_tick;
        m_client.SendMessage((int)GameChannel::Reliable, message);

        if (input_2.key_down(SDL_SCANCODE_SPACE)) {
            TestMessage* message = (TestMessage*)m_client.CreateMessage((int)GameMessageType::Test);
            message->m_data = 42;
            m_client.SendMessage((int)GameChannel::Unreliable, message);

            m_packet_sent_time = std::chrono::high_resolution_clock::now();
        }

        // if (defer_ack){
        //     auto message = (ThrottleComplete*)m_client.CreateMessage((int)GameMessageType::ThrottleComplete);
        //     m_client.SendMessage((int)GameChannel::Reliable, message);
        // }

        m_current_tick++;
    }

    m_client.SendPackets();


}

void GameClient::ProcessMessages() {}

void ProcessTestMessage(TestMessage* message) {
    std::cout << std::format("Test message received from server with data: {}\n", message->m_data);
}

void GameClient::ProcessMessage(yojimbo::Message* message) {
    switch (message->GetType()) {
    case (int)GameMessageType::Test:
        std::cout << std::format(
            "ping: {}\n",
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_packet_sent_time).count()
        );
        ProcessTestMessage((TestMessage*)message);
        break;
    case (int)GameMessageType::Snapshot:
        m_state = std::move(((SnapshotMessage*)message)->state);
        // printf("snapshot\n");
        // printf("snapshot: %f %f\n", m_pos.x, m_pos.y);
        // for (auto pos : m_state.players) {
        //     printf("%f %f", pos.x, pos.y);
        // }
        break;
        // case (int)GameMessageType::ThrottleCommand:
        //     printf("thing: %d\n", ((ThrottleCommand*)message)->ticks);
        //
        //     auto message = (ThrottleComplete*)m_client.CreateMessage((int)GameMessageType::ThrottleComplete);
        // m_client.SendMessage(int channelIndex, Message* message)

    default:
        break;
    }
}
