string_or_view
==============

A C++17 header-only library that implements a fast feature-rich wrapper on a possibly-owning string
(similar to a `std::variant<std::string, std::string_view>`, but with less overhead and easier
interoperability with existing `string_view` or `string` code).

After including the single header in `include/string_or_view.h`, basic usage is:

```c++
string_or_view sov;

sov = std::string{something};  // Set to a string
sov = std::string_view{something};  // Or a string_view
sov = "something";  // Will use `const char*` as a string_view

sov == std::string{"something"};  // Compare to objects convertible to string_view

std::hash<string_or_view>{}(sov);  // Hash

// Intended to be used in `unordered_map` or `unordered_set` objects

std::unordered_map<string_or_view, int> m;
// Will construct `std::string`s, dynamically allocating memory
m[function_returning_a_string(0)] = 0;
m[function_returning_a_string(1)] = 1;

// Does not construct a string
m.includes("abc");

// Also wstring_or_view or u8string_or_view if you currently use wstring_view or u8string_view
```

Documentation
-------------

A `basic_string_or_view` is "owning" if it currently holds a `basic_string` that has allocated memory, otherwise it is
"viewing" and holds a `basic_string_view`.

```c++
template<typename CharT, typename Traits = std::char_traits<CharT>, typename Allocator = std::allocator<CharT>>
struct basic_string_or_view {

    // Helper type aliases

    using char_type = CharT;
    using traits_type = Traits;
    using allocator_type = Allocator;  // Only used if owning
    using string_type = std::basic_string<char_type, traits_type, allocator_type>;  // owning type
    using string_view_type = std::basic_string_view<char_type, traits_type>;  // viewing type


    // Constructors
    constexpr basic_string_or_view() noexcept;
    constexpr basic_string_or_view(const basic_string_or_view&);
    constexpr basic_string_or_view(basic_string_or_view&&) noexcept;
    constexpr basic_string_or_view(string_type&&) noexcept;
    constexpr basic_string_or_view(string_view_type) noexcept;
    constexpr basic_string_or_view(const char_type*) noexcept(/* if possible */);
    constexpr basic_string_or_view(std::nullptr_t) noexcept;
    constexpr basic_string_or_view(const string_type&);
    constexpr basic_string_or_view(const string_type&, const allocator_type&);


    // Assignment operators
    constexpr basic_string_or_view& operator=(const basic_string_or_view&);
    constexpr basic_string_or_view& operator=(basic_string_or_view&&) noexcept;
    constexpr basic_string_or_view& operator=(/* other types */) noexcept(/* if possible */);


    // State observers
    [[nodiscard]] constexpr bool is_owning() const noexcept;
    [[nodiscard]] constexpr bool is_owning() const noexcept;


    // Named assignment functions. Less possible conversions than `operator=`
    constexpr string_type& own(string_type&&) noexcept;
    constexpr string_type& own(const string_type&);
    constexpr string_view_type& view(string_view_type) noexcept;


    // Possibly copy
    constexpr string_type& make_owning(const allocator_type& = allocator_type());
    constexpr string_type& make_owning_keep_existing_alloc(const allocator_type& = allocator_type());


    // Steal
    [[nodiscard]] constexpr string_type steal(const allocator_type& = allocator_type());


    // Conversions to viewing type
    [[nodiscard]] constexpr string_view_type operator*() const noexcept;
    [[nodiscard]] constexpr /* internal type */ operator->() const noexcept;
    [[nodiscard]] constexpr operator string_view_type() const noexcept;
    [[nodiscard]] constexpr string_view_type get() const noexcept;


    // Swap
    void swap(basic_string_or_view& other) noexcept(/* if possible */);
    void swap(string_type& other);


    // Consistency with string_view interface
    using value_type = char_type;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using reference = value_type&;
    using const_reference = const value_type&;
    using const_iterator = const_pointer;
    using iterator = const_iterator;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reverse_iterator = const_reverse_iterator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);
    // All of these member functions are equivalent to `(this->get()).function(arguments...)` or `(*this)->function(arguments...)`
    // See documentation on `std::basic_string_view`
    [[nodiscard]] constexpr const_iterator begin() const noexcept;
    [[nodiscard]] constexpr const_iterator cbegin() const noexcept;
    [[nodiscard]] constexpr const_iterator end() const noexcept;
    // Other iterator including reverse iterator member functions
    [[nodiscard]] constexpr const_reference operator[](size_type) const noexcept;
    constexpr const_reference at(size_type) const noexcept;
    [[nodiscard]] constexpr const_reference front() const noexcept;
    [[nodiscard]] constexpr const_reference back() const noexcept;
    [[nodiscard]] constexpr const_pointer data() const noexcept;
    [[nodiscard]] constexpr size_type size() const noexcept;
    [[nodiscard]] constexpr size_type length() const noexcept;
    [[nodiscard]] constexpr size_type max_size() const noexcept;
    [[nodiscard]] constexpr bool empty() const noexcept;
    constexpr size_type copy(char_type*, size_type, size_type = 0) const;


    // Allocator observer
    [[nodiscard]] constexpr std::optional<allocator_type> get_allocator() const noexcept;
    [[nodiscard]] constexpr allocator_type get_allocator_or(const allocator_type& = allocator_type()) const noexcept;

    /* possibly constexpr */ ~basic_string_or_view() noexcept;


    // Non-member functions

    // Comparison
    [[nodiscard]] friend constexpr bool operator==(const basic_string_or_view&, const basic_string_or_view&) noexcept;
    [[nodiscard]] friend constexpr bool operator==(const basic_string_or_view&, string_view_type) noexcept;
    [[nodiscard]] friend constexpr /* see below */ operator<=>(const basic_string_or_view&, const basic_string_or_view&) noexcept;  // (Since C++20)
    [[nodiscard]] friend constexpr /* see below */ operator<=>(const basic_string_or_view&, string_view_type) noexcept;  // (Since C++20)
    [[nodiscard]] friend constexpr bool operator==(string_view_type, const basic_string_or_view&) noexcept;  // (Until C++20)
    // And all the other comparison operators between
    // `const basic_string_or_view& OPERATOR const basic_string_or_view&`,
    // `const basic_string_or_view& OPERATOR string_view_type` and
    // `string_view_type OPERATOR const basic_string_or_view&`
    // (All removed in C++20 since they are implemented with three way comparison


    // Swap
    friend void swap(basic_string_or_view&, basic_string_or_view&) noexcept(/* if possible */);
    friend void swap(basic_string_or_view&, string_type&);
    friend void swap(string_type&, basic_string_or_view&);


    // Stream manipulation
    friend std::basic_ostream<char_type, traits_type>& operator<<(std::basic_ostream<char_type, traits_type>&, const basic_string_or_view&);
    friend std::basic_istream<char_type, traits_type>& operator>>(std::basic_istream<char_type, traits_type>&, basic_string_or_view&);
};


// Hashing
namespace std {
    template<typename CharT, typename Traits, typename Allocator>
    struct hash<basic_string_or_view<CharT, Traits, Allocator>> {
        constexpr size_t operator()(const basic_string_or_view<CharT, Traits, Allocator>&) const noexcept(/* if possible */);
    };
}


// Traits
template<typename StringOrView, typename Allocator = void>
struct to_string_or_view;


// Standard aliases
using string_or_view = basic_string_or_view<char>;
using wstring_or_view = basic_string_or_view<wchar_t>;
using u16string_or_view = basic_string_or_view<char16_t>;
using u32string_or_view = basic_string_or_view<char32_t>;
using u8string_or_view = basic_string_or_view<char8_t>;  // (Since C++20)

namespace pmr {
    using string_or_view = basic_string_or_view<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>>;
    using wstring_or_view = basic_string_or_view<wchar_t, std::char_traits<wchar_t>, std::pmr::polymorphic_allocator<wchar_t>>;
    using u16string_or_view = basic_string_or_view<char16_t, std::char_traits<char16_t>, std::pmr::polymorphic_allocator<char16_t>>;
    using u32string_or_view = basic_string_or_view<char32_t, std::char_traits<char32_t>, std::pmr::polymorphic_allocator<char32_t>>;
    using u8string_or_view = basic_string_or_view<char8_t, std::char_traits<char8_t>, std::pmr::polymorphic_allocator<char8_t>>;  // (Since C++20)
}
```

