String8 string8_literal(const char* str) {
    return (String8){
        .data = (u8*)str,
        .length = strlen(str),
    };
}

String cstr_to_string(const char* str) {
    return slice_create_view(u8, (u8*)str, strlen(str));
}

Slice_u8 slice_str_literal(const char* str) {
    return slice_create_view(u8, (u8*)str, strlen(str));
}

String8 string8_format(Arena* arena, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    va_list args_2;
    va_copy(args_2, args);

    i32 length = vsnprintf(NULL, 0, fmt, args) + 1;
    va_end(args);
    String8 string = {
        .data = slice_create_fixed(u8, arena, length).data,
        .length = length - 1,
    };
    vsnprintf((char* const)string.data, length, fmt, args_2);
    va_end(args_2);

    return string;
}

char* slice_str_to_cstr(Arena* arena, Slice_u8 str) {
    Slice_u8 cstr = slice_create(u8, arena, str.length + 1);
    slice_push_buffer(&cstr, str.data, str.length);
    slice_push(&cstr, '\0');
    return (char*)cstr.data;
}

char* string_to_cstr(Arena* arena, Slice_u8 str) {
    Slice_u8 cstr = slice_create(u8, arena, str.length + 1);
    slice_push_buffer(&cstr, str.data, str.length);
    slice_push(&cstr, '\0');
    return (char*)cstr.data;
}
// Slice<u8> cstr_to_string(const char* str) {
//     return {
//         .data = (u8*)str,
//         .length = strlen(str),
//     };
// }
//
// char* string_to_cstr(Arena* arena, const Slice<u8> string) {
//     Slice<u8> cstr = slice_create<u8>(arena, string.length + 1);
//     slice_push_range(&cstr, string.data, string.length);
//     slice_push(&cstr, (u8)'\0');
//     return (char*)cstr.data;
// }
//

String string_cat(Arena* arena, String a, String b) {
    String res = slice_create(u8, arena, a.length + b.length);
    slice_push_range(&res, a);
    slice_push_range(&res, b);
    return res;
}


// String cstr_to_string(Arena* arena, String a, String b) {

//
// bool string_compare(String8 a, String8 b) {
//     bool is_same = false;
//
//     if (a.length == b.length && memcmp(a.data, b.data, a.length) == 0){
//         is_same = true;
//     }
//
//     return is_same;
// }
//
// Slice<u8> string_to_byte_slice(String8 str) {
//     return Slice<u8> {
//         str.data,
//         str.length,
//         str.length
//     };
// }
