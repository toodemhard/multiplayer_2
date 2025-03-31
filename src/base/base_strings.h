// String8 contains nullterm, String is just a slice view
// Terrible names
// When refactor done rename everything consistently

// string buffer with null term
// length not including null term
typedef struct String8 String8;
struct String8 {
    u8* data;
    u64 length;
};

typedef Slice_u8 String;
String string_cat(Arena* arena, String a, String b);
String cstr_to_string(const char* str);
Slice_u8 slice_str_literal(const char* str);

String8 string8_literal(const char* str);
String8 string8_format(Arena* arena, const char* fmt, ...);
// Slice<u8> cstr_to_string(const char* str);
// char* string_to_cstr(Arena* arena, const Slice<u8> string);
// void string_cat(Slice<u8>* dst, const Slice<u8> src);

// add null term
char* slice_str_to_cstr(Arena* arena, Slice_u8 str);
char* string_to_cstr(Arena* arena, String str);
