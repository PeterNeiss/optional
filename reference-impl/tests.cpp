#include "optional.hpp"
#include <any>
#include <coroutine>
#include <iostream>
#include <cassert>
#include <cmath>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
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
// auto_sentinel tests — scalar types
// ============================================================================

TEST(auto_sentinel_signed_int) {
    slim_optional<int> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(int));

    opt = 42;
    ASSERT(opt.has_value());
    ASSERT(*opt == 42);

    opt = 0;
    ASSERT(opt.has_value());
    ASSERT(*opt == 0);

    opt = -1;
    ASSERT(opt.has_value());
    ASSERT(*opt == -1);

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(auto_sentinel_signed_int_rejects_sentinel) {
    // INT_MIN is the auto sentinel for signed int
    ASSERT_THROWS((slim_optional<int>(std::numeric_limits<int>::min())), bad_optional_access);
}

TEST(auto_sentinel_unsigned_int) {
    slim_optional<unsigned> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(unsigned));

    opt = 0u;
    ASSERT(opt.has_value());
    ASSERT(*opt == 0u);

    opt = 42u;
    ASSERT(opt.has_value());
    ASSERT(*opt == 42u);

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(auto_sentinel_unsigned_rejects_sentinel) {
    ASSERT_THROWS((slim_optional<unsigned>(std::numeric_limits<unsigned>::max())), bad_optional_access);
}

TEST(auto_sentinel_int8) {
    slim_optional<int8_t> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(int8_t));

    opt = int8_t{42};
    ASSERT(opt.has_value());
    ASSERT(*opt == 42);
}

TEST(auto_sentinel_int16) {
    slim_optional<int16_t> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(int16_t));

    opt = int16_t{1000};
    ASSERT(opt.has_value());
    ASSERT(*opt == 1000);
}

TEST(auto_sentinel_int64) {
    slim_optional<int64_t> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(int64_t));

    opt = int64_t{123456789LL};
    ASSERT(opt.has_value());
    ASSERT(*opt == 123456789LL);
}

TEST(auto_sentinel_uint64) {
    slim_optional<uint64_t> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(uint64_t));

    opt = uint64_t{42};
    ASSERT(opt.has_value());
    ASSERT(*opt == 42u);
}

TEST(auto_sentinel_size_t) {
    slim_optional<std::size_t> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(std::size_t));

    opt = std::size_t{100};
    ASSERT(opt.has_value());
    ASSERT(*opt == 100u);
}

// ============================================================================
// auto_sentinel tests — pointer types
// ============================================================================

TEST(auto_sentinel_pointer) {
    slim_optional<int*> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(int*));

    int x = 42;
    opt = &x;
    ASSERT(opt.has_value());
    ASSERT(**opt == 42);

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(auto_sentinel_pointer_rejects_nullptr) {
    ASSERT_THROWS((slim_optional<int*>(nullptr)), bad_optional_access);
}

TEST(auto_sentinel_const_pointer) {
    slim_optional<const char*> opt;
    ASSERT(!opt.has_value());

    opt = "hello";
    ASSERT(opt.has_value());

    opt.reset();
    ASSERT(!opt.has_value());
}

// ============================================================================
// auto_sentinel tests — floating point
// ============================================================================

TEST(auto_sentinel_float) {
    slim_optional<float> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(float));

    opt = 3.14f;
    ASSERT(opt.has_value());
    ASSERT(*opt == 3.14f);

    opt = 0.0f;
    ASSERT(opt.has_value());
    ASSERT(*opt == 0.0f);

    opt = -0.0f;
    ASSERT(opt.has_value());

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(auto_sentinel_double) {
    slim_optional<double> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(double));

    opt = 2.718281828;
    ASSERT(opt.has_value());
    ASSERT(*opt == 2.718281828);

    opt = 0.0;
    ASSERT(opt.has_value());

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(auto_sentinel_float_rejects_nan) {
    ASSERT_THROWS((slim_optional<float>(std::numeric_limits<float>::quiet_NaN())), bad_optional_access);
}

