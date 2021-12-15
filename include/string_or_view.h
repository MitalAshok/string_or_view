#ifndef STRING_OR_VIEW_UNREACHABLE_DEFAULT
// Reuse STRING_OR_VIEW_UNREACHABLE_DEFAULT macro as header guard

#include <cstddef>
#include <optional>
#include <utility>
#include <string_view>
#include <string>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>


#if defined(__has_builtin) && !defined(STRING_OR_VIEW_UNREACHABLE_DEFAULT)
#if __has_builtin(__builtin_unreachable)
#define STRING_OR_VIEW_UNREACHABLE_DEFAULT default: __builtin_unreachable()
#elif __has_builtin(__builtin_assume)
#define STRING_OR_VIEW_UNREACHABLE_DEFAULT default: __builtin_assume(0)
#elif __has_builtin(__assume)
#define STRING_OR_VIEW_UNREACHABLE_DEFAULT default: __assume(0)
#endif
#endif

#if defined(_MSC_VER) && !defined(STRING_OR_VIEW_UNREACHABLE_DEFAULT)
#define STRING_OR_VIEW_UNREACHABLE_DEFAULT default: __assume(0)
#endif

#if defined(__has_cpp_attribute) && !defined(STRING_OR_VIEW_UNREACHABLE_DEFAULT)
#if __has_cpp_attribute(unreachable)
#define STRING_OR_VIEW_UNREACHABLE_DEFAULT default: [[unreachable]]
#endif
#endif

#ifndef STRING_OR_VIEW_UNREACHABLE_DEFAULT
namespace STRING_OR_VIEW_UNREACHABLE_DEFAULT_impl {
    namespace detail {
        [[noreturn]] inline void unreachable() {}  // Return on purpose so it is UB to call this function
    }
}
#define STRING_OR_VIEW_UNREACHABLE_DEFAULT default: ::STRING_OR_VIEW_UNREACHABLE_DEFAULT_impl::detail::unreachable()
#endif

struct constexpr_new_tag {
    constexpr explicit constexpr_new_tag(int) noexcept {}
    constexpr_new_tag() = delete;
    constexpr constexpr_new_tag(const constexpr_new_tag&) = default;
};

constexpr void* operator new(std::size_t, void* ptr, constexpr_new_tag) noexcept {
    return ptr;
}

template<typename CharT, typename Traits = typename std::basic_string<CharT>::traits_type, typename Allocator = typename std::basic_string<CharT, Traits>::allocator_type>
struct basic_string_or_view {
    using char_type = CharT;
    using traits_type = Traits;
    using allocator_type = Allocator;
    using string_type = std::basic_string<char_type, traits_type, allocator_type>;
    using string_view_type = std::basic_string_view<char_type, traits_type>;

    template<typename ReplacementAllocator>
    using replace_allocator = basic_string_or_view<char_type, traits_type, ReplacementAllocator>;
    template<typename ReplacementTraits>
    using replace_traits = basic_string_or_view<char_type, ReplacementTraits, allocator_type>;

private:
    static constexpr bool can_noexcept_construct_view_from_char_pointer =
        // The `::length(const char_type*)` static member function on standard traits are either noexcept or UB, so count them as noexcept
        std::is_same<traits_type, std::char_traits<char>>::value ||
        std::is_same<traits_type, std::char_traits<wchar_t>>::value ||
        std::is_same<traits_type, std::char_traits<char16_t>>::value ||
        std::is_same<traits_type, std::char_traits<char32_t>>::value ||
#ifdef __cpp_lib_char8_t
        std::is_same<traits_type, std::char_traits<char8_t>>::value ||
#endif
        noexcept(traits_type::length(static_cast<const char_type*>(nullptr)));

    // traits_type::length(p) but traits_type::length(nullptr) == 0 instead of UB
    static constexpr std::size_t traits_length(const char_type* p) noexcept(can_noexcept_construct_view_from_char_pointer) {
        return p ? static_cast<std::size_t>(traits_type::length(p)) : static_cast<std::size_t>(0);
    }
public:

