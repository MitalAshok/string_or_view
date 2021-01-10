#include <unordered_map>
#include <string>
#include <iostream>

#include "string_or_view.h"

int main(int argc, char** argv) {
    std::unordered_map<string_or_view, int> m;

    m["abc"] = 0;  // Will not dynamically allocate with a std::string
    m[std::to_string(argc)] = 1;  // Will dynamically allocate, but will ensure the lifetime is managed properly

    std::cout << m["abc"] << '\n';  // Prints `0` without constructing a std::string
    std::cout << m["1"] << '\n';  // Prints (argc == 1) also without constructing a std::string

    string_or_view s;  // s is a view on a string of length 0
    s = "abc";  // s is now a view to a string literal of length 3
    s.make_owning() += "def";  // s now owns a std::string("abcdef")
    s = s.steal() + "ghi";  // s now owns a std::string("abcdefghi")

    std::string_view sv = s;  // Implicitly convertible
    // Or explicitly convertible with `s.get()` or `*s`

    std::cout << s << '\n';  // Outputs exactly like `std::cout << sv << '\n'`
}
