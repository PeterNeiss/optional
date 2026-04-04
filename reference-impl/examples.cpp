#include "optional.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <cassert>

using namespace std_proposal;

// ============================================================================
// Example 1: Basic usage - Backwards compatibility
// ============================================================================

void example_backwards_compatibility() {
    std::cout << "=== Example 1: Backwards Compatibility ===\n";

    // Traditional optional - uses bool storage
    optional<int> old_opt;
    std::cout << "sizeof(optional<int>): " << sizeof(old_opt) << " bytes\n";

    old_opt = 42;
    if (old_opt.has_value()) {
        std::cout << "Value: " << *old_opt << "\n";
    }

    old_opt.reset();
    std::cout << "After reset, has_value: " << old_opt.has_value() << "\n\n";
}

// ============================================================================
// Example 2: Sentinel-based pointers
// ============================================================================

void example_sentinel_pointers() {
    std::cout << "=== Example 2: Sentinel-Based Pointers ===\n";

    // Sentinel-based optional - no bool needed!
    optional<int*, nullptr> ptr_opt;
    std::cout << "sizeof(optional<int*, nullptr>): " << sizeof(ptr_opt) << " bytes\n";

    int value = 42;
    ptr_opt = &value;

    if (ptr_opt) {
        std::cout << "Pointer points to: " << **ptr_opt << "\n";
    }

    ptr_opt.reset();
    std::cout << "After reset, has_value: " << ptr_opt.has_value() << "\n";

    // Container of optional pointers
    std::vector<optional<int*, nullptr>> pointers;
    pointers.push_back(&value);
    pointers.push_back(nullopt);
    pointers.push_back(&value);

    std::cout << "Container size (3 elements): "
              << pointers.size() * sizeof(optional<int*, nullptr>)
              << " bytes\n\n";
}

// ============================================================================
// Example 3: Enums with invalid values
// ============================================================================

enum class FileHandle : int32_t {
    INVALID = -1,
    STDIN = 0,
    STDOUT = 1,
    STDERR = 2
};

void example_enum_sentinel() {
    std::cout << "=== Example 3: Enum with Sentinel ===\n";

    // Traditional optional - 8 bytes (bool + int32_t + padding)
    optional<FileHandle> handle1;
    std::cout << "sizeof(optional<FileHandle>): " << sizeof(handle1) << " bytes\n";

    // Sentinel-based - 4 bytes (just the enum!)
    optional<FileHandle, FileHandle::INVALID> handle2;
    std::cout << "sizeof(optional<FileHandle, INVALID>): " << sizeof(handle2) << " bytes\n";

    handle2 = FileHandle::STDOUT;
    if (handle2) {
        std::cout << "Handle is: " << static_cast<int>(*handle2) << "\n";
    }

    handle2 = nullopt;
    std::cout << "After nullopt, has_value: " << handle2.has_value() << "\n\n";
}

// ============================================================================
// Example 4: Constexpr usage
// ============================================================================

constexpr optional<int, -1> make_optional_int() {
    return 42;
}

constexpr optional<int, -1> make_empty_int() {
    return nullopt;
}

void example_constexpr() {
    std::cout << "=== Example 4: Constexpr Usage ===\n";

    static_assert(make_optional_int().has_value());
    static_assert(*make_optional_int() == 42);
    static_assert(!make_empty_int().has_value());

    constexpr auto opt = make_optional_int();
    std::cout << "Constexpr optional value: " << *opt << "\n";

    // Also test with pointers (runtime)
    int value = 99;
    optional<int*, nullptr> ptr_opt(&value);
    std::cout << "Runtime pointer optional: " << **ptr_opt << "\n\n";
}

// ============================================================================
// Example 5: Heterogeneous comparison
// ============================================================================

void example_heterogeneous_comparison() {
    std::cout << "=== Example 5: Heterogeneous Comparison ===\n";

    optional<int*> old_style;
    optional<int*, nullptr> new_style;

    // Both empty - equal
    std::cout << "Both empty: " << (old_style == new_style) << "\n";

    int x = 42;
    old_style = &x;
    new_style = &x;

    // Both point to same value - equal
    std::cout << "Both point to x: " << (old_style == new_style) << "\n";

    old_style.reset();
    std::cout << "One empty, one not: " << (old_style == new_style) << "\n\n";
}

// ============================================================================
// Example 6: Safety - Sentinel value protection
// ============================================================================

void example_safety() {
    std::cout << "=== Example 6: Safety (Sentinel Protection) ===\n";

    optional<int*, nullptr> opt;

    try {
        // This will throw - can't construct with sentinel value!
        opt = nullptr;
        std::cout << "ERROR: Should have thrown!\n";
    } catch (const bad_optional_access& e) {
        std::cout << "Caught expected exception: " << e.what() << "\n";
    }

    // Correct way to set "no value"
    opt.reset();    // OK
    opt = nullopt;  // OK
    std::cout << "After reset/nullopt, has_value: " << opt.has_value() << "\n";

    // Correct way to set a value
    int x = 42;
    opt = &x;  // OK
    std::cout << "After setting valid pointer, has_value: " << opt.has_value() << "\n\n";
}

// ============================================================================
// Example 7: Value semantics
// ============================================================================