### Constructors

```c++
constexpr basic_string_or_view() noexcept;  // (1)
constexpr basic_string_or_view(const basic_string_or_view& other);  // (2)
constexpr basic_string_or_view(basic_string_or_view&& other) noexcept;  // (3)
constexpr basic_string_or_view(string_type&& other) noexcept;  // (4)
constexpr basic_string_or_view(string_view_type other) noexcept;  // (5)
constexpr basic_string_or_view(const char_type* other) noexcept(/* if possible */);  // (6)
constexpr basic_string_or_view(std::nullptr_t) noexcept;  // (7)
constexpr basic_string_or_view(const string_type& other);  // (8)
constexpr basic_string_or_view(const string_type& other, const allocator_type& alloc);  // (9)
```

Constructs either an owning or viewing basic_string_or_view. While a string_or_view is in it's lifetime, it will always
either hold a string_view or string, even in case of exceptions.

1. Default constructor. Constructs viewing `basic_string_or_view(nullptr, 0)`  (An empty view)
2. Copy constructor. Afterwards, `this->is_owning() == other.is_owning()`, `*this == other`.
3. Move constructor. Afterwards, `this->is_owning() == other.is_owning()` (which is unchanged), and if
   viewing, `*this == other` and `*other` is unchanged, otherwise `*this` will move construct the held string.
