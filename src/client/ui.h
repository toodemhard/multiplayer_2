#pragma once

#include "renderer.h"
#include "font.h"

enum UI_SizeType {
    UI_SizeType_SumContents,
    UI_SizeType_Pixels,
    UI_SizeType_BaseFontRelative,
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

struct Element {
    i32 value;
    // user config
    const char* text;
    UI_Position semantic_position[Axis2_Count];
    UI_Size semantic_size[Axis2_Count];

    Axis2 grow_axis;

    float4 color;

    // dont touch
    f32 computed_position[Axis2_Count];
    f32 computed_size[Axis2_Count];
    f32 computed_padding[Axis2_Count];

    Element* first_child;
    Element* last_child;
    Element* next_sibling;
    Element* prev_sibling;
    Element* parent;

    f32 padding[Axis2_Count];
};

struct Vec2 {
    f32 x, y;
};

struct UI {
    Slice<Element> elements;
    Slice<Element*> parent_stack;

    Vec2 start_position;

    f32 base_font_size;
};

struct ElementProps {
};

void ui_set_ctx(UI* _ui);
void ui_init(UI* ui, Arena* arena);
void ui_draw(Renderer* renderer, Arena* temp_arena);
void ui_begin_row(Element element);
void ui_end_row();
void ui_begin();
void ui_end(Arena* temp_arena);

constexpr UI_Position position_offset_px(f32 value) {
    return {UI_PositionType_AutoOffset, value};
}

constexpr UI_Size size_px(f32 value) {
    return UI_Size{UI_SizeType_Pixels, value};
}
// #include "color.h"
// #include "font.h"
// #include "input.h"
//
// #include "renderer.h"


// constexpr int string_array_capacity{10};
//
// class StringCache {
//   public:
//     const char* add(std::string&& string);
//     void clear();
//
//   private:
//     std::array<std::string, string_array_capacity> strings;
//     int back = 0;
// };
//
// enum class StackDirection {
//     Horizontal,
//     Vertical,
// };
//
// struct Padding {
//     float left;
//     float right;
//     float top;
//     float bottom;
// };
//
// inline Padding even_padding(float amount) {
//     return {amount, amount, amount, amount};
// }
//
// namespace Position {
//
// // fraction of the parent bounds
// struct Anchor {
//     glm::vec2 position;
// };
//
// struct Absolute {
//     glm::vec2 position;
// };
//
// struct Relative {
//     glm::vec2 offset_position;
// };
//
// using Variant = std::variant<Relative, Anchor, Absolute>;
// } // namespace Position
//
// namespace Scale {
//
// struct Fixed {
//     float value;
// };
//
// struct Min {
//     float value;
// };
//
// struct FitParent {};
//
// // scale to the content
// struct FitContent {};
//
// using Variant = std::variant<FitContent, Min, Fixed, FitParent>;
//
// } // namespace Scale
//
// struct Inherit {};
//
// using TextColor = std::variant<RGBA, Inherit>;
//
// enum class TextAlign {
//     Left,
//     Center,
//     Right,
// };
//
// enum class Alignment {
//     Start,
//     Center,
//     End,
// };
//
// enum class TextWrap {
//     Overflow,
//     Cutoff,
//     Wrap,
// };
//
// enum class JustifyItems {
//     start,
//     apart,
// };
//
// struct Style {
//     Position::Variant position;
//     RGBA background_color;
//     RGBA border_color;
//     // RGBA border_color = color::red;
//
//     Padding padding;
//
//     Scale::Variant width;
//     Scale::Variant height;
//     bool overlap;
//
//     // row styling
//     StackDirection stack_direction = StackDirection::Horizontal;
//     Alignment align_items;
//     JustifyItems justify_items;
//     
//     float gap;
//
//     // text style
//     float font_size = 36;
//     TextColor text_color = color::white;
//     TextWrap text_wrap;
//     TextAlign text_align;
// };
//
// struct AnimState {
//     bool target_hover;
//     float mix;
// };
//
// struct AnimStyle {
//     RGBA alt_text_color = color::white;
//     RGBA alt_background_color;
//
//     float duration;
// };
//
// // struct Rect {
// //     glm::vec2 position;
// //     glm::vec2 scale;
// // };
//
// struct DrawRect {
//     Rect rect;
//     RGBA background_color;
//     RGBA border_color;
// };
//
//
// struct Text {
//     glm::vec2 position;
//     const char* text;
//     float font_size;
//     RGBA text_color;
//     TextWrap wrap
//     TextAlign text_align;
//     float max_width;
// };
//
// struct Button {
//     int rect_index;
//     std::function<void()> on_click;
// };
//
// enum class Command {
//     begin_row,
//     end_row,
//     text
// };
//
// struct ClickInfo {
//     glm::vec2 offset_pos;
//     glm::vec2 scale;
// };
//
// using OnClick = std::function<void()>;
//
// struct Row {
//     int rect_index;
//     Style style;
//
//     AnimState* anim_state;
//     int children;
// };
//
// struct ClickRect {
//     int rect_index;
//     int on_click_index;
// };
//
// struct SliderHeldRect {
//     int rect_index;
//     int on_held_index;
// };
//
// struct HoverRect {
//     int rect_index;
//     AnimState& anim_state;
// };
//
// struct TextFieldState {
//     std::string text;
//     bool focused;
// };
//
// struct TextFieldInputRect {
//     TextFieldState* state;
//     int rect_index;
// };
//
// struct Slider {
//     bool held;
// };
//
// struct SliderStyle {
//     Position::Variant position;
//     float width;
//     float height;
//     RGBA bg_color;
//     RGBA fg_color;
//     RGBA border_color;
// };
//
// struct SliderCallbacks {
//     std::function<void(float)> on_input;
//     std::optional<std::function<void()>> on_click;
//     std::optional<std::function<void()>> on_release;
// };
//
// struct DropDown {
//     bool menu_dropped;
//     bool clicked_last_frame;
// };
//
// using RectID = int;
//
// struct DropDownOverlay {
//     int rect_hook_index;
//     int on_click_index;
//     int selected_index;
//     std::vector<const char*>* items;
// };
//
// struct SliderOnInputInfo {
//     Slider& sldier;
//     int on_input_index;
// };
//
// enum DrawCommand {
//     rect,
//     text,
// };
//
// class UI {
//   public:
//     UI(Font* default_font) : m_default_font(default_font) {}
//     void text_field(TextFieldState* state, Style style);
//     RectID button(const char* text, Style style, OnClick&& on_click);
//     RectID button_anim(const char* text, AnimState* anim_state, const Style& style, const AnimStyle& anim_style, OnClick&& on_click);
//     void slider(Slider& state, SliderStyle style, float fraction, SliderCallbacks&& callbacks);
//     void drop_down_menu(
//         int selected_opt_index,
//         std::vector<const char*>&& options,
//         DropDown& state,
//         std::function<void(int)> on_input
//     );
//     void drop_down_menu(
//         int selected_opt_index,
//         std::vector<const char*>& options,
//         DropDown& state,
//         std::function<void(int)> on_input
//     );
//
//     RectID text(const char* text, const Style& style);
//
//     RectID begin_row(const Style& style);
//     void end_row();
//     Rect query_rect(RectID id);
//
//     RectID begin_row_button(const Style& style, OnClick&& on_click);
//     RectID begin_row_button_anim(AnimState* anim_state, Style style, const AnimStyle& anim_style, OnClick&& on_click);
//
//     void input(Input::Input& input);
//
//     void begin_frame(int width, int height);
//     void end_frame();
//
//     void draw(Renderer& renderer);
//
//     bool clicked = false;
//
//     StringCache strings{};
//
//   private:
//     Font* m_default_font;
//     Input::Input* m_input;
//
//     int m_screen_width = 0;
//     int m_screen_height = 0;
//
//     std::vector<Rect> m_rects;
//
//     std::vector<DrawRect> m_draw_rects;
//
//     std::vector<ClickRect> m_click_rects;
//     std::vector<HoverRect> m_hover_rects;
//     std::vector<SliderHeldRect> m_slider_input_rects;
//
//     std::vector<DrawCommand> m_draw_order;
//
//     std::vector<Row> m_rows;
//     std::vector<Text> m_texts;
//
//     std::vector<TextFieldInputRect> m_text_field_inputs;
//
//     std::vector<OnClick> m_on_click_callbacks;
//
//
//     //slider callbacks
//     std::vector<std::function<void()>> m_slider_user_callbacks; // slider on_click and on_release
//     std::vector<std::function<void(float)>> m_slider_on_held_callbacks;
//     std::vector<std::function<void()>> m_slider_on_release_callbacks;
//
//     //dropdown callbacks
//     std::vector<std::function<void(int)>> m_dropdown_on_select_callbacks; 
//
//     std::vector<const char*> m_options;
//     std::optional<DropDownOverlay> m_post_overlay;
//     std::vector<DropDown*> m_dropdown_clickoff_callbacks;
//
//     std::vector<int> m_row_stack;
//     std::vector<Command> m_command_tree;
//
//
//     void text_headless(const char* text, const Style& style);
//     void add_parent_scale(Row& row, glm::vec2 scale);
//
//     bool m_input_called{};
//     bool m_begin_frame_called{};
//     bool m_end_frame_called{};
// };
//
