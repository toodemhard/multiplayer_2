#include "EASTL/allocator.h"
#include "EASTL/bonus/fixed_ring_buffer.h"
#include "EASTL/bonus/ring_buffer.h"
#include "EASTL/fixed_allocator.h"
#include "net_common.h"
#include "yojimbo.h"
#include "server.h"
#include <iostream>
#include <EASTL/array.h>

int main() {
    // ring_buffer<int, 5> stuff{};
    // stuff.push_back(12);
    // stuff.push_back(12);
    // stuff.push_back(12);
    // stuff.pop_front();
    // stuff.pop_front();
    // stuff.push_back(5);
    // stuff.push_back(5);
    // stuff.push_back(5);
    // stuff.push_back(5);
    // stuff.pop_front();
    // stuff.pop_front();
    // stuff.push_back(90);
    // stuff.pop_front();
    // stuff.pop_front();
    // stuff.pop_front();
    // stuff.pop_front();
    // stuff.pop_front();

    // eastl::fixed_ring_buffer<typename T, size_t N>
    // eastl::ring_buffer<int, eastl::vector<int>> asdf;

    // eastl::ring_buffer<int, eastl::array<int,5>, eastl::fixed_allocator> stuff;
    // stuff.push_back(123);
    // stuff.push_back(2);
    // stuff.push_back(65);
    // stuff.push_back(123);
    // stuff.push_back(2);
    // stuff.push_back(65);
    // stuff.pop_front();
    // stuff.pop_front();
    // stuff.pop_front();
    InitializeYojimbo();

    GameServer server(server_address);
    server.Run();

}
