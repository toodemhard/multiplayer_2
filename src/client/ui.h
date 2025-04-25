#pragma once

typedef enum UI_SizeType {
    UI_SizeType_Fit,
    UI_SizeType_Pixels,
    UI_SizeType_BaseFontRelative,
    UI_SizeType_ParentFraction,
    UI_SizeType_ImageProportional,
    UI_SizeType_Grow,
} UI_SizeType;

typedef enum RectSide {
    RectSide_Left,
    RectSide_Right,
    RectSide_Top,
    RectSide_Bottom,
    RectSide_Count,
} RectSide;

typedef enum Axis2 {
    Axis2_X,
    Axis2_Y,
    Axis2_Count,
} Axis2;

typedef struct UI_Size {
    UI_SizeType type;      
    f32 value;
} UI_Size;

typedef enum UI_PositionType { 
    UI_PositionType_AutoOffset,
    UI_PositionType_Absolute,
    UI_PositionType_Anchor,
} UI_PositionType;

typedef struct UI_Position {
    UI_PositionType type;
    f32 value;
} UI_Position;

typedef enum UI_Flags {
    UI_Flags_Float = 1 << 0,
} UI_Flags;

typedef enum FontSizeType {
    FontSizeType_Default,
    FontSizeType_Pixels,
} FontSizeType;

typedef struct FontSize {
    FontSizeType type;
    f32 value;
} FontSize;

typedef u64 UI_Key;
hashmap_def(UI_Key, u32);

typedef struct UI_Element UI_Element;
struct UI_Element {
    // user config
    UI_Key* out_key;
    Slice_u8 exc_key; // replaces source key to be able to id key in multiple places
    Slice_u8 salt_key; // adds to source key
    UI_Flags flags;
    const char* text;
    FontID font;
    FontSize font_size;
    ImageID image;
    Rect image_src;

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
slice_def(UI_Element);
slice_p_def(UI_Element);


// need to have access to last frame state
typedef struct UI_Frame {
    Slice_UI_Element elements;
} UI_Frame;

typedef struct UI UI;
struct UI {
    UI_Frame frame_buffer[2]; //not that kind of frame buffer
    Arena frame_arenas[2]; // plan is to only have reinitializations instead of init and reset pairs

    // values should be index to last frame elements probably
    Hashmap_UI_Key_u32 cache;
    // Slice<UI_Element> element_cache;

    u32 active_frame;

    Slice_pUI_Element parent_stack;
    Slice_Font fonts;
    f32 base_size;

    UI_Key active_element;

    Renderer* renderer;
};

UI_Key ui_key(Slice_u8 key);
UI_Element* ui_get(UI_Key index);
bool ui_hover(UI_Key index);
void ui_set_ctx(UI* _ui);
void ui_init(UI* ui, Arena* arena, Slice_Font fonts, Renderer* renderer);
void ui_draw(UI* ui_ctx);
UI_Key ui_push_row_internal(UI_Element element, const char* file, i32 line);
UI_Key ui_pop_row();
void ui_begin();
bool ui_is_active(UI_Key element);
bool ui_button(UI_Key element);
// UI_Key ui_push_leaf(UI_Element element);
void ui_end(Arena* temp_arena);
UI_Element* ui_prev_element();

inline UI_Position position_offset_px(f32 value) {
    return (UI_Position){UI_PositionType_AutoOffset, value};
}

inline UI_Size size_px(f32 value) {
    return (UI_Size){UI_SizeType_Pixels, value};
}

inline UI_Size size_grow() {
    return (UI_Size){UI_SizeType_Grow};
}

inline UI_Size size_proportional() {
    return (UI_Size){UI_SizeType_ImageProportional};
}

FontSize font_px(f32 value) {
    return (FontSize) {
        FontSizeType_Pixels,
        value,
    };
}

UI_Position anchor(f32 value) {
    return (UI_Position){UI_PositionType_Anchor, value};
}

// relative units
f32 ru(f32 value);

inline UI_Size size_ru(f32 value) {
    return size_px(ru(value));
}

inline FontSize font_ru(f32 value) {
    return (FontSize){FontSizeType_Pixels, ru(value)};
}

#define size2_ru(v0, v1) {size_ru(v0), size_ru(v1)}
#define size2_px(v0, v1) {size_px(v0), size_px(v1)}

#define axis2(v) { v, v }

#define pos_anchor2(v0, v1) {anchor(v0), anchor(v1)}

#define sides(v) {v, v, v ,v}
#define sides2(v1, v2) {v1, v1, v2 ,v2}

#define sides_px(v) {size_px(v), size_px(v), size_px(v), size_px(v)}
#define sides2_px(v1, v2) {size_px(v1), size_px(v1), size_px(v2), size_px(v2)}


// i32 i = (printf("fakdhf\n"), 123);
// comma operator runs multiple expressions and return last although = is higher precedence

#define ui_push_row(...)\
ui_push_row_internal((UI_Element)__VA_ARGS__, __FILE__, __LINE__)

#define ui_row(...)\
defer_loop(ui_push_row(__VA_ARGS__), ui_pop_row())\

#define ui_row_ret(...)\
(ui_push_row(__VA_ARGS__), ui_pop_row())
