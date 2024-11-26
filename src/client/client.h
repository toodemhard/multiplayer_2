#pragma once
#include <yojimbo.h>
#include "game.h"
#include "input.h"
#include "game_server.h"

class OnlineGameScreen {
  public:
    OnlineGameScreen(const yojimbo::Address& server_address);
    ~OnlineGameScreen();
    void Update(float dt, Input::Input input);
    void ProcessMessages();
    void ProcessMessage(yojimbo::Message* message);

    State m_state;

  private:
    GameConnectConfig m_connection_config;
    GameAdapter m_adapter;
    yojimbo::Client m_client;
};

