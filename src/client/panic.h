#pragma once

#include <filesystem>
#include <format>
#include <iostream>
#include <source_location>
#include <utility>

// template <class T, typename... Args>
// constexpr void panic(const T& string, Args&&... args) {
//     std::format_string<Args...> format_string(string);
//     std::cout << std::format(format_string, std::forward<decltype(args)>(args)...);
//     __debugbreak();
// }


// c++ is retarded

template <class... Args> struct panic_format {
    template <class T>
    consteval panic_format( // note: consteval is what allows for compile-time checking of the
        const T& s,         //       format string
        std::source_location loc = std::source_location::current()
    ) noexcept
        : fmt{s}, loc{loc} {}

    std::format_string<Args...> fmt;
    std::source_location loc;
};

template <class... Args>
[[noreturn]] void panic(
    panic_format<std::type_identity_t<Args>...> fmt, // std::type_identity_t is needed to prevent
    Args&&... args
) noexcept // type deduction of the format string's
{          // arguments.

    std::cout << std::format(
        "[{}:{}] PANIC: {}\n",
        std::filesystem::path(fmt.loc.file_name()).filename().string(),
        fmt.loc.line(),
        std::format(fmt.fmt, std::forward<Args>(args)...)
    );
    __debugbreak();
    exit(1);
}