    constexpr basic_string_or_view() noexcept : viewing(), tag(VIEWING) {}
    constexpr basic_string_or_view(const basic_string_or_view& other) {
        monostate.~empty_type();
        if (this == &other) {
            // `basic_string_or_view sv = sv;`
            // other is *not* in lifetime, so this does not have to be supported
            // But it can be so do so, as equivalent to `basic_string_or_view sv;`
            construct_viewing();
            return;
        }
        switch (other.tag) {
        case 0:
            construct_viewing(other.viewing);
            break;
        case 1:
            // NOTE: If this throws, tag is neither OWNING nor VIEWING.
            // This and `basic_string_or_view(const string_type&)` are the only places where this can happen
            // but then this is propagated out of the constructor
            // and ~basic_string_or_view() isn't called (this is not an object in lifetime yet)
            // so no basic_string_or_view object will ever have an invalid tag
            construct_owning<true>(other.owning);
            break;
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
    }

    constexpr basic_string_or_view(basic_string_or_view&& other) noexcept {
        monostate.~empty_type();
        if (this == &other) {
            construct_viewing();
            return;
        }
        switch (other.tag) {
        case VIEWING:
            construct_viewing(other.viewing);
            break;
        case OWNING:
            construct_owning(static_cast<string_type&&>(other.owning));
            break;
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
    }

    constexpr basic_string_or_view(string_type&& other) noexcept : owning(static_cast<string_type&&>(other)), tag(OWNING) {}
    constexpr basic_string_or_view(string_view_type other) noexcept : viewing(other), tag(VIEWING) {}
    constexpr basic_string_or_view(const char_type* other) noexcept(can_noexcept_construct_view_from_char_pointer) : viewing(other, traits_length(other)), tag(VIEWING) {}
    constexpr basic_string_or_view(std::nullptr_t) noexcept : basic_string_or_view() {}

    // Same note as copy constructor: string_type constructor throwing is fine
    constexpr basic_string_or_view(const string_type& other) : owning(other), tag(OWNING) {}
    constexpr basic_string_or_view(const string_type& other, const allocator_type& alloc) : owning(other, alloc), tag(OWNING) {}
    constexpr basic_string_or_view(string_type&& other, const allocator_type& alloc) noexcept(string_type_nothrow_constructible<string_type, const allocator_type&>()) : owning(static_cast<string_type&&>(other), alloc), tag(OWNING) {}

    constexpr basic_string_or_view& operator=(const basic_string_or_view& other) {
        switch (tag) {
        case VIEWING:
            switch (other.tag) {
            case VIEWING:
                viewing = other.viewing;
                break;
            case OWNING:
                copy_string_when_holding_view(other.owning);
                break;
            STRING_OR_VIEW_UNREACHABLE_DEFAULT;
            }
            break;
        case OWNING:
            switch (other.tag) {
            case VIEWING:
                owning.~basic_string();
                construct_viewing(other.viewing);
                break;
            case OWNING:
                owning = other.owning;
                break;
            STRING_OR_VIEW_UNREACHABLE_DEFAULT;
            }
            break;
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
        return *this;
    }

    constexpr basic_string_or_view& operator=(basic_string_or_view&& other) noexcept {
        switch (tag) {
        case VIEWING:
            switch (other.tag) {
            case VIEWING:
                viewing = other.viewing;
                break;
            case OWNING:
                viewing.~basic_string_view();
                construct_owning(static_cast<string_type&&>(other.owning));
                break;
            STRING_OR_VIEW_UNREACHABLE_DEFAULT;
            }
            break;
        case OWNING:
            switch (other.tag) {
            case VIEWING:
                owning.~basic_string();
                construct_viewing(other.viewing);
                break;
            case OWNING:
                // All other self-assignments are well defined, other than basic_string's move assign
                if (this != ::std::addressof(other)) {
                    owning = static_cast<string_type&&>(other.owning);
                }
                break;
            STRING_OR_VIEW_UNREACHABLE_DEFAULT;
            }
            break;
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
        return *this;
    }

    constexpr basic_string_or_view& operator=(string_type&& other) noexcept {
        switch (tag) {
        case VIEWING:
            viewing.~basic_string_view();
            construct_owning(static_cast<string_type&&>(other));
            break;
        case OWNING:
            if (::std::addressof(owning) != ::std::addressof(other)) {
                owning = static_cast<string_type&&>(other);
            }
            break;
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
        return *this;
    }

    constexpr basic_string_or_view& operator=(const string_type& other) {
        switch (tag) {
        case VIEWING:
            copy_string_when_holding_view(other);
            break;
        case OWNING:
            owning = other;
            break;
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
        return *this;
    }

    // Split into two instead of one `operator=(string_view_type)`
    // so `operator=(string_type&&)` is never a better match for `string_or_view{} = {(const char_t*), (const char_t*)}`
    constexpr basic_string_or_view& operator=(const string_view_type& other) noexcept {
        switch (tag) {
        case VIEWING:
            viewing = other;
            break;
        case OWNING:
            owning.~basic_string();
            construct_viewing(other);
            break;
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
        return *this;
    }

    constexpr basic_string_or_view& operator=(const char_type* other) noexcept(can_noexcept_construct_view_from_char_pointer) {
        return *this = string_view_type(other, traits_length(other));
    }

    constexpr basic_string_or_view& operator=(char_type* other) noexcept(false) {
        return *this = string_type(other);
    }

    constexpr basic_string_or_view& operator=(std::nullptr_t) noexcept {
        switch (tag) {
        case VIEWING:
            viewing = string_view_type();
            break;
        case OWNING:
            owning.~basic_string();
            construct_viewing();
            break;
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
        return *this;
    }

    constexpr string_type& own(string_type&& s) noexcept {
        *this = static_cast<string_type&&>(s);
        return owning;
    }
    constexpr string_type& own(const string_type& s) {
        *this = s;
        return owning;
    }
    constexpr string_view_type& view(string_view_type s) noexcept {
        *this = s;
        return viewing;
    }

    // If previously holding a view, copy it into a string and own that string. No effect if already is_owning().
    // Either throws in string constructor or is_owning() is now true.
    // Use the provided allocator if not owning, else replace the current one with the provided allocator
    // if they compare unequal
    constexpr string_type& make_owning(const allocator_type& alloc = allocator_type()) {
        switch (tag) {
        case VIEWING:
            copy_string_when_holding_view(string_type(viewing, alloc));
            break;
        case OWNING:
            if constexpr (!std::allocator_traits<allocator_type>::is_always_equal::value) {
                if (!static_cast<bool>(owning.get_allocator() == alloc)) {
                    owning = string_type(static_cast<string_type&&>(owning), alloc);
                }
            }
            break;
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
        return owning;
    }

    // As above but does not try replace existing allocator
    constexpr string_type& make_owning_keep_existing_alloc(const allocator_type& alloc = allocator_type()) {
        switch (tag) {
        case VIEWING:
            copy_string_when_holding_view(string_type(viewing, alloc));
            break;
        case OWNING:
            break;
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
        return owning;
    }

    // Copies from view (using provided allocator) if viewing else moves from owning
    [[nodiscard]] constexpr string_type steal(const allocator_type& alloc = allocator_type()) {
        switch (tag) {
        case VIEWING:
            return string_type(viewing, alloc);
        case OWNING:
            return static_cast<string_type&&>(owning);
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
    }

    [[nodiscard]] constexpr bool is_owning() const noexcept {
        switch (tag) {
        case VIEWING:
            return false;
        case OWNING:
            return true;
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
    }

    [[nodiscard]] constexpr bool is_viewing() const noexcept {
        return !is_owning();
    }

    [[nodiscard]] constexpr string_view_type operator*() const noexcept {
        switch (tag) {
        case VIEWING:
            return viewing;
        case OWNING:
            return string_view_type(owning.data(), owning.size());
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
    }
private:
    struct temporary_dereference_type {
        const string_view_type sv;
        [[nodiscard]] constexpr const string_view_type* operator->() const noexcept {
            return ::std::addressof(sv);
        }
    };
public:
    [[nodiscard]] constexpr temporary_dereference_type operator->() const noexcept {
        return temporary_dereference_type{ **this };
    }

    [[nodiscard]] constexpr operator string_view_type() const noexcept {
        return **this;
    }

    [[nodiscard]] constexpr string_view_type get() const noexcept {
        return **this;
    }

    [[nodiscard]] friend constexpr bool operator==(const basic_string_or_view& l, const basic_string_or_view& r) noexcept { return *l == *r; }
    [[nodiscard]] friend constexpr bool operator==(const basic_string_or_view& l, string_view_type r) noexcept { return *l == r; }

#ifdef __cpp_impl_three_way_comparison
    [[nodiscard]] friend constexpr decltype(auto) operator<=>(const basic_string_or_view& l, const basic_string_or_view& r) noexcept { return *l <=> *r; }
    [[nodiscard]] friend constexpr decltype(auto) operator<=>(const basic_string_or_view& l, string_view_type r) noexcept { return *l <=> r; }
#else
    [[nodiscard]] friend constexpr bool operator==(string_view_type l, const basic_string_or_view& r) noexcept { return l == *r; }

    [[nodiscard]] friend constexpr bool operator!=(const basic_string_or_view& l, const basic_string_or_view& r) noexcept { return *l != *r; }
    [[nodiscard]] friend constexpr bool operator< (const basic_string_or_view& l, const basic_string_or_view& r) noexcept { return *l <  *r; }
    [[nodiscard]] friend constexpr bool operator> (const basic_string_or_view& l, const basic_string_or_view& r) noexcept { return *l >  *r; }
    [[nodiscard]] friend constexpr bool operator<=(const basic_string_or_view& l, const basic_string_or_view& r) noexcept { return *l <= *r; }
    [[nodiscard]] friend constexpr bool operator>=(const basic_string_or_view& l, const basic_string_or_view& r) noexcept { return *l >= *r; }

    [[nodiscard]] friend constexpr bool operator!=(string_view_type l, const basic_string_or_view& r) noexcept { return l != *r; }
    [[nodiscard]] friend constexpr bool operator< (string_view_type l, const basic_string_or_view& r) noexcept { return l <  *r; }
    [[nodiscard]] friend constexpr bool operator> (string_view_type l, const basic_string_or_view& r) noexcept { return l >  *r; }
    [[nodiscard]] friend constexpr bool operator<=(string_view_type l, const basic_string_or_view& r) noexcept { return l <= *r; }
    [[nodiscard]] friend constexpr bool operator>=(string_view_type l, const basic_string_or_view& r) noexcept { return l >= *r; }

    [[nodiscard]] friend constexpr bool operator!=(const basic_string_or_view& l, string_view_type r) noexcept { return *l != r; }
    [[nodiscard]] friend constexpr bool operator< (const basic_string_or_view& l, string_view_type r) noexcept { return *l <  r; }
    [[nodiscard]] friend constexpr bool operator> (const basic_string_or_view& l, string_view_type r) noexcept { return *l >  r; }
    [[nodiscard]] friend constexpr bool operator<=(const basic_string_or_view& l, string_view_type r) noexcept { return *l <= r; }
    [[nodiscard]] friend constexpr bool operator>=(const basic_string_or_view& l, string_view_type r) noexcept { return *l >= r; }
#endif

    void swap(basic_string_or_view& other) noexcept(noexcept(owning.swap(other.owning))) {
        switch (tag) {
        case VIEWING:
            switch (other.tag) {
            case VIEWING:
                viewing.swap(other.viewing);
                break;
            case OWNING:
                swap_my_viewing_with_other_owning(other);
                break;
            STRING_OR_VIEW_UNREACHABLE_DEFAULT;
            }
            break;
        case OWNING:
            switch (other.tag) {
            case VIEWING:
                other.swap_my_viewing_with_other_owning(*this);
                break;
            case OWNING:
                if (this != ::std::addressof(other)) {
                    owning.swap(other.owning);
                }
                break;
            STRING_OR_VIEW_UNREACHABLE_DEFAULT;
            }
            break;
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
    }

    void swap(string_type& other) {
        switch (tag) {
        case VIEWING: {
            string_view_type sv = viewing;
            viewing.~basic_string_view();
            construct_owning(static_cast<string_type&&>(other));
            other = sv;
            break;
        }
        case OWNING:
            owning.swap(other);
            break;
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
    }

    friend void swap(basic_string_or_view& l, basic_string_or_view& r) noexcept(noexcept(l.swap(r))) {
        l.swap(r);
    }

    friend void swap(basic_string_or_view& l, string_type& r) {
        l.swap(r);
    }

    friend void swap(string_type& l, basic_string_or_view& r) {
        r.swap(l);
    }

    // copying string_view interface
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

    [[nodiscard]] constexpr const_iterator begin() const noexcept { return data(); }
    [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
    [[nodiscard]] constexpr const_iterator end() const noexcept {
        switch (tag) {
        case VIEWING:
            return viewing.data() + viewing.size();
        case OWNING:
            return owning.data() + owning.size();
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
    }
    [[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }
    [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return rend(); }

    [[nodiscard]] constexpr const_reference operator[](size_type pos) const noexcept {
        switch (tag) {
        case VIEWING:
            return viewing[pos];
        case OWNING:
            return owning[pos];
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
    }
    constexpr const_reference at(size_type pos) const {
        switch (tag) {
        case VIEWING:
            return viewing.at(pos);
        case OWNING:
            return owning.at(pos);
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
    }
    [[nodiscard]] constexpr const_reference front() const noexcept {
        switch (tag) {
        case VIEWING:
            return viewing.front();
        case OWNING:
            return owning.front();
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
    }
    [[nodiscard]] constexpr const_reference back() const noexcept {
        switch (tag) {
        case VIEWING:
            return viewing.back();
        case OWNING:
            return owning.back();
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
    }
    [[nodiscard]] constexpr const_pointer data() const noexcept {
        switch (tag) {
        case VIEWING:
            return viewing.data();
        case OWNING:
            return owning.data();
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
    }
    [[nodiscard]] constexpr size_type size() const noexcept {
        switch (tag) {
        case VIEWING:
            return viewing.size();
        case OWNING:
            return owning.size();
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
    }
    [[nodiscard]] constexpr size_type length() const noexcept { return size(); }
    [[nodiscard]] constexpr size_type max_size() const noexcept {
        switch (tag) {
        case VIEWING:
            return viewing.max_size();
        case OWNING:
            return owning.max_size();
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
    }
    [[nodiscard]] constexpr bool empty() const noexcept {
        switch (tag) {
        case VIEWING:
            return viewing.empty();
        case OWNING:
            return owning.empty();
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
    }
    constexpr size_type copy(char_type* dest, size_type count, size_type pos = 0) const {
        return (**this).copy(dest, count, pos);
    }
    // [[nodiscard]] constexpr string_view_type substr(size_type ) const;  // TODO: Rest of string_view interface
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    friend std::basic_ostream<char_type, traits_type>& operator<<(std::basic_ostream<char_type, traits_type>& os, const basic_string_or_view& sov) {
        return os << *sov;
    }
    friend std::basic_istream<char_type, traits_type>& operator>>(std::basic_istream<char_type, traits_type>& is, basic_string_or_view& sov) {
        if (!sov.is_owning()) sov = string_type();
        return is >> sov.owning;
    }

    [[nodiscard]] constexpr std::optional<allocator_type> get_allocator() const noexcept {
        switch (tag) {
        case VIEWING:
            return std::nullopt;
        case OWNING:
            return std::optional<allocator_type>(std::in_place, owning.get_allocator());
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
    }

    // Returns default_alloc if viewing instead of owning
    [[nodiscard]] constexpr allocator_type get_allocator_or(const allocator_type& default_alloc = allocator_type()) const noexcept {
        if constexpr (std::allocator_traits<allocator_type>::is_always_equal::value) {
            return default_alloc;
        }

        switch (tag) {
        case VIEWING:
            return default_alloc;
        case OWNING:
            return owning.get_allocator();
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
    }

    // NOTE: these references can only be used if is_owning(). Use carefully!
    [[nodiscard]] constexpr string_type& access_underlying_owned() noexcept {
        return owning;
    }
    [[nodiscard]] constexpr const string_type& access_underlying_owned() const noexcept {
        return owning;
    }
    // NOTE: these references can only be used if !is_owning(). Use carefully!
    [[nodiscard]] constexpr string_view_type& access_underlying_view() noexcept {
        return viewing;
    }
    [[nodiscard]] constexpr const string_view_type& access_underlying_view() const noexcept {
        return viewing;
    }

#ifdef __cpp_constexpr_dynamic_alloc
    constexpr
#endif
    ~basic_string_or_view() noexcept {
        switch (tag) {
        case VIEWING:
            viewing.~basic_string_view();
            break;
        case OWNING:
            owning.~basic_string();
            break;
        STRING_OR_VIEW_UNREACHABLE_DEFAULT;
        }
        tag = static_cast<tag_t>(-1);  // Should be optimized out; Just marking now object is in an invalid state.
    }

private:
    constexpr void swap_my_viewing_with_other_owning(basic_string_or_view& other) noexcept {
        string_view_type tmp = viewing;
        viewing.~basic_string_view();
        construct_owning(static_cast<string_type&&>(other.owning));
        other.owning.~basic_string();
        other.construct_viewing(tmp);
    }

    constexpr void copy_string_when_holding_view(string_type s) noexcept {
        viewing.~basic_string_view();
        construct_owning(static_cast<string_type&&>(s));
    }

    template<typename... Args>
    static constexpr bool string_type_nothrow_constructible() noexcept {
        return noexcept(::new (static_cast<void*>(nullptr), constexpr_new_tag{0}) string_type(::std::declval<Args>()...));
    }

    template<typename... Args>
    static constexpr bool string_view_type_nothrow_constructible() noexcept {
        return noexcept(::new (static_cast<void*>(nullptr), constexpr_new_tag{0}) string_view_type(::std::declval<Args>()...));
    }

    template<bool AllowExceptions = false, typename... Args>
    constexpr void construct_owning(Args&&... args) noexcept(string_type_nothrow_constructible<Args...>()) {
        static_assert(string_type_nothrow_constructible<Args...>() != AllowExceptions, "Either the constructor must be noexcept (to prevent being in an invalid state) or AllowExceptions in the few places where an invalid state is fine. But not both because that's a logical error.");
        ::new (static_cast<void*>(::std::addressof(owning)), constexpr_new_tag{0}) string_type(static_cast<Args&&>(args)...);
        tag = OWNING;
    }

    template<bool AllowExceptions = false, typename... Args>
    constexpr void construct_viewing(Args&&... args) noexcept(string_view_type_nothrow_constructible<Args...>()) {
        static_assert(string_view_type_nothrow_constructible<Args...>() != AllowExceptions, "Either the constructor must be noexcept (to prevent being in an invalid state) or AllowExceptions in the few places where an invalid state is fine. But not both because that's a logical error.");
        ::new (static_cast<void*>(::std::addressof(viewing)), constexpr_new_tag{0}) string_view_type(static_cast<Args&&>(args)...);
        tag = VIEWING;
    }

    struct empty_type {};

    [[no_unique_address]] union {
        [[no_unique_address]] string_view_type viewing;
        [[no_unique_address]] string_type owning;
        [[maybe_unused, no_unique_address]] empty_type monostate{};  // For constexpr-ness, where every field needs to be initialised
    };
    // previous union should be aligned on pointer, which should be the same align as size_t
    // (Not using enum to prevent warnings about the `default: unreachable()` branch on every switch)
    using tag_t = size_type;
    [[no_unique_address]] tag_t tag = static_cast<tag_t>(2);
    static constexpr tag_t OWNING = static_cast<tag_t>(1);
    static constexpr tag_t VIEWING = static_cast<tag_t>(0);
};

template<typename StringOrView, typename Allocator = void>
struct to_string_or_view;

template<typename CharT, typename Traits, typename Allocator>
struct to_string_or_view<std::basic_string<CharT, Traits, Allocator>, void>
#ifdef __cpp_lib_type_identity
    : std::type_identity<
#else
    { using type =
#endif
        basic_string_or_view<CharT, Traits, Allocator>
#ifdef __cpp_lib_type_identity
    > {
#else
    ;
#endif
};

template<typename CharT, typename Traits, typename Allocator, typename ReplacementAllocator>
struct to_string_or_view<std::basic_string<CharT, Traits, Allocator>, ReplacementAllocator>
#ifdef __cpp_lib_type_identity
    : std::type_identity<
#else
    { using type =
#endif
        basic_string_or_view<CharT, Traits, ReplacementAllocator>
#ifdef __cpp_lib_type_identity
    > {
#else
    ;
#endif
};

template<typename CharT, typename Traits>
struct to_string_or_view<std::basic_string<CharT, Traits>, void>
#ifdef __cpp_lib_type_identity
    : std::type_identity<
#else
    { using type =
#endif
        basic_string_or_view<CharT, Traits>
#ifdef __cpp_lib_type_identity
    > {
#else
    ;
#endif
};

template<typename CharT, typename Traits, typename ReplacementAllocator>
struct to_string_or_view<std::basic_string_view<CharT, Traits>, ReplacementAllocator>
#ifdef __cpp_lib_type_identity
    : std::type_identity<
#else
    { using type =
#endif
        basic_string_or_view<CharT, Traits, ReplacementAllocator>
#ifdef __cpp_lib_type_identity
    > {
#else
    ;
#endif
};

// to_string_or_view_t<std::basic_string<...>> can own a std::basic_string<...>
// to_string_or_view_t<std::basic_string_view<...>> can view a std::basic_string_view<...>
template<typename StringOrView, typename Allocator = void>
using to_string_or_view_t = typename to_string_or_view<StringOrView, Allocator>::type;


using string_or_view = to_string_or_view_t<std::string>;
using wstring_or_view = to_string_or_view_t<std::wstring>;
using u16string_or_view = to_string_or_view_t<std::u16string>;
using u32string_or_view = to_string_or_view_t<std::u32string>;


namespace pmr {
    using string_or_view = to_string_or_view_t<std::pmr::string>;
    using wstring_or_view = to_string_or_view_t<std::pmr::wstring>;
    using u16string_or_view = to_string_or_view_t<std::pmr::u16string>;
    using u32string_or_view = to_string_or_view_t<std::pmr::u32string>;
}

#ifdef __cpp_lib_char8_t
using u8string_or_view = to_string_or_view_t<std::u8string>;
namespace pmr {
    using u8string_or_view = to_string_or_view_t<std::pmr::u8string>;
}
#endif

namespace std {

    template<typename CharT, typename Traits, typename Allocator>
    struct hash<basic_string_or_view<CharT, Traits, Allocator>> : private hash<basic_string_view<CharT, Traits>> {
        using argument_type = basic_string_or_view<CharT, Traits, Allocator>;
        using result_type = size_t;

        constexpr result_type operator()(const argument_type& s) const noexcept(noexcept(static_cast<const hash<basic_string_view<CharT, Traits>>&>(*this)(*s))) {
            return static_cast<const hash<basic_string_view<CharT, Traits>>&>(*this)(*s);
        }
    };

}

#endif  // STRING_OR_VIEW_UNREACHABLE_DEFAULT
