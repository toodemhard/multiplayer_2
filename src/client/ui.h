#pragma once

#include "input.h"

#include "renderer.h"
#include "font.h"

enum UI_SizeType {
    UI_SizeType_SumContents,
    UI_SizeType_Pixels,
    UI_SizeType_BaseFontRelative,
};

enum RectSide {
    RectSide_Top,
    RectSide_Bottom,
    RectSide_Left,
    RectSide_Right,
    RectSide_Count,
};

enum Axis2 {
    Axis2_X,
    Axis2_Y,
    Axis2_Count,
};

struct UI_Size {
    UI_SizeType type;      
    f32 value;
};

enum UI_PositionType{ 
    UI_PositionType_AutoOffset,
    UI_PositionType_Absolute,
};

struct UI_Position {
    UI_PositionType type;
    f32 value;
};

struct UI_Element {
    i32 value;
    // user config
    const char* text;
    FontID font;
    i32 font_size;

    TextureID image;

    UI_Position semantic_position[Axis2_Count];
    UI_Size semantic_size[Axis2_Count];

    Axis2 grow_axis;

    float4 color;
    UI_Size border[RectSide_Count];
    UI_Size padding[RectSide_Count];

    // computed stuff possibly read only to user
    f32 computed_position[Axis2_Count];
    f32 computed_size[Axis2_Count];
    f32 computed_padding[RectSide_Count];
    f32 computed_border[RectSide_Count];

    UI_Element* first_child;
    UI_Element* last_child;
    UI_Element* next_sibling;
    UI_Element* prev_sibling;
    UI_Element* parent;

    
    bool is_hovered;
};

typedef u64 UI_Index;


// need to have access to last frame state
struct UI_Frame {
    Slice<UI_Element> elements;
    Hashmap hashed_elements;
};

struct UI {
    UI_Frame frame_buffer[2]; //not that kind of frame buffer
    Arena frame_arenas[2]; // plan is to only have reinitializations instead of init and reset pairs

    u32 active_frame;

    Slice<UI_Element*> parent_stack;
    Slice<Font> fonts;
    f32 base_font_size;

    float2 cursor_pos;
};

struct ElementProps {
};

UI_Element* ui_get(UI_Index index);
bool ui_element_signals(UI_Index index);
void ui_set_ctx(UI* _ui);
void ui_init(UI* ui, Arena* arena, Slice<Font> fonts);
void ui_draw(Renderer* renderer, Arena* temp_arena);
UI_Index ui_begin_row(UI_Element element);
void ui_end_row();
void ui_begin(Input::Input* input);
void ui_end(Arena* temp_arena);

constexpr UI_Position position_offset_px(f32 value) {
    return {UI_PositionType_AutoOffset, value};
}

constexpr UI_Size size_px(f32 value) {
    return UI_Size{UI_SizeType_Pixels, value};
}
