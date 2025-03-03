#include "pch.h"

#include "ui.h"


global UI* ui_ctx;

void ui_set_ctx(UI* _ui) {
    ui_ctx = _ui;
}

internal void ui_frame_init(UI_Frame* frame, Arena* arena) {
    arena_reset(arena);
    frame->elements = slice_create<UI_Element>(arena, 1024);
    frame->hashed_elements = hashmap_create(arena, 1024);
}

void ui_init(UI* ui_ctx, Arena* arena, Slice<Font> fonts, Renderer* renderer) {
    for (i32 i = 0; i < 2; i++) {
        ui_ctx->frame_arenas[i] = arena_suballoc(arena, megabytes(0.5));
        ui_frame_init(&ui_ctx->frame_buffer[i], &ui_ctx->frame_arenas[i]);
    }
    ui_ctx->renderer = renderer;

    slice_init(&ui_ctx->parent_stack, arena, 1024);
    ui_ctx->fonts = fonts;
    ui_ctx->base_font_size = 16;
}

void ui_begin(Input::Input* input) {
    ui_ctx->active_frame = (ui_ctx->active_frame + 1) % 2;
    u32 i = ui_ctx-> active_frame;
    ui_frame_init(&ui_ctx->frame_buffer[i], &ui_ctx->frame_arenas[i]);
    slice_clear(&ui_ctx->parent_stack);

    ui_ctx->cursor_pos = *((float2*)&input->mouse_pos);

    Renderer* renderer = ui_ctx->renderer;
    ui_push_row({
        .size = {size_px(renderer->window_width), size_px(renderer->window_height)},
    });
}

UI_Key ui_push_leaf(UI_Element element) {
    UI_Key index = ui_push_row(element);
    ui_pop_row();
    return index;
}

UI_Key ui_push_row(UI_Element element) {
    UI_Frame* ui = &ui_ctx->frame_buffer[ui_ctx->active_frame];

    UI_Key index = ui->elements.length;
    slice_push(&ui->elements, element);
    UI_Element* element_ptr = &slice_back(&ui->elements);

    if (ui_ctx->parent_stack.length > 0) {

        UI_Element* parent = slice_back(&ui_ctx->parent_stack);
        if (parent->last_child) {
            UI_Element* last = parent->last_child;

            last->next_sibling = element_ptr;
            element_ptr->prev_sibling = last;

            parent->last_child = element_ptr;

        } else {
            parent->first_child = element_ptr;
            parent->last_child = element_ptr;
        }

        element_ptr->parent = parent;
    }

    slice_push(&ui_ctx->parent_stack, element_ptr);

    return index;
}

UI_Element* ui_get(UI_Key index) {
    return &ui_ctx->frame_buffer[ui_ctx->active_frame].elements[index];
}

bool ui_hover(UI_Key index) {
    u64 last_index = (ui_ctx->active_frame + 1) % 2;
    UI_Frame* last_frame = &ui_ctx->frame_buffer[last_index];

    UI_Element zero{};
    UI_Element* element = &zero;
    if (last_frame->elements.length > index) {
        element = &last_frame->elements[index];
    }

    float2 p0 = *(float2*)&element->computed_position;
    float2 p1 = {p0.x + element->computed_size[Axis2_X], p0.y + element->computed_size[Axis2_Y]};
    float2 cursor = ui_ctx->cursor_pos;
    bool is_hover = false;
    if (cursor.x >= p0.x && cursor.y >= p0.y && cursor.x <= p1.x && cursor.y <= p1.y) {
        is_hover = true;
    }

    return is_hover;
}

void ui_pop_row() {
    slice_pop(&ui_ctx->parent_stack);
}