void example_value_semantics() {
    std::cout << "=== Example 7: Value Semantics ===\n";

    optional<int*, nullptr> opt1;
    int x = 100;
    opt1 = &x;

    // Copy construction
    auto opt2 = opt1;
    std::cout << "opt1 == opt2: " << (opt1 == opt2) << "\n";
    std::cout << "Both point to same address: " << (*opt1 == *opt2) << "\n";

    // Move construction
    auto opt3 = std::move(opt1);
    std::cout << "After move, opt3 has_value: " << opt3.has_value() << "\n";
    std::cout << "After move, opt1 still valid: " << opt1.has_value() << "\n";

    // Swap
    optional<int*, nullptr> opt4;
    opt3.swap(opt4);
    std::cout << "After swap, opt3 has_value: " << opt3.has_value() << "\n";
    std::cout << "After swap, opt4 has_value: " << opt4.has_value() << "\n\n";
}

// ============================================================================
// Example 8: Monadic operations
// ============================================================================

void example_monadic() {
    std::cout << "=== Example 8: Monadic Operations ===\n";

    optional<int*, nullptr> opt;
    int x = 42;
    opt = &x;

    // transform
    auto doubled = opt.transform([](int* p) { return *p * 2; });
    std::cout << "Transformed value: " << *doubled << "\n";

    // and_then
    auto result = opt.and_then([](int* p) -> optional<int> {
        if (*p > 0) {
            return *p;
        } else {
            return nullopt;
        }
    });
    std::cout << "and_then result has_value: " << result.has_value() << "\n";
    if (result) {
        std::cout << "and_then result value: " << *result << "\n";
    }

    // or_else
    optional<int*, nullptr> empty_opt;
    auto fallback = empty_opt.or_else([]() -> optional<int*, nullptr> {
        static int default_val = 99;
        return &default_val;
    });
    std::cout << "or_else result: " << **fallback << "\n\n";
}

// ============================================================================
// Example 9: value_or
// ============================================================================

void example_value_or() {
    std::cout << "=== Example 9: value_or ===\n";

    optional<int*, nullptr> opt;
    int default_val = 999;

    int* result = opt.value_or(&default_val);
    std::cout << "value_or when empty: " << *result << "\n";

    int actual_val = 42;
    opt = &actual_val;
    result = opt.value_or(&default_val);
    std::cout << "value_or when has value: " << *result << "\n\n";
}

// ============================================================================
// Example 10: Integer with sentinel value
// ============================================================================

void example_integer_sentinel() {
    std::cout << "=== Example 10: Integer with Sentinel ===\n";

    // Using -1 as sentinel for unsigned indices
    optional<size_t, static_cast<size_t>(-1)> index_opt;
    std::cout << "sizeof(optional<size_t, -1>): " << sizeof(index_opt) << " bytes\n";

    index_opt = 42;
    std::cout << "Index: " << *index_opt << "\n";

    try {
        // Can't set to sentinel value
        index_opt = static_cast<size_t>(-1);
        std::cout << "ERROR: Should have thrown!\n";
    } catch (const bad_optional_access& e) {
        std::cout << "Caught expected exception\n";
    }

    index_opt.reset();
    std::cout << "After reset, has_value: " << index_opt.has_value() << "\n\n";
}

// ============================================================================
// Example 11: Custom type with sentinel
// ============================================================================

struct ResourceHandle {
    int id;

    constexpr bool operator==(const ResourceHandle&) const = default;
    constexpr auto operator<=>(const ResourceHandle&) const = default;
};

constexpr ResourceHandle INVALID_HANDLE{-1};

void example_custom_type() {
    std::cout << "=== Example 11: Custom Type with Sentinel ===\n";

    optional<ResourceHandle, INVALID_HANDLE> handle;
    std::cout << "sizeof(optional<ResourceHandle, INVALID>): " << sizeof(handle) << " bytes\n";

    handle = ResourceHandle{42};
    std::cout << "Handle ID: " << handle->id << "\n";

    handle.reset();
    std::cout << "After reset, has_value: " << handle.has_value() << "\n\n";
}

// ============================================================================
// Example 12: In-place construction
// ============================================================================

struct Point {
    int x, y;

    constexpr Point(int x_, int y_) : x(x_), y(y_) {}

    constexpr bool operator==(const Point&) const = default;
    constexpr auto operator<=>(const Point&) const = default;
};

constexpr Point INVALID_POINT{-1, -1};

void example_in_place() {
    std::cout << "=== Example 12: In-Place Construction ===\n";

    optional<Point, INVALID_POINT> point_opt;

    point_opt.emplace(10, 20);
    std::cout << "Point: (" << point_opt->x << ", " << point_opt->y << ")\n";

    optional<Point, INVALID_POINT> point_opt2(in_place, 5, 15);
    std::cout << "Point2: (" << point_opt2->x << ", " << point_opt2->y << ")\n\n";
}

// ============================================================================
// Example 13: Make optional
// ============================================================================

void example_make_optional() {
    std::cout << "=== Example 13: make_optional ===\n";

    int x = 42;
    auto opt = make_optional<int*, nullptr>(&x);
    std::cout << "make_optional value: " << **opt << "\n";

    auto opt2 = make_optional<Point, INVALID_POINT>(3, 4);
    std::cout << "make_optional Point: (" << opt2->x << ", " << opt2->y << ")\n\n";
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "C++ Sentinel-Based Optional Examples\n";
    std::cout << "====================================\n\n";

    example_backwards_compatibility();
    example_sentinel_pointers();
    example_enum_sentinel();
    example_constexpr();
    example_heterogeneous_comparison();
    example_safety();
    example_value_semantics();
    example_monadic();
    example_value_or();
    example_integer_sentinel();
    example_custom_type();
    example_in_place();
    example_make_optional();

    std::cout << "All examples completed successfully!\n";
    return 0;
}
