global UI* ui_ctx;

void ui_set_ctx(UI* _ui) {
    ui_ctx = _ui;
}

f32 ru(f32 value) {
    return ui_ctx->base_size * value;
}

internal void ui_frame_init(UI_Frame* frame, Arena* arena) {
    arena_reset(arena);
    frame->elements = slice_create(UI_Element, arena, 1024);
    // frame->hashed_elements = hashmap_create(arena, 1024);
}

void ui_init(UI* ui, Arena* arena, Slice_Font fonts, Renderer* renderer) {
    for (i32 i = 0; i < 2; i++) {
        ui->frame_arenas[i] = arena_suballoc(arena, megabytes(0.5));
        ui_frame_init(&ui->frame_buffer[i], &ui->frame_arenas[i]);
    }
    ui->renderer = renderer;

    slice_init(&ui->parent_stack, arena, 1024);
    ui->fonts = fonts;
    ui->base_size = 16;

    ui->cache = hashmap_alloc(UI_Key, u32, arena, 1024);
}

void ui_begin() {
    ui_ctx->active_frame = (ui_ctx->active_frame + 1) % 2;
    u32 i = ui_ctx-> active_frame;
    ui_frame_init(&ui_ctx->frame_buffer[i], &ui_ctx->frame_arenas[i]);
    slice_clear(&ui_ctx->parent_stack);

    Renderer* renderer = ui_ctx->renderer;
    ui_push_row({
        .font_size = font_px(ui_ctx->base_size),
        .size = {size_px(renderer->window_width), size_px(renderer->window_height)},
        // .background_color = {0.5,0.5,0.5,1},
    });
}

// UI_Key ui_push_leaf(UI_Element element) {
//     UI_Key index = ui_push_row(element);
//     ui_pop_row();
//     return index;
// }

bool ui_is_active(UI_Key element) {
    return ui_ctx->active_element == element;
}

UI_Key ui_key(Slice_u8 key) {
    return fnv1a(key);
}

UI_Element* ui_prev_element() {
    UI_Frame* ui = &ui_ctx->frame_buffer[ui_ctx->active_frame];
    return slice_back(ui->elements);
}

UI_Key ui_push_row_internal(UI_Element element, const char* file, i32 line) {
    UI_Frame* ui = &ui_ctx->frame_buffer[ui_ctx->active_frame];

    UI_Key index = ui->elements.length;

    bool use_src_key = false;
    if (element.exc_key.length == 0) {
        use_src_key = true;
    }

    u32 key_size = element.exc_key.length + element.salt_key.length;
    if (use_src_key) {
        key_size += sizeof(line);
    }

    ArenaTemp scratch = scratch_get(0,0);
    defer_loop(0, scratch_release(scratch)) {
    
    Slice_u8 key = slice_create(u8, scratch.arena, key_size);

    slice_push_range(&key, slice_data_raw(element.exc_key));
    if (use_src_key) {
        slice_push_buffer(&key, (u8*)&line, sizeof(line));
    }
    slice_push_range(&key, slice_data_raw(element.salt_key));

    element.computed_key = fnv1a(key);

    // if (element.image_src.w == 0 && element.image_src.h == 0) {
    //     element.image_src 
    //
    // }

    // if (element.key == ui_ctx->active_element) {
    //     UI_Size border[RectSide_Count] = sides_px(4);
    //     memcpy(&element.border, &border, sizeof(element.border));
    //     element.border_color = {1,1,0,1};
    // }
    //
    }

    slice_push(&ui->elements, element);
    UI_Element* current = slice_back(ui->elements);

    for (i32 i = 0; i < Axis2_Count; i++) {
        UI_Size* size = &current->size[i];
        if (size->type == UI_SizeType_Pixels) {
            current->computed_size.v[i] = size->value;
        }
    }

    if (ui_ctx->parent_stack.length > 0) {

        UI_Element* parent = *slice_back(ui_ctx->parent_stack);
        if (parent->last_child) {
            UI_Element* last = parent->last_child;

            last->next_sibling = current;
            current->prev_sibling = last;

            parent->last_child = current;

        } else {
            parent->first_child = current;
            parent->last_child = current;
        }

        current->parent = parent;
    }

    slice_push(&ui_ctx->parent_stack, current);

    if (element.out_key != NULL) {
        *element.out_key = element.computed_key;
    }

    return element.computed_key;
}