4. Construct by moving from a string. Afterwards, `this->is_owning()`.
5. Construct by coping from a view. Afterwards, `this->is_viewing()` and `*this == other`.
6. Construct by viewing a `const char*`. Afterwards, `this->is_viewing()` and `*this == other`.
7. Construct from `nullptr`. Same effect as default constructor.
8. Copy from a string. Afterwards, `this->is_owning() && *this == other`.
9. Copy from a string with a new allocator. Afterwards, `this->is_owning() && *this == other`.

Complexity:

1. Constant
2. Linear in size of other
3. Constant
4. Constant
5. Constant
6. Complexity of traits::length(other) and linear in the result of traits::length(other)
7. Constant
8. Linear in size of other

Exceptions

1. `noexcept`
2. No exceptions thrown if `other.is_owning()`. Otherwise any exceptions thrown by `string_type`'s copy constructor.
3. `noexcept`
4. `noexcept`
5. `noexcept`
6. `noexcept` if `traits::length(other)` is `noexcept`, or `traits` is a standard CharTraits (`std::char_traits<T>` for `T = { char, wchar_t, char16_t, char32_t, char8_t }`)
7. `noexcept`
8. Any exception thrown by `string_type`'s copy constructor.
9. Any exception thrown by `string_type`'s constructor that takes another string and an allocator.


### Assignment operators

```c++
// Copy / move
constexpr basic_string_or_view& operator=(const basic_string_or_view& other);
constexpr basic_string_or_view& operator=(basic_string_or_view&& other) noexcept;

// Owning
constexpr basic_string_or_view& operator=(string_type&&) noexcept;
constexpr basic_string_or_view& operator=(const string_type&);

// Viewing
constexpr basic_string_or_view& operator=(string_view_type) noexcept;
constexpr basic_string_or_view& operator=(const char_type*) noexcept(/* See constructor */);
constexpr basic_string_or_view& operator=(std::nullptr_t) noexcept;
```

Copy and move constructors copy or move the held string or view from `other` into `*this` and then return `*thid`.

The rest of the converting assignment operators taking `other` are equivalent to `*this = basic_string_or_view(other); return *this`,
and throw the same exceptions as the constructor.

### State observers

```c++
[[nodiscard]] constexpr bool is_owning() const noexcept;
[[nodiscard]] constexpr bool is_viewing() const noexcept;
```

`is_owning()`: Return `true` iff `*this` is holding a `string_type`. Otherwise, `*this` is holding a `string_view_type` and return `false`.  
`is_viewing()`: Return `!is_owning()`.

### Named assignment functions

```c++
constexpr string_type& own(string_type&& other) noexcept;  // (1)
constexpr string_type& own(const string_type& other);  // (1)

constexpr string_view_type& view(string_view_type other) noexcept;  // (2)
```

1. Calls `*this = other` and returns the newly held owning string. Afterwards, `is_owning()` is `true`.
2. Calls `*this = other` and returns the newly held view. Afterwards, `is_viewing()` is `true`.

### Possibly copy

```c++
constexpr string_type& make_owning(const allocator_type& = allocator_type());  // (1)
constexpr string_type& make_owning_keep_existing_alloc(const allocator_type& = allocator_type());  // (2)
```

After calling either of these functions, if no exception is thrown, `is_owning()` will be `true`.

If currently viewing, constructs a `string_type` using the provided allocator, and own that.

If currently owning, `make_owning_keep_existing_alloc` will do nothing. `make_owning` will replace the allocator
with the provided allocator (by move constructing from it). So `*(this->make_owning(alloc).get_allocator() == alloc` is always `true`

Returns the newly constructed or existing held owning string.

### Steal

```c++
[[nodiscard]] constexpr string_type steal(const allocator_type& = allocator_type());
```

If `is_owning()`, move from the held string. Otherwise, copy from the held view with the given allocator.

Will never throw exceptions if `is_owning()`.

### Conversions to viewing type

