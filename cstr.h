#pragma once

#include <string_view>

// NTTP-compatible fixed-size string. N includes the null terminator.
// C++20 allows this struct to be used directly as a template parameter,
// enabling string literals like template_fn<"hello">().
template <size_t N, typename CharT = char>
struct CStr {
    using charT = CharT;

    CharT _str[N]{};

    // Construct from a string literal: CStr<6>("hello")
    constexpr CStr(const CharT (&str)[N]) {
        for (size_t i = 0; i < N; ++i) {
            _str[i] = str[i];
        }
    }

    // Construct from a pointer + length; used internally by substrStr/concatStrImpl
    // to build a CStr from a temporary buffer inside constexpr functions.
    explicit constexpr CStr(const CharT* str, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            _str[i] = str[i];
        }
        _str[len] = CharT{};
    }

    constexpr CharT operator[](size_t i) const {
        return _str[i];
    }

    // Index as a template argument required by toCs() to expand
    // characters into a variadic pack via index_sequence.
    template <size_t I>
    constexpr CharT get() const {
        static_assert(I < N);
        return _str[I];
    }

    // Returns string length excluding the null terminator.
    constexpr size_t size() const {
        return N - 1;
    }
};

// Cross-size equality: CStr<N1> and CStr<N2> are different types, so the
// member operator== only covers same-length strings. This free function
// handles the general case different lengths are always unequal.
template <size_t N1, size_t N2, typename CharT>
constexpr bool operator==(const CStr<N1, CharT>& a, const CStr<N2, CharT>& b) {
    if constexpr (N1 != N2) return false;
    for (size_t i = 0; i < N1 - 1; ++i) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// Holds a string as a variadic pack of char template arguments,
// making the string content part of the type itself.
// Useful when you need the characters to participate in template deduction.
template <typename CharT, typename ViewT, CharT... Cs>
struct CStrWrap {
    static constexpr auto N = sizeof...(Cs);
    CharT _str[N]{ Cs... };  // Runtime storage initialized from the pack expansion

    // Returns a string_view / wstring_view over the stored characters (no null terminator needed).
    constexpr ViewT str() const {
        return ViewT(_str, N);
    }
};

// Converts a CStr NTTP into a CStrWrap by unpacking each character
// using an index_sequence. This "explodes" the string into its type.
template <CStr Str, typename CharT = typename decltype(Str)::charT>
constexpr auto toCs()
    requires std::is_same_v<CharT, char> {
    return [] <size_t... N>(std::index_sequence<N...>) {
        return CStrWrap<CharT, std::string_view, Str.template get<N>()...>();
    }(std::make_index_sequence<Str.size()>());
}

// wchar_t overload of toCs(): same mechanics, produces CStrWrap with wstring_view.
template <CStr Str, typename CharT = typename decltype(Str)::charT>
constexpr auto toCs()
    requires std::is_same_v<CharT, wchar_t> {
    return [] <size_t... N>(std::index_sequence<N...>) {
        return CStrWrap<CharT, std::wstring_view, Str.template get<N>()...>();
    }(std::make_index_sequence<Str.size()>());
}

// Extracts a compile-time substring. Len defaults to "rest of string".
//   substrStr<"ABCDEF", 2, 3>()  ->  "CDE"
//   substrStr<"ABCDEF", 4>()     ->  "EF"
template <CStr Str, size_t Start, size_t Len = Str.size() - Start>
constexpr auto substrStr() {
    static_assert(Start <= Str.size());
    static_assert(Start + Len <= Str.size());
    char buf[Len + 1]{};
    for (size_t i = 0; i < Len; ++i) {
        buf[i] = Str[Start + i];
    }
    return CStr<Len + 1>(buf, Len);
}

// Sentinel returned by findStr when the pattern is not found.
inline constexpr size_t npos = size_t(-1);

// Finds the first occurrence of Pattern in Str at compile time.
// Returns the starting index, or npos if not found.
//   findStr<"ABCDEF", "CD">()  ->  2
//   findStr<"ABCDEF", "XY">()  ->  npos
template <CStr Str, CStr Pattern>
constexpr size_t findStr() {
    if constexpr (Pattern.size() > Str.size()) return npos;
    for (size_t i = 0; i <= Str.size() - Pattern.size(); ++i) {
        bool match = true;
        for (size_t j = 0; j < Pattern.size(); ++j) {
            if (Str[i + j] != Pattern[j]) {
                match = false;
                break;
            }
        }
        if (match) return i;
    }
    return npos;
}

// Compares two compile-time strings lexicographically, mirroring strcmp semantics.
// Returns negative if Str1 < Str2, 0 if equal, positive if Str1 > Str2.
//   strcmpStr<"ABC", "ABC">()  ->  0
//   strcmpStr<"ABC", "ABD">()  ->  negative
//   strcmpStr<"ABD", "ABC">()  ->  positive
template <CStr Str1, CStr Str2>
constexpr int strcmpStr() {
    constexpr size_t len = Str1.size() < Str2.size() ? Str1.size() : Str2.size();
    for (size_t i = 0; i < len; ++i) {
        if (Str1[i] != Str2[i])
            return static_cast<unsigned char>(Str1[i]) - static_cast<unsigned char>(Str2[i]);
    }
    // All compared chars equal; longer string is greater
    return static_cast<int>(Str1.size()) - static_cast<int>(Str2.size());
}

// Two-string helper used by concatStr. Allocates a local buffer,
// copies both strings in, then returns a new CStr with size N1+N2-1
// (the -1 collapses the two null terminators into one).
template <size_t N1, size_t N2, typename CharT>
constexpr auto concatStrImpl(const CStr<N1, CharT>& str1, const CStr<N2, CharT>& str2) {
    CharT str[N1 + N2 - 1]{};
    for (size_t i = 0; i < str1.size(); ++i) {
        str[i] = str1[i];
    }
    for (size_t i = 0; i < str2.size(); ++i) {
        str[i + str1.size()] = str2[i];
    }
    str[str1.size() + str2.size()] = '\0';

    return CStr<N1 + N2 - 1, CharT>(str);
}

// Variadic compile-time string concatenation. Folds left via recursion:
//   concatStr<"A", "B", "C">()  ->  concatStr<concatStrImpl("A","B"), "C">()
template <CStr Str1, CStr Str2, CStr... Rest>
constexpr auto concatStr() {
    if constexpr (sizeof...(Rest) == 0) {
        return concatStrImpl(Str1, Str2);
    } else {
        return concatStr<concatStrImpl(Str1, Str2), Rest...>();
    }
}