UI_Element* ui_get(UI_Key index) {
    return slice_getp(ui_ctx->frame_buffer[ui_ctx->active_frame].elements, index);
}

bool ui_button(UI_Key element) {
    return input_mouse_down(SDL_BUTTON_LEFT) && ui_hover(element);
}

bool ui_hover(UI_Key key) {
    u64 last_index = (ui_ctx->active_frame + 1) % 2;
    UI_Frame* last_frame = &ui_ctx->frame_buffer[last_index];

    UI_Element zero = {0};
    UI_Element* element = &zero;
    if (hashmap_key_exists(ui_ctx->cache, key)) {
        element = slice_getp(last_frame->elements, hashmap_get(ui_ctx->cache, key));
    }

    float2 p0 = *(float2*)&element->computed_position;
    float2 p1 = {p0.x + element->computed_size.v[Axis2_X], p0.y + element->computed_size.v[Axis2_Y]};
    float2 cursor = input_mouse_position();
    bool is_hover = false;
    if (cursor.x >= p0.x && cursor.y >= p0.y && cursor.x <= p1.x && cursor.y <= p1.y) {
        is_hover = true;
    }

    return is_hover;
}

UI_Key ui_pop_row() {
    UI_Key element = (*slice_back(ui_ctx->parent_stack))->computed_key;
    slice_pop(&ui_ctx->parent_stack);
    return element;
}


// no idea how last ui worked
// there was only 1 post order and 1 pre order?????
// post order is free in build call

// possible on init: fixed size, absolute pos, size fraction of parent if parent fixed
// postorder: fit content
// preorder: grow
// preorder: positions