TEST(auto_sentinel_double_rejects_nan) {
    ASSERT_THROWS((slim_optional<double>(std::numeric_limits<double>::quiet_NaN())), bad_optional_access);
}

TEST(auto_sentinel_long_double) {
    slim_optional<long double> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(long double));

    opt = 1.0L;
    ASSERT(opt.has_value());
    ASSERT(*opt == 1.0L);

    opt = 0.0L;
    ASSERT(opt.has_value());

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(auto_sentinel_long_double_rejects_nan) {
    ASSERT_THROWS((slim_optional<long double>(std::numeric_limits<long double>::quiet_NaN())), bad_optional_access);
}

// ============================================================================
// auto_sentinel tests — char16/32 types
// ============================================================================

TEST(auto_sentinel_char16) {
    slim_optional<char16_t> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(char16_t));

    opt = u'A';
    ASSERT(opt.has_value());
    ASSERT(*opt == u'A');

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(auto_sentinel_char32) {
    slim_optional<char32_t> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(char32_t));

    opt = U'\x1F600'; // emoji codepoint
    ASSERT(opt.has_value());

    opt.reset();
    ASSERT(!opt.has_value());
}

// ============================================================================
// auto_sentinel tests — standard library class types
// ============================================================================

TEST(auto_sentinel_unique_ptr) {
    slim_optional<std::unique_ptr<int>> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(std::unique_ptr<int>));

    opt = std::make_unique<int>(42);
    ASSERT(opt.has_value());
    ASSERT(**opt == 42);

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(auto_sentinel_shared_ptr) {
    slim_optional<std::shared_ptr<int>> opt;
    ASSERT(!opt.has_value());

    opt = std::make_shared<int>(99);
    ASSERT(opt.has_value());
    ASSERT(**opt == 99);

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(auto_sentinel_string_view) {
    slim_optional<std::string_view> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(std::string_view));

    opt = std::string_view{"hello"};
    ASSERT(opt.has_value());
    ASSERT(*opt == "hello");

    // Empty string (data != nullptr) is a valid value
    opt = std::string_view{""};
    ASSERT(opt.has_value());
    ASSERT(opt->empty());
    ASSERT(opt->data() != nullptr);

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(auto_sentinel_span) {
    slim_optional<std::span<int>> opt;
    ASSERT(!opt.has_value());

    int arr[] = {1, 2, 3};
    opt = std::span<int>{arr};
    ASSERT(opt.has_value());
    ASSERT(opt->size() == 3);
    ASSERT((*opt)[0] == 1);

    // Empty span with non-null data is valid
    opt = std::span<int>{arr, 0};
    ASSERT(opt.has_value());
    ASSERT(opt->empty());

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(auto_sentinel_function) {
    slim_optional<std::function<int()>> opt;
    ASSERT(!opt.has_value());

    opt = std::function<int()>{[]{ return 42; }};
    ASSERT(opt.has_value());
    ASSERT((*opt)() == 42);

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(auto_sentinel_move_only_function) {
    slim_optional<std::move_only_function<int()>> opt;
    ASSERT(!opt.has_value());

    opt = std::move_only_function<int()>{[]{ return 99; }};
    ASSERT(opt.has_value());
    ASSERT((*opt)() == 99);

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(auto_sentinel_weak_ptr) {
    slim_optional<std::weak_ptr<int>> opt;
    ASSERT(!opt.has_value());

    auto sp = std::make_shared<int>(42);
    opt = std::weak_ptr<int>{sp};
    ASSERT(opt.has_value());
    ASSERT(*opt->lock() == 42);

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(auto_sentinel_weak_ptr_expired_vs_empty) {
    // A weak_ptr whose shared_ptr has died is expired but was assigned —
    // the sentinel is a default-constructed weak_ptr (use_count == 0 && expired),
    // not merely an expired one. However, once the shared_ptr dies the weak_ptr
    // becomes indistinguishable from default-constructed in terms of observable state.
    // This is an inherent limitation: weak_ptr has no "never assigned" flag.
    slim_optional<std::weak_ptr<int>> opt;
    {
        auto sp = std::make_shared<int>(42);
        opt = std::weak_ptr<int>{sp};
        ASSERT(opt.has_value());
    }
    // sp is gone — weak_ptr is now expired, indistinguishable from sentinel
    // This is expected: weak_ptr's sentinel is "expired + use_count==0"
    ASSERT(!opt.has_value());
}

TEST(auto_sentinel_coroutine_handle) {
    slim_optional<std::coroutine_handle<>> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(std::coroutine_handle<>));

    // We can't easily create a real coroutine_handle in a test without
    // a coroutine, but we can verify sentinel behavior
    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(auto_sentinel_any) {
    slim_optional<std::any> opt;
    ASSERT(!opt.has_value());

    opt = std::any{42};
    ASSERT(opt.has_value());
    ASSERT(std::any_cast<int>(*opt) == 42);

    opt = std::any{std::string("hello")};
    ASSERT(opt.has_value());
    ASSERT(std::any_cast<std::string>(*opt) == "hello");

    opt.reset();
    ASSERT(!opt.has_value());
}

// ============================================================================
// auto_sentinel sizeof tests
// ============================================================================

TEST(auto_sentinel_sizeof_comparisons) {
    // All auto-sentinel types should be same size as the underlying type
    ASSERT(sizeof(slim_optional<int>) == sizeof(int));
    ASSERT(sizeof(slim_optional<unsigned>) == sizeof(unsigned));
    ASSERT(sizeof(slim_optional<int64_t>) == sizeof(int64_t));
    ASSERT(sizeof(slim_optional<float>) == sizeof(float));
    ASSERT(sizeof(slim_optional<double>) == sizeof(double));
    ASSERT(sizeof(slim_optional<int*>) == sizeof(int*));
    ASSERT(sizeof(slim_optional<std::string_view>) == sizeof(std::string_view));
    ASSERT(sizeof(slim_optional<std::unique_ptr<int>>) == sizeof(std::unique_ptr<int>));

    // All should be smaller than std::optional
    ASSERT(sizeof(slim_optional<int>) < sizeof(std::optional<int>));
    ASSERT(sizeof(slim_optional<double>) < sizeof(std::optional<double>));
    ASSERT(sizeof(slim_optional<int*>) < sizeof(std::optional<int*>));
    ASSERT(sizeof(slim_optional<std::string_view>) < sizeof(std::optional<std::string_view>));

    std::cout << "  sizeof(slim_optional<int>): " << sizeof(slim_optional<int>) << "\n";
    std::cout << "  sizeof(std::optional<int>): " << sizeof(std::optional<int>) << "\n";
    std::cout << "  sizeof(slim_optional<double>): " << sizeof(slim_optional<double>) << "\n";
    std::cout << "  sizeof(std::optional<double>): " << sizeof(std::optional<double>) << "\n";
    std::cout << "  sizeof(slim_optional<int*>): " << sizeof(slim_optional<int*>) << "\n";
    std::cout << "  sizeof(std::optional<int*>): " << sizeof(std::optional<int*>) << "\n";
    std::cout << "  sizeof(slim_optional<string_view>): " << sizeof(slim_optional<std::string_view>) << "\n";
    std::cout << "  sizeof(std::optional<string_view>): " << sizeof(std::optional<std::string_view>) << "\n";
    std::cout << "  sizeof(slim_optional<unique_ptr<int>>): " << sizeof(slim_optional<std::unique_ptr<int>>) << "\n";
    std::cout << "  sizeof(std::optional<unique_ptr<int>>): " << sizeof(std::optional<std::unique_ptr<int>>) << "\n";
}

// ============================================================================
// auto_sentinel constexpr tests
// ============================================================================

constexpr bool auto_sentinel_constexpr_test() {
    slim_optional<int> i;
    if (i.has_value()) return false;
    i = 42;
    if (*i != 42) return false;
    i.reset();
    if (i.has_value()) return false;

    slim_optional<double> d;
    if (d.has_value()) return false;
    d = 3.14;
    if (*d != 3.14) return false;

    slim_optional<int*> p;
    if (p.has_value()) return false;

    return true;
}

TEST(auto_sentinel_constexpr) {
    static_assert(auto_sentinel_constexpr_test());
    ASSERT(auto_sentinel_constexpr_test());
}

// ============================================================================
// auto_sentinel interop with explicit sentinel
// ============================================================================

TEST(auto_sentinel_interop_with_explicit) {
    // auto sentinel int (INT_MIN) and explicit sentinel int (-1) can interop
    slim_optional<int> auto_opt;
    slim_optional<int, -1> explicit_opt;

    ASSERT(!auto_opt.has_value());
    ASSERT(!explicit_opt.has_value());
    ASSERT(auto_opt == explicit_opt); // both empty

    auto_opt = 42;
    ASSERT(auto_opt != explicit_opt);

    explicit_opt = 42;
    ASSERT(auto_opt == explicit_opt);
}

// ============================================================================
// sentinel_traits customization point tests
// ============================================================================

// A non-structural user type — cannot be an NTTP, but can use auto_sentinel
// via sentinel_traits specialization.
struct Color {
    uint8_t r, g, b, a;
    constexpr bool operator==(const Color&) const = default;
};

namespace std_proposal {
template<>
struct sentinel_traits<Color> {
    static constexpr Color sentinel() noexcept { return {0, 0, 0, 0}; }
    static constexpr bool is_sentinel(const Color& v) noexcept {
        return v.r == 0 && v.g == 0 && v.b == 0 && v.a == 0;
    }
};
}

TEST(sentinel_traits_user_type_basic) {
    // Color can use auto_sentinel thanks to sentinel_traits specialization
    slim_optional<Color> c;
    ASSERT(!c.has_value());

    c = Color{255, 0, 0, 255};
    ASSERT(c.has_value());
    ASSERT(c->r == 255);
    ASSERT(c->a == 255);

    c.reset();
    ASSERT(!c.has_value());

    // Construct with value
    slim_optional<Color> c2{Color{0, 128, 0, 255}};
    ASSERT(c2.has_value());
    ASSERT(c2->g == 128);

    // value_or
    slim_optional<Color> empty;
    Color fallback{1, 1, 1, 1};
    ASSERT(empty.value_or(fallback) == fallback);
}

TEST(sentinel_traits_user_type_rejects_sentinel) {
    // Constructing with the sentinel value should throw
    try {
        slim_optional<Color> c{Color{0, 0, 0, 0}};
        ASSERT(false); // should not reach
    } catch (const std_proposal::bad_optional_access&) {
        // expected
    }
}

TEST(sentinel_traits_user_type_sizeof) {
    // slim_optional<Color> should be same size as Color itself
    ASSERT(sizeof(slim_optional<Color>) == sizeof(Color));
    ASSERT(sizeof(std::optional<Color>) > sizeof(Color));
}

TEST(sentinel_traits_concept_check) {
    // Types with sentinel_traits should satisfy the concept
    static_assert(std_proposal::has_sentinel_traits<int>);
    static_assert(std_proposal::has_sentinel_traits<float>);
    static_assert(std_proposal::has_sentinel_traits<double>);
    static_assert(std_proposal::has_sentinel_traits<int*>);
    static_assert(std_proposal::has_sentinel_traits<std::string_view>);
    static_assert(std_proposal::has_sentinel_traits<Color>);

    // Types without sentinel_traits should not
    struct Unregistered { int x; };
    static_assert(!std_proposal::has_sentinel_traits<Unregistered>);
    static_assert(!std_proposal::has_sentinel_traits<std::string>);
}

TEST(sentinel_traits_nullopt_construction) {
    slim_optional<Color> c1(std_proposal::nullopt);
    ASSERT(!c1.has_value());

    slim_optional<Color> c2(std::nullopt);
    ASSERT(!c2.has_value());
}

TEST(sentinel_traits_copy_move) {
    slim_optional<Color> c1{Color{10, 20, 30, 40}};
    slim_optional<Color> c2 = c1;  // copy
    ASSERT(c2.has_value());
    ASSERT(c2->r == 10);

    slim_optional<Color> c3 = std::move(c1);  // move
    ASSERT(c3.has_value());
    ASSERT(c3->r == 10);

    slim_optional<Color> empty;
    slim_optional<Color> c4 = empty;  // copy empty
    ASSERT(!c4.has_value());
}

TEST(sentinel_traits_std_optional_interop) {
    // Convert to std::optional
    slim_optional<Color> c{Color{1, 2, 3, 4}};
    std::optional<Color> stdopt = c;
    ASSERT(stdopt.has_value());
    ASSERT(stdopt->r == 1);

    // Empty conversion
    slim_optional<Color> empty;
    std::optional<Color> stdopt2 = empty;
    ASSERT(!stdopt2.has_value());
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

    std::cout << "\nauto_sentinel scalar tests:\n";
    RUN_TEST(auto_sentinel_signed_int);
    RUN_TEST(auto_sentinel_signed_int_rejects_sentinel);
    RUN_TEST(auto_sentinel_unsigned_int);
    RUN_TEST(auto_sentinel_unsigned_rejects_sentinel);
    RUN_TEST(auto_sentinel_int8);
    RUN_TEST(auto_sentinel_int16);
    RUN_TEST(auto_sentinel_int64);
    RUN_TEST(auto_sentinel_uint64);
    RUN_TEST(auto_sentinel_size_t);

    std::cout << "\nauto_sentinel pointer tests:\n";
    RUN_TEST(auto_sentinel_pointer);
    RUN_TEST(auto_sentinel_pointer_rejects_nullptr);
    RUN_TEST(auto_sentinel_const_pointer);

    std::cout << "\nauto_sentinel floating point tests:\n";
    RUN_TEST(auto_sentinel_float);
    RUN_TEST(auto_sentinel_double);
    RUN_TEST(auto_sentinel_float_rejects_nan);
    RUN_TEST(auto_sentinel_double_rejects_nan);
    RUN_TEST(auto_sentinel_long_double);
    RUN_TEST(auto_sentinel_long_double_rejects_nan);

    std::cout << "\nauto_sentinel char16/32 tests:\n";
    RUN_TEST(auto_sentinel_char16);
    RUN_TEST(auto_sentinel_char32);

    std::cout << "\nauto_sentinel standard library type tests:\n";
    RUN_TEST(auto_sentinel_unique_ptr);
    RUN_TEST(auto_sentinel_shared_ptr);
    RUN_TEST(auto_sentinel_string_view);
    RUN_TEST(auto_sentinel_span);
    RUN_TEST(auto_sentinel_function);

    std::cout << "\nauto_sentinel sizeof tests:\n";
    RUN_TEST(auto_sentinel_sizeof_comparisons);

    std::cout << "\nauto_sentinel constexpr tests:\n";
    RUN_TEST(auto_sentinel_constexpr);

    std::cout << "\nauto_sentinel interop tests:\n";
    RUN_TEST(auto_sentinel_interop_with_explicit);

    std::cout << "\nsentinel_traits customization tests:\n";
    RUN_TEST(sentinel_traits_user_type_basic);
    RUN_TEST(sentinel_traits_user_type_rejects_sentinel);
    RUN_TEST(sentinel_traits_user_type_sizeof);
    RUN_TEST(sentinel_traits_concept_check);
    RUN_TEST(sentinel_traits_nullopt_construction);
    RUN_TEST(sentinel_traits_copy_move);
    RUN_TEST(sentinel_traits_std_optional_interop);

    std::cout << "\n======================================\n";
    std::cout << "All tests passed successfully!\n";
    return 0;
}