void ui_end(Arena* temp_arena) {
    ui_pop_row();
    ArenaTemp checkpoint = arena_begin_temp_allocs(temp_arena);
    defer(arena_end_temp_allocs(checkpoint));

    UI_Frame* ui = &ui_ctx->frame_buffer[ui_ctx->active_frame];


    Slice<UI_Element*> pre_stack = slice_create<UI_Element*>(temp_arena, 512);
    Slice<UI_Element*> post_stack = slice_create<UI_Element*>(temp_arena, 512);

    if (ui->elements.length > 0) {
        slice_push(&pre_stack, &ui->elements[0]);
    }


    // postorder dfs with 2 stacks idfk
    while (pre_stack.length > 0) {
        UI_Element* current = slice_back(&pre_stack);
        slice_push(&post_stack, current);
        slice_pop(&pre_stack);

        UI_Element* child = current->last_child;
        while (child) {
            slice_push(&pre_stack, child);

            child = child->prev_sibling;
        }



        UI_Element* parent = current->parent;
        if (parent) {
            for (i32 i = 0; i < Axis2_Count; i++) {
                UI_Size* size = &current->size[i];
                if (size->type == UI_SizeType_Pixels) {
                    current->computed_size[i] = size->value;
                }

                if (size->type == UI_SizeType_ParentFraction) {
                    current->computed_size[i] = parent->computed_size[i] * size->value;
                }
            }
        }
    }

    while (post_stack.length > 0) {
        UI_Element* current = slice_back(&post_stack);
        slice_pop(&post_stack);

        float2 text_size = font::text_dimensions(ui_ctx->fonts[current->font], current->text, current->font_size);

        float2 image_size = {};

        if (current->image != ImageID_NULL) {
            const Texture* texture = &ui_ctx->renderer->textures[current->image];
            image_size = {(f32)texture->w, (f32)texture->h};
        }


        // text_size

        // post order stuff goes here
        for (i32 i = 0; i < Axis2_Count; i++) {
            UI_Size* semantic_size = &current->size[i];

            if (semantic_size->type == UI_SizeType_SumContent) {
                current->computed_size[i] += text_size[i];

                current->computed_size[i] += image_size[i];
            }
        }

        for (i32 i = 0; i < Axis2_Count; i++) {
            UI_Size* semantic_size = &current->size[i];
            if (semantic_size->type == UI_SizeType_ImageProportional) {
                i32 other_axis = (i + 1) % Axis2_Count;
                current->computed_size[i] += current->computed_size[other_axis] * (image_size[i] / image_size[other_axis]);

            }
        }

        for (i32 i = 0; i < RectSide_Count; i++) {

            if (current->padding[i].type == UI_SizeType_Pixels) {
                current->computed_padding[i] += current->padding[i].value;
                current->computed_size[i / 2] += current->computed_padding[i];
            }

            if (current->border[i].type == UI_SizeType_Pixels) {
                current->computed_border[i] += current->border[i].value;
                current->computed_size[i / 2] += current->computed_border[i];
            }

        }

        UI_Element* parent = current->parent;
        if (parent) {
            Axis2 grow_axis = parent->grow_axis;
            for (i32 i = 0; i < Axis2_Count; i++) {
                if (parent->size[i].type == UI_SizeType_SumContent) {

                    if (i == grow_axis) {
                        parent->computed_size[i] += current->computed_size[i];
                    } else {
                        parent->computed_size[i] = f32_max(parent->computed_size[i], current->computed_size[i]);
                    }
                }
            }
        }

    }

    Slice<float2> position_stack = slice_create<float2>(temp_arena, 512);
    slice_clear(&pre_stack);
    if (ui->elements.length > 0) {
        slice_push(&pre_stack, &ui->elements[0]);
    }


    // 2nd preorder for pos after compute size
    while (pre_stack.length > 0) {
        UI_Element* current = slice_back(&pre_stack);
        slice_push(&post_stack, current);
        slice_pop(&pre_stack);



        UI_Element* child = current->last_child;
        while (child) {
            slice_push(&pre_stack, child);

            child = child->prev_sibling;
        }


        // pre order stuff goes here

        UI_Element* parent = current->parent;
        for (i32 i = 0; i < Axis2_Count; i++) {
            if (current->position[i].type == UI_PositionType_AutoOffset) {
                if (current->parent) {
                    current->computed_position[i] = parent->next_position[i];
                    if (i == parent->grow_axis && !(current->flags & UI_Flags_Float)) {
                        parent->next_position[i] += current->computed_size[i];
                    }


                }
                current->computed_position[i] += current->position[i].value;
            }

            current->content_position[i] = current->computed_position[i];
            current->content_position[i] += current->computed_padding[i * 2] + current->computed_border[i * 2];
        }
        current->next_position = current->content_position;
    }
}

float2 f32arr_to_float2(f32 vec[2]) {
    return {vec[0], vec[1]};
}

float2 float2_add(float2 a, float2 b) {
    return {a.x + b.x, a.y + b.y};
}

void ui_draw(UI* ui_ctx, Renderer* renderer, Arena* temp_arena) {
    ArenaTemp checkpoint = arena_begin_temp_allocs(temp_arena);
    defer(arena_end_temp_allocs(checkpoint));

    UI_Frame* ui = &ui_ctx->frame_buffer[ui_ctx->active_frame];

    Slice<UI_Element*> pre_stack = slice_create<UI_Element*>(temp_arena, 512);
    Slice<UI_Element*> post_stack = slice_create<UI_Element*>(temp_arena, 512);

    if (ui->elements.length > 0) {
        slice_push(&pre_stack, &ui->elements[0]);
    }


    // postorder dfs with 2 stacks idfk
    while (pre_stack.length > 0) {
        UI_Element* current = slice_back(&pre_stack);
        slice_push(&post_stack, current);
        slice_pop(&pre_stack);


        UI_Element* child = current->last_child;
        while (child) {
            slice_push(&pre_stack, child);

            child = child->prev_sibling;
        }
        Rect rect = {current->computed_position, current->computed_size};
        renderer::draw_screen_rect(renderer, rect, current->background_color);

        for (i32 i = 0; i < RectSide_Count; i++) {
            i32 horizontal = i / 2;
            i32 vertical = 1 - i / 2;
            f32 border_width = current->computed_border[i];
            Rect border_rect = {
                .x = current->computed_position[Axis2_X],
                .y = current->computed_position[Axis2_Y],
                .w = vertical * border_width + horizontal * current->computed_size[Axis2_X],
                .h = horizontal * border_width + vertical * current->computed_size[Axis2_Y],
            };
            if (i == RectSide_Right) {
                border_rect.x += current->computed_size[Axis2_X] - border_width;
            }
            if (i == RectSide_Bottom) {
                border_rect.y += current->computed_size[Axis2_Y] - border_width;
            }
            renderer::draw_screen_rect(renderer, border_rect, current->border_color);
        }

        float2 content_size = current->computed_size;
        for (i32 i = 0; i < RectSide_Count; i++) {
            i32 axis = i / Axis2_Count;
            content_size[axis] -= current->computed_border[i] + current->computed_padding[i];
        }
        

        if (current->image != ImageID_NULL) {
            renderer::draw_sprite_screen(renderer, {current->content_position, content_size}, 
            SpriteProperties{
                .texture_id = image_id_to_texture_id(current->image)
            });
        }

        font::draw_text(renderer, ui_ctx->fonts[current->font], current->text, current->font_size, current->content_position);
    }

    while (post_stack.length > 0) {
        UI_Element* current = slice_back(&post_stack);
        slice_pop(&post_stack);
    }
}