void ui_end(Arena* temp_arena) {
    ui_pop_row();
    ArenaTemp checkpoint = arena_begin_temp_allocs(temp_arena);
    defer_loop(0, arena_end_temp_allocs(checkpoint)) {

    UI_Frame* ui = &ui_ctx->frame_buffer[ui_ctx->active_frame];


    Slice_pUI_Element pre_stack = slice_p_create(UI_Element, temp_arena, 512);
    Slice_pUI_Element post_stack = slice_p_create(UI_Element, temp_arena, 512);

    if (ui->elements.length > 0) {
        slice_push(&pre_stack, slice_getp(ui->elements, 0));
    }

    // postorder dfs with 2 stacks idfk
    while (pre_stack.length > 0) {
        UI_Element* current = *slice_back(pre_stack);
        slice_push(&post_stack, current);
        slice_pop(&pre_stack);

        UI_Element* child = current->last_child;
        while (child) {
            slice_push(&pre_stack, child);

            child = child->prev_sibling;
        }

        //preorder
        UI_Element* parent = current->parent;
        if (parent) {
            for (i32 i = 0; i < Axis2_Count; i++) {
                UI_Size* size = &current->size[i];

                if (size->type == UI_SizeType_ParentFraction) {
                    current->computed_size.v[i] = parent->computed_size.v[i] * size->value;
                }
            }
            if (current->font_size.type == FontSizeType_Default) {
                current->font_size.value = parent->font_size.value;
            }
        }
    }

    while (post_stack.length > 0) {
        UI_Element* current = *slice_back(post_stack);
        slice_pop(&post_stack);

        // postorder

        float2 text_size = text_dimensions(slice_get(ui_ctx->fonts,current->font), current->text, current->font_size.value);

        float2 image_size = {0};

        if (current->image != ImageID_NULL) {
            const Texture* texture = &ui_ctx->renderer->textures[current->image];
            image_size = (float2){(f32)texture->w, (f32)texture->h};
        }



        for (i32 i = 0; i < Axis2_Count; i++) {
            UI_Size* semantic_size = &current->size[i];

            if (semantic_size->type == UI_SizeType_Fit) {
                current->computed_size.v[i] += text_size.v[i];

                current->computed_size.v[i] += image_size.v[i];
            }
        }

        for (i32 i = 0; i < Axis2_Count; i++) {
            UI_Size* semantic_size = &current->size[i];
            if (semantic_size->type == UI_SizeType_ImageProportional) {
                i32 other_axis = (i + 1) % Axis2_Count;
                current->computed_size.v[i] += current->computed_size.v[other_axis] * (image_size.v[i] / image_size.v[other_axis]);

            }
        }

        for (i32 i = 0; i < RectSide_Count; i++) {

            if (current->padding[i].type == UI_SizeType_Pixels) {
                current->computed_padding[i] += current->padding[i].value;
                current->computed_size.v[i / 2] += current->computed_padding[i];
            }

            if (current->border[i].type == UI_SizeType_Pixels) {
                current->computed_border[i] += current->border[i].value;
                current->computed_size.v[i / 2] += current->computed_border[i];
            }

        }

        UI_Element* parent = current->parent;
        if (parent) {
            Axis2 grow_axis = parent->stack_axis;
            for (i32 i = 0; i < Axis2_Count; i++) {
                if (parent->size[i].type == UI_SizeType_Fit) {
                    if (i == grow_axis) {
                        parent->computed_size.v[i] += current->computed_size.v[i];
                    } else {
                        parent->computed_size.v[i] = f32_max(parent->computed_size.v[i], current->computed_size.v[i]);
                    }
                }
            }
        }
    }

    slice_clear(&pre_stack);
    if (ui->elements.length > 0) {
        slice_push(&pre_stack, slice_getp(ui->elements,0));
    }

    // preorder for grow
    // could be static init actually cuz should only grow if parent fixed size
    while (pre_stack.length > 0) {
        UI_Element* current = *slice_back(pre_stack);
        slice_pop(&pre_stack);

        // grow child elements which have grow sizing property
        float2 grow_space = current->computed_size;
        for (i32 side = 0; side < RectSide_Count; side++) {
            grow_space.v[side / 2] -= current->computed_padding[side] + current->computed_border[side];
        }

        i32 grow_children_count_in_stack_dir = 0;

        UI_Element* child = current->last_child;
        while (child) {
            slice_push(&pre_stack, child);

            for (i32 axis = 0; axis < Axis2_Count; axis++) {
                if (axis == current->stack_axis) {
                    grow_space.v[axis] -= child->computed_size.v[axis];
                }
                if (child->size[axis].type == UI_SizeType_Grow && axis == current->stack_axis) {
                    grow_children_count_in_stack_dir++;
                }

                if(child->size[axis].type == UI_SizeType_Grow && axis != current->stack_axis) {
                    child->computed_size.v[axis] += grow_space.v[axis];
                }
            }

            child = child->prev_sibling;
        }

        child = current->last_child;
        f32 distribution_of_capital = grow_space.v[current->stack_axis] / grow_children_count_in_stack_dir;
        while (child) {
            for (i32 axis = 0; axis < Axis2_Count; axis++) {
                if (child->size[axis].type == UI_SizeType_Grow && axis == current->stack_axis) {
                    child->computed_size.v[axis] += distribution_of_capital;
                }
            }

            child = child->prev_sibling;
        }

        
    }

    slice_clear(&pre_stack);
    if (ui->elements.length > 0) {
        slice_push(&pre_stack, slice_getp(ui->elements,0));
    }


    // preorder for pos
    while (pre_stack.length > 0) {
        UI_Element* current = *slice_back(pre_stack);
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
                    current->computed_position.v[i] = parent->next_position.v[i];
                    if (i == parent->stack_axis && !(current->flags & UI_Flags_Float)) {
                        parent->next_position.v[i] += current->computed_size.v[i];
                    }


                }
                current->computed_position.v[i] += current->position[i].value;
            }

            UI_Position position = current->position[i];

            if (position.type == UI_PositionType_Anchor) {
                current->computed_position.v[i] = parent->computed_position.v[i] + (parent->computed_size.v[i] - current->computed_size.v[i]) * position.value;
            }

            current->content_position.v[i] = current->computed_position.v[i];
            current->content_position.v[i] += current->computed_padding[i * 2] + current->computed_border[i * 2];
        }
        current->next_position = current->content_position;
    }

    for (i32 i = 0; i < ui->elements.length; i++) {
        // hashmap_set_raw(
        //         (&ui_ctx->cache)->pairs,
        //         (&ui_ctx->cache)->capacity,
        //         (&ui_ctx->cache)->count,
        //         (typeof(element->computed_key)[1]{element->computed_key}),
        //         sizeof(element->computed_key),
        //         (typeof((u32)i)[1]{(u32)i}),
        //         sizeof((u32)i)
        //         );
        UI_Element* element = slice_getp(ui->elements,i);

// hashmap_set_raw(
//     (&ui_ctx->cache)->pairs,
//     (&ui_ctx->cache)->capacity,
//     &(&ui_ctx->cache)->count,
//     ((typeof(element->computed_key)[1]){element->computed_key}),
//     sizeof(element->computed_key),
//     __builtin_offsetof(typeof(*(&ui_ctx->cache)->pairs), key)((typeof((u32)i)[1]){(u32)i}),
//     sizeof((u32)i),
//     __builtin_offsetof(typeof(*(&ui_ctx->cache)->pairs), value)
// )
        hashmap_set(&ui_ctx->cache, element->computed_key, (u32)i);

        if (input_mouse_down(SDL_BUTTON_LEFT)) {
            float2 p0 = element->computed_position;
            float2 p1 = (float2){p0.x + element->computed_size.v[Axis2_X], p0.y + element->computed_size.v[Axis2_Y]};
            float2 cursor = input_mouse_position();
            // bool is_hover = false;
            if (cursor.x >= p0.x && cursor.y >= p0.y && cursor.x <= p1.x && cursor.y <= p1.y) {
                ui_ctx->active_element = element->computed_key;
            }
        }
    }

    }
}

