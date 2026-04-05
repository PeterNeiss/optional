#include "optional.hpp"
#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <optional>

using namespace std_proposal;

// Test framework macros
#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Running " #name "..."; \
    test_##name(); \
    std::cout << " PASSED\n"; \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        std::cerr << "Assertion failed: " #cond " at line " << __LINE__ << "\n"; \
        std::abort(); \
    } \
} while(0)

#define ASSERT_THROWS(expr, exception_type) do { \
    bool caught = false; \
    try { \
        expr; \
    } catch (const exception_type&) { \
        caught = true; \
    } \
    ASSERT(caught); \
} while(0)

// ============================================================================
// slim_optional basic tests
// ============================================================================

TEST(sentinel_default_construction) {
    slim_optional<int*, nullptr> opt;
    ASSERT(!opt.has_value());
    ASSERT(!opt);
}

TEST(sentinel_nullopt_construction) {
    slim_optional<int*, nullptr> opt(nullopt);
    ASSERT(!opt.has_value());
}

TEST(sentinel_std_nullopt_construction) {
    slim_optional<int*, nullptr> opt(std::nullopt);
    ASSERT(!opt.has_value());
}

TEST(sentinel_value_construction) {
    int x = 42;
    slim_optional<int*, nullptr> opt(&x);
    ASSERT(opt.has_value());
    ASSERT(**opt == 42);
}

TEST(sentinel_rejects_sentinel_value) {
    ASSERT_THROWS((slim_optional<int*, nullptr>(nullptr)), bad_optional_access);
}

TEST(sentinel_copy_construction) {
    int x = 42;
    slim_optional<int*, nullptr> opt1(&x);
    slim_optional<int*, nullptr> opt2(opt1);
    ASSERT(opt2.has_value());
    ASSERT(*opt1 == *opt2);
}

TEST(sentinel_move_construction) {
    int x = 42;
    slim_optional<int*, nullptr> opt1(&x);
    slim_optional<int*, nullptr> opt2(std::move(opt1));
    ASSERT(opt2.has_value());
    ASSERT(**opt2 == 42);
}

TEST(sentinel_in_place_construction) {
    int x = 42;
    slim_optional<int*, nullptr> opt(in_place, &x);
    ASSERT(opt.has_value());
    ASSERT(**opt == 42);
}

TEST(sentinel_std_in_place_construction) {
    int x = 42;
    slim_optional<int*, nullptr> opt(std::in_place, &x);
    ASSERT(opt.has_value());
    ASSERT(**opt == 42);
}

TEST(sentinel_assignment) {
    int x = 42;
    slim_optional<int*, nullptr> opt;
    opt = &x;
    ASSERT(opt.has_value());
    ASSERT(**opt == 42);

    opt = nullopt;
    ASSERT(!opt.has_value());
}

TEST(sentinel_std_nullopt_assignment) {
    int x = 42;
    slim_optional<int*, nullptr> opt(&x);
    opt = std::nullopt;
    ASSERT(!opt.has_value());
}

TEST(sentinel_assignment_rejects_sentinel) {
    slim_optional<int*, nullptr> opt;
    ASSERT_THROWS(opt = nullptr, bad_optional_access);
}

TEST(sentinel_copy_assignment) {
    int x = 42;
    slim_optional<int*, nullptr> opt1(&x);
    slim_optional<int*, nullptr> opt2;
    opt2 = opt1;
    ASSERT(opt2.has_value());
    ASSERT(*opt1 == *opt2);
}

TEST(sentinel_move_assignment) {
    int x = 42;
    slim_optional<int*, nullptr> opt1(&x);
    slim_optional<int*, nullptr> opt2;
    opt2 = std::move(opt1);
    ASSERT(opt2.has_value());
    ASSERT(**opt2 == 42);
}

TEST(sentinel_value) {
    int x = 42;
    slim_optional<int*, nullptr> opt(&x);
    ASSERT(opt.value() == &x);

    slim_optional<int*, nullptr> empty;
    ASSERT_THROWS(empty.value(), bad_optional_access);
}

TEST(sentinel_value_or) {
    int x = 42;
    int y = 99;
    slim_optional<int*, nullptr> opt(&x);
    ASSERT(opt.value_or(&y) == &x);

    slim_optional<int*, nullptr> empty;
    ASSERT(empty.value_or(&y) == &y);
}

