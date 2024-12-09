#include "EASTL/fixed_vector.h"
#include "imgui.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"
#include "game.h"
#include "server.h"
#include "glm/detail/qualifier.hpp"
#include "net_common.h"
#include "yojimbo_allocator.h"
#include "yojimbo_client.h"
#include "yojimbo_config.h"
#include "yojimbo_utils.h"

#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <string>


GameServer::GameServer(const yojimbo::Address& address)
    : m_adapter(this), m_server(yojimbo::GetDefaultAllocator(), default_private_key, address, m_connection_config, m_adapter, 0.0) {
    m_server.Start(max_player_count);
    if (!m_server.IsRunning()) {
        throw std::runtime_error("could not start server at port " + std::to_string(address.GetPort()));
    }

    char buffer[256];
    m_server.GetAddress().ToString(buffer, sizeof(buffer));
    std::cout << "server address is " << buffer << '\n';
}

void GameServer::ClientConnected(int client_index) {
    std::cout << std::format("client {} connected\n", client_index);

    auto message = (InitMessage*)m_server.CreateMessage(client_index, (int)GameMessageType::Init);
    message->frame = m_frame;
    m_server.SendMessage(client_index, (int)GameChannel::Reliable, message);
    m_server.SendPackets();

    // auto message = (SnapshotMessage*)m_server.CreateMessage(client_index, (int)GameMessageType::Snapshot);
    create_player(m_state);
    // m_server.SendMessage(client_index, (int)GameChannel::Unreliable, message);
    // m_server.SendPackets();

    // m_state.players.push_back({1,0});
}

void GameServer::ClientDisconnected(int client_index) {
    std::cout << std::format("client {} disconnected\n", client_index);
    remove_player(m_state, client_index);
}

#define GUI

void GameServer::Run() {
#ifdef GUI
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_CreateWindowAndRenderer("server", 640, 480, 0, &window, &renderer);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    glm::vec<4, int> clear_color(0,0,0,255);
#endif

    double fixed_dt = 1.0 / tick_rate;
    auto last_frame_time = std::chrono::high_resolution_clock::now();
    double accumulator = 0.0;

    uint32_t current_tick = 0;
    double time = 0.0;

    bool quit = false;
    while (!quit) {
        auto this_frame_time = std::chrono::high_resolution_clock::now();
        double delta_time = std::chrono::duration_cast<std::chrono::duration<double>>(this_frame_time - last_frame_time).count();
        // double delta_time = duration.count();
        accumulator += delta_time;
        last_frame_time = this_frame_time;
        // printf("frame:%d %fs\n", frame, time);

#ifdef GUI
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                quit = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                quit = true;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
#endif

        while (accumulator >= fixed_dt) {
            current_tick++;
            accumulator -= fixed_dt;
            time += fixed_dt;
            // printf("frame:%d %fs\n", frame, time);
            // printf("update\n");

            Update(time, fixed_dt);
            m_time += fixed_dt;

            // printf("tick\n");
        }

#ifdef GUI
        ImGui::Begin("net info");
        ImGui::Text("current tick: %d", current_tick);
        ImGui::End();
#endif

#ifdef GUI
        ImGui::Render();
        //SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColorFloat(renderer, clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
#endif
    }
    m_server.Stop();

#ifdef GUI
    // Cleanup
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif
}


void GameServer::Update(double time, double dt) {
    // if (!m_server.IsRunning()) {
    //     m_running = false;
    //     return;
    // }

    int inputs_this_frame = 0;

    m_server.AdvanceTime(m_time + dt);
    m_server.ReceivePackets();

    PlayerInput inputs[max_player_count]{};
    // ProcessMessages();
    for (int client_index = 0; client_index < max_player_count; client_index++) {
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
                        // m_input_buffer.push_back()
                        // printf("frame %d: %d %d %d %d %d\n", frame, input.up, input.down, input.left, input.right, input.fire);
                        // if (input.fire) {
                        //     printf("askdlfh\n");
                        //     printf("askdlfh\n");
                        //     printf("askdlfh\n");
                        //     printf("askdlfh\n");
                        //     printf("askdlfh\n");
                        //     printf("askdlfh\n");
                        // }
                        auto& input_buffer = m_input_buffers[client_index];
                        // if (!input_buffer.full()) {
                        input_buffer.push_back(input);
                        inputs_this_frame++;
                        // }

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


            auto& input_buffer = m_input_buffers[client_index];
            // if (input_buffer.empty()) {
            //     continue;
            // }

            if (!input_buffer.empty()) {
                inputs[client_index] = input_buffer.front();
                input_buffer.pop_front();
                // printf("input_pop: %d\n", (int)input_buffer.size());
            }
            printf("received: %d, input buffer: %d\n", inputs_this_frame, (int)input_buffer.size());
            {
                auto message = (ThrottleInfo*)m_server.CreateMessage(client_index, (int)GameMessageType::ThrottleInfo);
                message->input_buffer_size = input_buffer.size();
                m_server.SendMessage(client_index, (int)GameChannel::Unreliable, message);
                m_server.SendPackets();
            }

        }

        // update(m_state, inputs, dt);

        m_frame++;
    }
    // m_server.ReceiveMessage(i,)

    // printf("input buffer: %d\n", (int)m_input_buffer.size());
    // for (int i = 0; i < inputs.size(); i++) {
    //     printf("frame: %d, input buffer size: %d\n", frame, (int)m_input_buffers[i].size());
    //     if (m_input_buffers[i].empty()) {
    //         continue;
    //     }
    //
    //     inputs[i] = m_input_buffers[i].back();
    //     m_input_buffers[i].pop_back();
    // }

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
    for (int i = 0; i < max_player_count; i++) {
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
