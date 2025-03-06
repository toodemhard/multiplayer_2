#pragma once

#include "input.h"

#include "renderer.h"
#include "font.h"

enum UI_SizeType {
    UI_SizeType_Fit,
    UI_SizeType_Pixels,
    UI_SizeType_BaseFontRelative,
    UI_SizeType_ParentFraction,
    UI_SizeType_ImageProportional,
    UI_SizeType_Grow,
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

enum FontSizeType {
    FontSizeType_Default,
    FontSizeType_Pixels,
};

struct FontSize {
    FontSizeType type;
    f32 value;
};

typedef u64 UI_Key;

struct UI_Element {
    // user config
    UI_Key* out_key;
    Slice<u8> exc_key; // replaces source key to be able to id key in multiple places
    Slice<u8> salt_key; // adds to source key
    UI_Flags flags;
    const char* text;
    FontID font;
    FontSize font_size;
    ImageID image;
    Axis2 stack_axis;

    UI_Position position[Axis2_Count];
    UI_Size size[Axis2_Count];
    UI_Size border[RectSide_Count];
    UI_Size padding[RectSide_Count];
    float4 background_color;
    float4 border_color;
    float4 font_color;

    // computed stuff possibly read only to user
    UI_Key computed_key;
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


// need to have access to last frame state
struct UI_Frame {
    Slice<UI_Element> elements;
};

struct UI {
    UI_Frame frame_buffer[2]; //not that kind of frame buffer
    Arena frame_arenas[2]; // plan is to only have reinitializations instead of init and reset pairs

    // values should be index to last frame elements probably
    Hashmap<u64, u32> cache;
    // Slice<UI_Element> element_cache;

    u32 active_frame;

    Slice<UI_Element*> parent_stack;
    Slice<Font> fonts;
    f32 base_font_size;

    UI_Key active_element;

    Renderer* renderer;
};

struct ElementProps {
};

UI_Key ui_key(Slice<u8> key);
UI_Element* ui_get(UI_Key index);
bool ui_hover(UI_Key index);
void ui_set_ctx(UI* _ui);
void ui_init(UI* ui, Arena* arena, const Slice<Font> fonts, Renderer* renderer);
void ui_draw(UI* ui_ctx, Arena* temp_arena);
UI_Key ui_push_row_internal(UI_Element element, const char* file, i32 line);
UI_Key ui_pop_row();
void ui_begin();
bool ui_is_active(UI_Key element);
bool ui_button(UI_Key element);
// UI_Key ui_push_leaf(UI_Element element);
void ui_end(Arena* temp_arena);

constexpr UI_Position position_offset_px(f32 value) {
    return {UI_PositionType_AutoOffset, value};
}

constexpr UI_Size size_px(f32 value) {
    return UI_Size{UI_SizeType_Pixels, value};
}

constexpr UI_Size size_grow() {
    return UI_Size{UI_SizeType_Grow};
}

constexpr UI_Size size_proportional() {
    return UI_Size{UI_SizeType_ImageProportional};
}

constexpr FontSize font_px(f32 value) {
    return FontSize {
        FontSizeType_Pixels,
        value,
    };
}

#define sides_px(v) {size_px(v), size_px(v), size_px(v), size_px(v)}

#define sides2_px(v1, v2) {size_px(v1), size_px(v1), size_px(v2), size_px(v2)}

// https://github.com/EpicGamesExt/raddebugger/blob/master/src/base/base_core.h
#define DeferLoop(begin, end) for(int _i_ = ((begin), 0); !_i_; _i_ += 1, (end))

// i32 i = (printf("fakdhf\n"), 123);
// comma operator runs multiple expressions and return last although = is higher precedence

#define ui_push_row(...)\
ui_push_row_internal(__VA_ARGS__, __FILE__, __LINE__)

#define ui_row(...)\
DeferLoop(ui_push_row(__VA_ARGS__), ui_pop_row())\

#define ui_row_ret(...)\
(ui_push_row(__VA_ARGS__), ui_pop_row())