```c++
[[nodiscard]] constexpr string_view_type operator*() const noexcept;  // (1)
[[nodiscard]] constexpr operator string_view_type() const noexcept;  // (1)
[[nodiscard]] constexpr string_view_type get() const noexcept;  // (1)

[[nodiscard]] constexpr /* internal type */ operator->() const noexcept;  // (2)
```

1. If `is_viewing()`, return a copy of the held string view. Otherwise, convert the held string into
   a string view and return that.
2. Return a struct holding the result of `this->get()` with a member function `constexpr const string_view_type operator->() const noexcept`
   that returns the held string view. This means that `(*this)->f(args...)` is equivalent to `(*this).get().f(args...)`
   if `f` is a `const` qualified member function of `string_view_type`.


### Swap

```c++
void swap(basic_string_or_view& other) noexcept(/* if possible */);  // (1)
void swap(string_type& other);  // (2)

friend void swap(basic_string_or_view& l, basic_string_or_view& r) noexcept(noexcept(l.swap(r))) {
    l.swap(r);
}
friend void swap(basic_string_or_view& l, string_type& r) {
    l.swap(r);
}
friend void swap(string_type& l, basic_string_or_view& r) {
    r.swap(l);
}
```

1. Swap the contents of `*this` and `other`. Will swap `this->is_owning()` and `other.is_owning()`, as well
   as their held strings or views.
2. If `this->is_owning()`, swap the held string with `other`. Otherwise, move `other` into `*this` so that `*this`
   is now owning, and copy the held view into `other`.

(1) is noexcept if `swap(string_type&, string_type&)` is noexcept. It can only throw if both `*this` and `other` are
owning and swapping their held strings throws.

(2) will throw in the same situations if `this->is_owning()`. If not owning, it will throw whatever the `string_type`
constructor from `string_view_type` throws.


### Allocator observer

```c++
[[nodiscard]] constexpr std::optional<allocator_type> get_allocator() const noexcept;  // (1)
[[nodiscard]] constexpr allocator_type get_allocator_or(const allocator_type& alloc = allocator_type()) const noexcept;  // (2)
```

1. Return the allocator of the held string if `this->owning()`, otherwise an empty optional.
2. Equivalent to `get_allocator().value_or(alloc)`.


### Comparison

All comparison functions delegate to `this->get()` (using the `string_view_type` comparison functions).
As such, in C++20, `operator<=>` returns the result of `string_view_type`'s `operator<=>`, namely `traits_type::comparison_category`
if that exists, else `std::weak_ordering`.

See documentation for `operator<=>` and `operator==` for `basic_string_view`.


### Stream manipulation

```c++
friend std::basic_ostream<char_type, traits_type>& operator<<(std::basic_ostream<char_type, traits_type>& os, const basic_string_or_view& sov);  // (1)
friend std::basic_istream<char_type, traits_type>& operator>>(std::basic_istream<char_type, traits_type>& is, basic_string_or_view& sov);  // (1)
```

1. Equivalent to `os << sov.get()`.
2. If `sov` is not owning, assign to a default constructed string. Then return `is >> sov./* internally_held_string */`.
   Requires that the allocator type is default constructible to create an owning string if it is not already owning.


### Hashing

```c++
template<typename CharT, typename Traits, typename Allocator>
constexpr std::size_t std::hash<basic_string_or_view<CharT, Traits, Allocator>>::operator()(const basic_string_or_view<CharT, Traits, Allocator>& sov) const noexcept(/* if possible */);
```

Returns the `std::hash` of `sov.get()`. `noexcept` if `noexcept(std::declval<const std::hash<std::basic_string_view<CharT, Traits>>&>()(sov.get()))`.

### Traits

```c++
template<typename StringOrView, typename Allocator = void>
struct to_string_or_view;
```

Given a `string_type = std::basic_string<CharT, Traits, Alloc>` and `string_view_type = std::basic_string_view<CharT, Traits>`

`to_string_or_view<string_type>` has a member type alias `type` `basic_string_or_view<CharT, Traits, Alloc>`.  
`to_string_or_view<string_view_type>` has a member type alias `type` `basic_string_or_view<CharT, Traits>`.  
`to_string_or_view<string_view_type, Allocator>` has a member type alias `type` `basic_string_or_view<CharT, Traits, Allocator>`.  
`to_string_or_view<string_type, ReplacementAllocator>` has a member type alias `type` `basic_string_or_view<CharT, Traits, ReplacementAllocator>`.  

`to_string_or_view<T>::type` will be a `basic_string_or_view` capable of holding or viewing `T`.
