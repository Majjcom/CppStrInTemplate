#include <iostream>
#include "cstr.h"

#include <codecvt>

using namespace std;

// Convenience wrapper: prints a compile-time string to stdout.
template <CStr Str>
void print_template_str() {
    cout << toCs<Str>().str();
}

int main() {
    // Basic usage: string literals as template arguments (C++20 NTTP)
    print_template_str<"Hello, ">();
    print_template_str<"World!!!">();
    cout << endl;

    // Two-string concat
    constexpr auto concat1 = concatStr<"A and ", "B!">();
    print_template_str<concat1>();
    cout << endl;

    // Variadic concat: "AB" + "CD" + "EF" -> "ABCDEF"
    constexpr auto concat2 = concatStr<"AB", "CD", "EF">();
    cout << toCs<concat2>().str() << endl;

    // Concat with a prior result: "ABCDEF" + "GH" -> "ABCDEFGH"
    constexpr auto concat3 = concatStr<concat2, "GH">();
    cout << toCs<concat3>().str() << endl;

    // substr with explicit length: "ABCDEFGH"[2..4] -> "CDE"
    constexpr auto sub1 = substrStr<concat3, 2, 3>();
    cout << toCs<sub1>().str() << endl;

    // substr to end: "ABCDEFGH"[4..] -> "EFGH"
    constexpr auto sub2 = substrStr<concat3, 4>();
    cout << toCs<sub2>().str() << endl;

    // findStr: "ABCDEFGH" contains "CDE" at index 2
    constexpr size_t pos1 = findStr<concat3, "CDE">();
    cout << pos1 << endl;  // 2

    // findStr: pattern not present -> npos
    constexpr size_t pos2 = findStr<concat3, "XY">();
    cout << (pos2 == npos ? "npos" : "found") << endl;

    // operator==: same content -> true
    static_assert(CStr("ABC") == CStr("ABC"));
    // operator==: different content -> false
    static_assert(CStr("ABC") != CStr("ABD"));
    // operator==: different lengths -> false (cross-size free function)
    static_assert(CStr("AB") != CStr("ABC"));
    cout << "operator==: all assertions passed" << endl;

    // strcmpStr: equal strings -> 0
    static_assert(strcmpStr<"ABC", "ABC">() == 0);
    // strcmpStr: lexicographic order
    static_assert(strcmpStr<"ABC", "ABD">() < 0);
    static_assert(strcmpStr<"ABD", "ABC">() > 0);
    // strcmpStr: prefix is less than longer string
    static_assert(strcmpStr<"AB", "ABC">() < 0);
    cout << "strcmpStr: all assertions passed" << endl;

    // wchar_t support: variadic concat of wide strings, then compare with strcmpStr
    constexpr auto wCstring = concatStr<L"你好", L"世界", L"!!">();
    static_assert(strcmpStr<wCstring, L"你好世界!!">() == 0);
    cout << "strcmpStr: wstring assertions passed" << endl;

    return 0;
}
