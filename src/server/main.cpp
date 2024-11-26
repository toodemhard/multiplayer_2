#include "yojimbo.h"
#include "game_server.h"
#include <iostream>

int main() {
    InitializeYojimbo();


    GameServer server(yojimbo::Address(127, 0, 0, 1, 5000));
    server.Run();
}
