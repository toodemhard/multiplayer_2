﻿#pragma once

#include "input.h"

#include "renderer.h"
#include "font.h"

enum UI_SizeType {
    UI_SizeType_SumContent,
    UI_SizeType_Pixels,
    UI_SizeType_ParentFraction,
    UI_SizeType_ImageProportional,
    UI_SizeType_BaseFontRelative,
};

enum RectSide {
    RectSide_Left,
    RectSide_Right,
    RectSide_Top,
    RectSide_Bottom,
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

enum UI_PositionType { 
    UI_PositionType_AutoOffset,
    UI_PositionType_Absolute,
};

struct UI_Position {
    UI_PositionType type;
    f32 value;
};

enum UI_Flags {
    UI_Flags_Float = 1 << 0,
};

struct UI_Element {
    // user config
    UI_Flags flags;
    const char* text;
    FontID font;
    i32 font_size;
    ImageID image;
    Axis2 grow_axis;

    UI_Position position[Axis2_Count];
    UI_Size size[Axis2_Count];
    UI_Size border[RectSide_Count];
    UI_Size padding[RectSide_Count];
    float4 background_color;
    float4 border_color;
    float4 font_color;

    // computed stuff possibly read only to user
    float2 computed_position;
    float2 computed_size;
    f32 computed_padding[RectSide_Count];
    f32 computed_border[RectSide_Count];
    float2 content_position;

    float2 next_position;

    UI_Element* first_child;
    UI_Element* last_child;
    UI_Element* next_sibling;
    UI_Element* prev_sibling;
    UI_Element* parent;

    
    bool is_hovered;
};


typedef u64 UI_Key;


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

    Renderer* renderer;
};

struct ElementProps {
};

UI_Element* ui_get(UI_Key index);
bool ui_hover(UI_Key index);
void ui_set_ctx(UI* _ui);
void ui_init(UI* ui, Arena* arena, Slice<Font> fonts, Renderer* renderer);
void ui_draw(UI* ui_ctx, Renderer* renderer, Arena* temp_arena);
UI_Key ui_push_row(UI_Element element);
void ui_pop_row();
void ui_begin(Input::Input* input);
UI_Key ui_push_leaf(UI_Element element);
void ui_end(Arena* temp_arena);

constexpr UI_Position position_offset_px(f32 value) {
    return {UI_PositionType_AutoOffset, value};
}

constexpr UI_Size size_px(f32 value) {
    return UI_Size{UI_SizeType_Pixels, value};
}
constexpr UI_Size size_proportional() {
    return UI_Size{UI_SizeType_ImageProportional};
}

#define sides_px(v) {size_px(v), size_px(v), size_px(v), size_px(v)}

#define sides2_px(v1, v2) {size_px(v1), size_px(v1), size_px(v2), size_px(v2)}