float2 f32arr_to_float2(f32 vec[2]) {
    return (float2){vec[0], vec[1]};
}

void ui_draw(UI* ui_ctx) {
    // ArenaTemp checkpoint = arena_begin_temp_allocs(temp_arena);
    // defer(arena_end_temp_allocs(checkpoint));

    UI_Frame* ui = &ui_ctx->frame_buffer[ui_ctx->active_frame];

    ArenaTemp scratch = scratch_get(0, 0);

    Slice_pUI_Element pre_stack = slice_p_create(UI_Element, scratch.arena, 512);
    Slice_pUI_Element post_stack = slice_p_create(UI_Element, scratch.arena, 512);

    if (ui->elements.length > 0) {
        slice_push(&pre_stack, slice_getp(ui->elements, 0));
    }


    // postorder dfs with 2 stacks idfk
    while (pre_stack.length > 0) {
        UI_Element* current = *slice_back(pre_stack);
        slice_push(&post_stack, current);
        slice_pop(&pre_stack);


        UI_Element* child = current->last_child;
        while (child) {
            slice_push(&pre_stack, child);

            child = child->prev_sibling;
        }
        Rect rect = {current->computed_position, current->computed_size};
        draw_screen_rect(rect, current->background_color);

        for (i32 i = 0; i < RectSide_Count; i++) {
            i32 horizontal = i / 2;
            i32 vertical = 1 - i / 2;
            f32 border_width = current->computed_border[i];
            Rect border_rect = {
                .x = current->computed_position.v[Axis2_X],
                .y = current->computed_position.v[Axis2_Y],
                .w = vertical * border_width + horizontal * current->computed_size.v[Axis2_X],
                .h = horizontal * border_width + vertical * current->computed_size.v[Axis2_Y],
            };
            if (i == RectSide_Right) {
                border_rect.x += current->computed_size.v[Axis2_X] - border_width;
            }
            if (i == RectSide_Bottom) {
                border_rect.y += current->computed_size.v[Axis2_Y] - border_width;
            }
            draw_screen_rect(border_rect, current->border_color);
        }

        float2 content_size = current->computed_size;
        for (i32 i = 0; i < RectSide_Count; i++) {
            i32 axis = i / Axis2_Count;
            content_size.v[axis] -= current->computed_border[i] + current->computed_padding[i];
        }

        if (current->image != ImageID_NULL) {
            draw_sprite_screen((Rect){current->content_position, content_size}, 
            (SpriteProperties){
                .texture_id = image_id_to_texture_id(current->image),
                .src_rect = current->image_src,
            });
        }

        // RGBA font_color;
        // for (u32 i = 0; i < 4; i++) {
        //     font_color.v[i] = 255.0 * current->font_color.v[i];
        // }
        //
        if (current->font_color.a == 0) {
            current->font_color = (float4) {1,1,1,1};
        }

        draw_text(slice_get(ui_ctx->fonts, current->font), current->text, current->font_size.value, current->content_position, current->font_color, 9999);
    }

    while (post_stack.length > 0) {
        UI_Element* current = *slice_back(post_stack);
        slice_pop(&post_stack);
    }

    scratch_release(scratch);
}
