#pragma once

struct DLL_State {

};

struct Control {
    bool quit;
    bool reload;
};

#define INIT(name) void name(DLL_State* state)
typedef INIT(init_func);
extern "C" INIT(init);


#define UPDATE(name) Control name(DLL_State*)
typedef UPDATE(update_func);
UPDATE(update);