TEST(sentinel_reset) {
    int x = 42;
    slim_optional<int*, nullptr> opt(&x);
    ASSERT(opt.has_value());
    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(sentinel_emplace) {
    int x = 42;
    slim_optional<int*, nullptr> opt;
    opt.emplace(&x);
    ASSERT(opt.has_value());
    ASSERT(**opt == 42);
}

TEST(sentinel_emplace_rejects_sentinel) {
    slim_optional<int*, nullptr> opt;
    ASSERT_THROWS(opt.emplace(nullptr), bad_optional_access);
}

TEST(sentinel_swap) {
    int x = 42;
    int y = 99;
    slim_optional<int*, nullptr> opt1(&x);
    slim_optional<int*, nullptr> opt2(&y);

    opt1.swap(opt2);
    ASSERT(**opt1 == 99);
    ASSERT(**opt2 == 42);

    slim_optional<int*, nullptr> opt3;
    opt1.swap(opt3);
    ASSERT(!opt1.has_value());
    ASSERT(opt3.has_value());
    ASSERT(**opt3 == 99);
}

TEST(sentinel_comparison) {
    int x = 42;
    int y = 99;
    slim_optional<int*, nullptr> opt1(&x);
    slim_optional<int*, nullptr> opt2(&x);
    slim_optional<int*, nullptr> opt3(&y);
    slim_optional<int*, nullptr> empty;

    ASSERT(opt1 == opt2);
    ASSERT(opt1 != opt3);
    ASSERT(opt1 != empty);
    ASSERT(empty == nullopt);
    ASSERT(empty == std::nullopt);
    ASSERT(opt1 == &x);
    ASSERT(opt1 != &y);
}

TEST(sentinel_monadic_transform) {
    int x = 42;
    slim_optional<int*, nullptr> opt(&x);
    auto result = opt.transform([](int* p) { return *p * 2; });
    ASSERT(result.has_value());
    ASSERT(*result == 84);

    slim_optional<int*, nullptr> empty;
    auto result2 = empty.transform([](int* p) { return *p * 2; });
    ASSERT(!result2.has_value());
}

TEST(sentinel_monadic_and_then) {
    int x = 42;
    slim_optional<int*, nullptr> opt(&x);
    auto result = opt.and_then([](int* p) -> slim_optional<int, -1> {
        if (*p > 0) return *p * 2;
        return nullopt;
    });
    ASSERT(result.has_value());
    ASSERT(*result == 84);
}

TEST(sentinel_monadic_or_else) {
    slim_optional<int*, nullptr> empty;
    int y = 99;
    auto result = empty.or_else([&y]() -> slim_optional<int*, nullptr> {
        return &y;
    });
    ASSERT(result.has_value());
    ASSERT(**result == 99);
}

// ============================================================================
// Interop with std::optional tests
// ============================================================================

TEST(interop_construct_from_std_optional) {
    int x = 42;
    std::optional<int*> std_opt(&x);
    slim_optional<int*, nullptr> slim(std_opt);
    ASSERT(slim.has_value());
    ASSERT(**slim == 42);

    std::optional<int*> empty_std;
    slim_optional<int*, nullptr> slim2(empty_std);
    ASSERT(!slim2.has_value());
}

TEST(interop_assign_from_std_optional) {
    int x = 42;
    std::optional<int*> std_opt(&x);
    slim_optional<int*, nullptr> slim;
    slim = std_opt;
    ASSERT(slim.has_value());
    ASSERT(**slim == 42);

    std::optional<int*> empty_std;
    slim = empty_std;
    ASSERT(!slim.has_value());
}

TEST(interop_convert_to_std_optional) {
    int x = 42;
    slim_optional<int*, nullptr> slim(&x);
    std::optional<int*> std_opt = slim;
    ASSERT(std_opt.has_value());
    ASSERT(**std_opt == 42);

    slim_optional<int*, nullptr> empty;
    std::optional<int*> empty_std = empty;
    ASSERT(!empty_std.has_value());
}

TEST(interop_compare_with_std_optional) {
    int x = 42;
    slim_optional<int*, nullptr> slim(&x);
    std::optional<int*> std_opt(&x);

    ASSERT(slim == std_opt);
    ASSERT(std_opt == slim);

    slim.reset();
    ASSERT(slim != std_opt);

    std_opt.reset();
    ASSERT(slim == std_opt);  // Both empty
}

// ============================================================================
// Enum sentinel tests
// ============================================================================

enum class Status : int32_t {
    INVALID = -1,
    OK = 0,
    ERROR = 1,
    WARNING = 2
};

TEST(enum_sentinel_basic) {
    slim_optional<Status, Status::INVALID> opt;
    ASSERT(!opt.has_value());

    opt = Status::OK;
    ASSERT(opt.has_value());
    ASSERT(*opt == Status::OK);

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(enum_sentinel_rejects_sentinel) {
    ASSERT_THROWS((slim_optional<Status, Status::INVALID>(Status::INVALID)),
                  bad_optional_access);
}

TEST(enum_sentinel_sizeof) {
    // slim_optional should be same size as enum
    ASSERT(sizeof(slim_optional<Status, Status::INVALID>) == sizeof(Status));

    // std::optional is larger
    ASSERT(sizeof(std::optional<Status>) > sizeof(Status));
}

// ============================================================================
// Integer sentinel tests
// ============================================================================

TEST(integer_sentinel_basic) {
    slim_optional<int32_t, -1> opt;
    ASSERT(!opt.has_value());

    opt = 42;
    ASSERT(opt.has_value());
    ASSERT(*opt == 42);

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(integer_sentinel_rejects_sentinel) {
    ASSERT_THROWS((slim_optional<int32_t, -1>(-1)), bad_optional_access);

    slim_optional<int32_t, -1> opt;
    ASSERT_THROWS(opt = -1, bad_optional_access);
}

TEST(integer_sentinel_sizeof) {
    ASSERT(sizeof(slim_optional<int32_t, -1>) == sizeof(int32_t));
    ASSERT(sizeof(std::optional<int32_t>) > sizeof(int32_t));
}

// ============================================================================
// Constexpr tests
// ============================================================================

constexpr bool constexpr_sentinel_test() {
    slim_optional<int, -1> opt;
    if (opt.has_value()) return false;

    opt = 42;
    if (!opt.has_value()) return false;
    if (*opt != 42) return false;

    opt.reset();
    if (opt.has_value()) return false;

    return true;
}

TEST(constexpr_sentinel) {
    static_assert(constexpr_sentinel_test());
    ASSERT(constexpr_sentinel_test());
}

// ============================================================================
// Size tests
// ============================================================================

TEST(sizeof_comparisons) {
    // Pointers
    ASSERT(sizeof(slim_optional<int*, nullptr>) == sizeof(int*));
    ASSERT(sizeof(std::optional<int*>) > sizeof(int*));

    // 32-bit integers
    ASSERT(sizeof(slim_optional<int32_t, -1>) == sizeof(int32_t));
    ASSERT(sizeof(std::optional<int32_t>) > sizeof(int32_t));

    // 64-bit integers
    ASSERT(sizeof(slim_optional<int64_t, -1L>) == sizeof(int64_t));
    ASSERT(sizeof(std::optional<int64_t>) > sizeof(int64_t));

    std::cout << "  sizeof(std::optional<int*>): " << sizeof(std::optional<int*>) << "\n";
    std::cout << "  sizeof(slim_optional<int*, nullptr>): " << sizeof(slim_optional<int*, nullptr>) << "\n";
    std::cout << "  sizeof(std::optional<int32_t>): " << sizeof(std::optional<int32_t>) << "\n";
    std::cout << "  sizeof(slim_optional<int32_t, -1>): " << sizeof(slim_optional<int32_t, -1>) << "\n";
}

// ============================================================================
// Custom type tests
// ============================================================================

struct Point {
    int x, y;

    constexpr Point(int x_, int y_) : x(x_), y(y_) {}

    constexpr bool operator==(const Point&) const = default;
    constexpr auto operator<=>(const Point&) const = default;
};

constexpr Point INVALID_POINT{-9999, -9999};

TEST(custom_type_sentinel) {
    slim_optional<Point, INVALID_POINT> opt;
    ASSERT(!opt.has_value());

    opt = Point{10, 20};
    ASSERT(opt.has_value());
    ASSERT(opt->x == 10);
    ASSERT(opt->y == 20);

    opt.emplace(5, 15);
    ASSERT(opt->x == 5);
    ASSERT(opt->y == 15);

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(custom_type_rejects_sentinel) {
    ASSERT_THROWS((slim_optional<Point, INVALID_POINT>(INVALID_POINT)),
                  bad_optional_access);
}

// ============================================================================
// Hash tests
// ============================================================================

TEST(hash_support) {
    int x = 42;
    slim_optional<int*, nullptr> opt1(&x);
    slim_optional<int*, nullptr> opt2(&x);
    slim_optional<int*, nullptr> empty;

    std::hash<slim_optional<int*, nullptr>> hasher;

    ASSERT(hasher(opt1) == hasher(opt2));
    ASSERT(hasher(empty) == 0);

    // Hash of slim_optional should match hash of value
    ASSERT(hasher(opt1) == std::hash<int*>{}(&x));
}

// ============================================================================
// make_slim_optional tests
// ============================================================================

TEST(make_slim_optional_test) {
    int x = 42;
    auto opt = make_slim_optional<int*, nullptr>(&x);
    ASSERT(opt.has_value());
    ASSERT(**opt == 42);
}

// ============================================================================
// Container tests
// ============================================================================

TEST(vector_of_slim_optionals) {
    int x = 1, y = 2, z = 3;

    std::vector<slim_optional<int*, nullptr>> vec;
    vec.push_back(&x);
    vec.push_back(nullopt);
    vec.push_back(&y);
    vec.push_back(&z);

    ASSERT(vec.size() == 4);
    ASSERT(vec[0].has_value() && **vec[0] == 1);
    ASSERT(!vec[1].has_value());
    ASSERT(vec[2].has_value() && **vec[2] == 2);
    ASSERT(vec[3].has_value() && **vec[3] == 3);
}

// ============================================================================
// Main test runner
// ============================================================================

int main() {
    std::cout << "Running slim_optional Tests\n";
    std::cout << "======================================\n\n";

    std::cout << "slim_optional basic tests:\n";
    RUN_TEST(sentinel_default_construction);
    RUN_TEST(sentinel_nullopt_construction);
    RUN_TEST(sentinel_std_nullopt_construction);
    RUN_TEST(sentinel_value_construction);
    RUN_TEST(sentinel_rejects_sentinel_value);
    RUN_TEST(sentinel_copy_construction);
    RUN_TEST(sentinel_move_construction);
    RUN_TEST(sentinel_in_place_construction);
    RUN_TEST(sentinel_std_in_place_construction);
    RUN_TEST(sentinel_assignment);
    RUN_TEST(sentinel_std_nullopt_assignment);
    RUN_TEST(sentinel_assignment_rejects_sentinel);
    RUN_TEST(sentinel_copy_assignment);
    RUN_TEST(sentinel_move_assignment);
    RUN_TEST(sentinel_value);
    RUN_TEST(sentinel_value_or);
    RUN_TEST(sentinel_reset);
    RUN_TEST(sentinel_emplace);
    RUN_TEST(sentinel_emplace_rejects_sentinel);
    RUN_TEST(sentinel_swap);
    RUN_TEST(sentinel_comparison);
    RUN_TEST(sentinel_monadic_transform);
    RUN_TEST(sentinel_monadic_and_then);
    RUN_TEST(sentinel_monadic_or_else);

    std::cout << "\nInterop with std::optional tests:\n";
    RUN_TEST(interop_construct_from_std_optional);
    RUN_TEST(interop_assign_from_std_optional);
    RUN_TEST(interop_convert_to_std_optional);
    RUN_TEST(interop_compare_with_std_optional);

    std::cout << "\nEnum sentinel tests:\n";
    RUN_TEST(enum_sentinel_basic);
    RUN_TEST(enum_sentinel_rejects_sentinel);
    RUN_TEST(enum_sentinel_sizeof);

    std::cout << "\nInteger sentinel tests:\n";
    RUN_TEST(integer_sentinel_basic);
    RUN_TEST(integer_sentinel_rejects_sentinel);
    RUN_TEST(integer_sentinel_sizeof);

    std::cout << "\nConstexpr tests:\n";
    RUN_TEST(constexpr_sentinel);

    std::cout << "\nSize tests:\n";
    RUN_TEST(sizeof_comparisons);

    std::cout << "\nCustom type tests:\n";
    RUN_TEST(custom_type_sentinel);
    RUN_TEST(custom_type_rejects_sentinel);

    std::cout << "\nHash tests:\n";
    RUN_TEST(hash_support);

    std::cout << "\nmake_slim_optional tests:\n";
    RUN_TEST(make_slim_optional_test);

    std::cout << "\nContainer tests:\n";
    RUN_TEST(vector_of_slim_optionals);

    std::cout << "\n======================================\n";
    std::cout << "All tests passed successfully!\n";
    return 0;
}
