#include <slim/optional.hpp>
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

using namespace slim;

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
// User types with sentinel_traits for testing
// ============================================================================

enum class Status : int32_t {
    INVALID = -1,
    OK = 0,
    ERROR = 1,
    WARNING = 2
};

namespace slim {
template<>
struct sentinel_traits<Status> {
    static constexpr Status sentinel() noexcept { return Status::INVALID; }
    static constexpr bool is_sentinel(const Status& v) noexcept { return v == Status::INVALID; }
};
}

struct Point {
    int x, y;

    constexpr Point(int x_, int y_) : x(x_), y(y_) {}

    constexpr bool operator==(const Point&) const = default;
    constexpr auto operator<=>(const Point&) const = default;
};

namespace slim {
template<>
struct sentinel_traits<Point> {
    static constexpr Point sentinel() noexcept { return {-9999, -9999}; }
    static constexpr bool is_sentinel(const Point& v) noexcept { return v.x == -9999 && v.y == -9999; }
};
}

// A non-structural user type — cannot be an NTTP, works via sentinel_traits.
struct Color {
    uint8_t r, g, b, a;
    constexpr bool operator==(const Color&) const = default;
};

namespace slim {
template<>
struct sentinel_traits<Color> {
    static constexpr Color sentinel() noexcept { return {0, 0, 0, 0}; }
    static constexpr bool is_sentinel(const Color& v) noexcept {
        return v.r == 0 && v.g == 0 && v.b == 0 && v.a == 0;
    }
};
}

// ============================================================================
// optional basic tests (using int* — sentinel is nullptr via sentinel_traits)
// ============================================================================

TEST(sentinel_default_construction) {
    optional<int*> opt;
    ASSERT(!opt.has_value());
    ASSERT(!opt);
}

TEST(sentinel_nullopt_construction) {
    optional<int*> opt(nullopt);
    ASSERT(!opt.has_value());
}

TEST(sentinel_std_nullopt_construction) {
    optional<int*> opt(std::nullopt);
    ASSERT(!opt.has_value());
}

TEST(sentinel_value_construction) {
    int x = 42;
    optional<int*> opt(&x);
    ASSERT(opt.has_value());
    ASSERT(**opt == 42);
}

TEST(sentinel_rejects_sentinel_value) {
    ASSERT_THROWS((optional<int*>(nullptr)), bad_optional_access);
}

TEST(sentinel_copy_construction) {
    int x = 42;
    optional<int*> opt1(&x);
    optional<int*> opt2(opt1);
    ASSERT(opt2.has_value());
    ASSERT(*opt1 == *opt2);
}

TEST(sentinel_move_construction) {
    int x = 42;
    optional<int*> opt1(&x);
    optional<int*> opt2(std::move(opt1));
    ASSERT(opt2.has_value());
    ASSERT(**opt2 == 42);
}

TEST(sentinel_in_place_construction) {
    int x = 42;
    optional<int*> opt(in_place, &x);
    ASSERT(opt.has_value());
    ASSERT(**opt == 42);
}

TEST(sentinel_std_in_place_construction) {
    int x = 42;
    optional<int*> opt(std::in_place, &x);
    ASSERT(opt.has_value());
    ASSERT(**opt == 42);
}

TEST(sentinel_assignment) {
    int x = 42;
    optional<int*> opt;
    opt = &x;
    ASSERT(opt.has_value());
    ASSERT(**opt == 42);

    opt = nullopt;
    ASSERT(!opt.has_value());
}

TEST(sentinel_std_nullopt_assignment) {
    int x = 42;
    optional<int*> opt(&x);
    opt = std::nullopt;
    ASSERT(!opt.has_value());
}

TEST(sentinel_assignment_rejects_sentinel) {
    optional<int*> opt;
    ASSERT_THROWS(opt = nullptr, bad_optional_access);
}

TEST(sentinel_copy_assignment) {
    int x = 42;
    optional<int*> opt1(&x);
    optional<int*> opt2;
    opt2 = opt1;
    ASSERT(opt2.has_value());
    ASSERT(*opt1 == *opt2);
}

TEST(sentinel_move_assignment) {
    int x = 42;
    optional<int*> opt1(&x);
    optional<int*> opt2;
    opt2 = std::move(opt1);
    ASSERT(opt2.has_value());
    ASSERT(**opt2 == 42);
}

TEST(sentinel_value) {
    int x = 42;
    optional<int*> opt(&x);
    ASSERT(opt.value() == &x);

    optional<int*> empty;
    ASSERT_THROWS(empty.value(), bad_optional_access);
}

TEST(sentinel_value_or) {
    int x = 42;
    int y = 99;
    optional<int*> opt(&x);
    ASSERT(opt.value_or(&y) == &x);

    optional<int*> empty;
    ASSERT(empty.value_or(&y) == &y);
}

TEST(sentinel_reset) {
    int x = 42;
    optional<int*> opt(&x);
    ASSERT(opt.has_value());
    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(sentinel_emplace) {
    int x = 42;
    optional<int*> opt;
    opt.emplace(&x);
    ASSERT(opt.has_value());
    ASSERT(**opt == 42);
}

TEST(sentinel_emplace_rejects_sentinel) {
    optional<int*> opt;
    ASSERT_THROWS(opt.emplace(nullptr), bad_optional_access);
}

TEST(sentinel_swap) {
    int x = 42;
    int y = 99;
    optional<int*> opt1(&x);
    optional<int*> opt2(&y);

    opt1.swap(opt2);
    ASSERT(**opt1 == 99);
    ASSERT(**opt2 == 42);

    optional<int*> opt3;
    opt1.swap(opt3);
    ASSERT(!opt1.has_value());
    ASSERT(opt3.has_value());
    ASSERT(**opt3 == 99);
}

TEST(sentinel_comparison) {
    int x = 42;
    int y = 99;
    optional<int*> opt1(&x);
    optional<int*> opt2(&x);
    optional<int*> opt3(&y);
    optional<int*> empty;

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
    optional<int*> opt(&x);
    auto result = opt.transform([](int* p) { return *p * 2; });
    ASSERT(result.has_value());
    ASSERT(*result == 84);

    optional<int*> empty;
    auto result2 = empty.transform([](int* p) { return *p * 2; });
    ASSERT(!result2.has_value());
}

TEST(sentinel_monadic_and_then) {
    int x = 42;
    optional<int*> opt(&x);
    auto result = opt.and_then([](int* p) -> optional<int> {
        if (*p > 0) return *p * 2;
        return nullopt;
    });
    ASSERT(result.has_value());
    ASSERT(*result == 84);
}

TEST(sentinel_monadic_or_else) {
    optional<int*> empty;
    int y = 99;
    auto result = empty.or_else([&y]() -> optional<int*> {
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
    optional<int*> slim(std_opt);
    ASSERT(slim.has_value());
    ASSERT(**slim == 42);

    std::optional<int*> empty_std;
    optional<int*> slim2(empty_std);
    ASSERT(!slim2.has_value());
}

TEST(interop_assign_from_std_optional) {
    int x = 42;
    std::optional<int*> std_opt(&x);
    optional<int*> slim;
    slim = std_opt;
    ASSERT(slim.has_value());
    ASSERT(**slim == 42);

    std::optional<int*> empty_std;
    slim = empty_std;
    ASSERT(!slim.has_value());
}

TEST(interop_convert_to_std_optional) {
    int x = 42;
    optional<int*> slim(&x);
    std::optional<int*> std_opt = slim;
    ASSERT(std_opt.has_value());
    ASSERT(**std_opt == 42);

    optional<int*> empty;
    std::optional<int*> empty_std = empty;
    ASSERT(!empty_std.has_value());
}

TEST(interop_compare_with_std_optional) {
    int x = 42;
    optional<int*> slim(&x);
    std::optional<int*> std_opt(&x);

    ASSERT(slim == std_opt);
    ASSERT(std_opt == slim);

    slim.reset();
    ASSERT(slim != std_opt);

    std_opt.reset();
    ASSERT(slim == std_opt);  // Both empty
}

// ============================================================================
// Enum sentinel tests (via sentinel_traits<Status>)
// ============================================================================

TEST(enum_sentinel_basic) {
    optional<Status> opt;
    ASSERT(!opt.has_value());

    opt = Status::OK;
    ASSERT(opt.has_value());
    ASSERT(*opt == Status::OK);

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(enum_sentinel_rejects_sentinel) {
    ASSERT_THROWS((optional<Status>(Status::INVALID)),
                  bad_optional_access);
}

TEST(enum_sentinel_sizeof) {
    ASSERT(sizeof(optional<Status>) == sizeof(Status));
    ASSERT(sizeof(std::optional<Status>) > sizeof(Status));
}

// ============================================================================
// Constexpr tests
// ============================================================================

constexpr bool constexpr_sentinel_test() {
    optional<int> opt;
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
    ASSERT(sizeof(optional<int*>) == sizeof(int*));
    ASSERT(sizeof(std::optional<int*>) > sizeof(int*));

    // 32-bit integers
    ASSERT(sizeof(optional<int32_t>) == sizeof(int32_t));
    ASSERT(sizeof(std::optional<int32_t>) > sizeof(int32_t));

    // 64-bit integers
    ASSERT(sizeof(optional<int64_t>) == sizeof(int64_t));
    ASSERT(sizeof(std::optional<int64_t>) > sizeof(int64_t));

    std::cout << "  sizeof(std::optional<int*>): " << sizeof(std::optional<int*>) << "\n";
    std::cout << "  sizeof(optional<int*>): " << sizeof(optional<int*>) << "\n";
    std::cout << "  sizeof(std::optional<int32_t>): " << sizeof(std::optional<int32_t>) << "\n";
    std::cout << "  sizeof(optional<int32_t>): " << sizeof(optional<int32_t>) << "\n";
}

// ============================================================================
// Custom type tests (via sentinel_traits<Point>)
// ============================================================================

TEST(custom_type_sentinel) {
    optional<Point> opt;
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
    ASSERT_THROWS((optional<Point>(Point{-9999, -9999})),
                  bad_optional_access);
}

// ============================================================================
// Hash tests
// ============================================================================

TEST(hash_support) {
    int x = 42;
    optional<int*> opt1(&x);
    optional<int*> opt2(&x);
    optional<int*> empty;

    std::hash<optional<int*>> hasher;

    ASSERT(hasher(opt1) == hasher(opt2));
    ASSERT(hasher(empty) == 0);

    // Hash of optional should match hash of value
    ASSERT(hasher(opt1) == std::hash<int*>{}(&x));
}

// ============================================================================
// make_optional tests
// ============================================================================

TEST(make_optional_test) {
    int x = 42;
    auto opt = make_optional(&x);
    ASSERT(opt.has_value());
    ASSERT(**opt == 42);
}

// ============================================================================
// Container tests
// ============================================================================

TEST(vector_of_optionals) {
    int x = 1, y = 2, z = 3;

    std::vector<optional<int*>> vec;
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
// Scalar type tests
// ============================================================================

TEST(signed_int) {
    optional<int> opt;
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

TEST(signed_int_rejects_sentinel) {
    // INT_MIN is the sentinel for signed int
    ASSERT_THROWS((optional<int>(std::numeric_limits<int>::min())), bad_optional_access);
}

TEST(unsigned_int) {
    optional<unsigned> opt;
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

TEST(unsigned_rejects_sentinel) {
    ASSERT_THROWS((optional<unsigned>(std::numeric_limits<unsigned>::max())), bad_optional_access);
}

TEST(int8) {
    optional<int8_t> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(int8_t));

    opt = int8_t{42};
    ASSERT(opt.has_value());
    ASSERT(*opt == 42);
}

TEST(int16) {
    optional<int16_t> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(int16_t));

    opt = int16_t{1000};
    ASSERT(opt.has_value());
    ASSERT(*opt == 1000);
}

TEST(int64) {
    optional<int64_t> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(int64_t));

    opt = int64_t{123456789LL};
    ASSERT(opt.has_value());
    ASSERT(*opt == 123456789LL);
}

TEST(uint64) {
    optional<uint64_t> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(uint64_t));

    opt = uint64_t{42};
    ASSERT(opt.has_value());
    ASSERT(*opt == 42u);
}

TEST(size_t_type) {
    optional<std::size_t> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(std::size_t));

    opt = std::size_t{100};
    ASSERT(opt.has_value());
    ASSERT(*opt == 100u);
}

// ============================================================================
// Pointer type tests
// ============================================================================

TEST(pointer) {
    optional<int*> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(int*));

    int x = 42;
    opt = &x;
    ASSERT(opt.has_value());
    ASSERT(**opt == 42);

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(pointer_rejects_nullptr) {
    ASSERT_THROWS((optional<int*>(nullptr)), bad_optional_access);
}

TEST(const_pointer) {
    optional<const char*> opt;
    ASSERT(!opt.has_value());

    opt = "hello";
    ASSERT(opt.has_value());

    opt.reset();
    ASSERT(!opt.has_value());
}

// ============================================================================
// Floating point tests
// ============================================================================

TEST(float_type) {
    optional<float> opt;
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

TEST(double_type) {
    optional<double> opt;
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

TEST(float_rejects_nan) {
    ASSERT_THROWS((optional<float>(std::numeric_limits<float>::quiet_NaN())), bad_optional_access);
}

TEST(double_rejects_nan) {
    ASSERT_THROWS((optional<double>(std::numeric_limits<double>::quiet_NaN())), bad_optional_access);
}

TEST(long_double_type) {
    optional<long double> opt;
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

TEST(long_double_rejects_nan) {
    ASSERT_THROWS((optional<long double>(std::numeric_limits<long double>::quiet_NaN())), bad_optional_access);
}

// ============================================================================
// char16/32 tests
// ============================================================================

TEST(char16) {
    optional<char16_t> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(char16_t));

    opt = u'A';
    ASSERT(opt.has_value());
    ASSERT(*opt == u'A');

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(char32) {
    optional<char32_t> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(char32_t));

    opt = U'\x1F600'; // emoji codepoint
    ASSERT(opt.has_value());

    opt.reset();
    ASSERT(!opt.has_value());
}

// ============================================================================
// Standard library class type tests
// ============================================================================

TEST(unique_ptr_type) {
    optional<std::unique_ptr<int>> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(std::unique_ptr<int>));

    opt = std::make_unique<int>(42);
    ASSERT(opt.has_value());
    ASSERT(**opt == 42);

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(shared_ptr_type) {
    optional<std::shared_ptr<int>> opt;
    ASSERT(!opt.has_value());

    opt = std::make_shared<int>(99);
    ASSERT(opt.has_value());
    ASSERT(**opt == 99);

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(string_view_type) {
    optional<std::string_view> opt;
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

TEST(span_type) {
    optional<std::span<int>> opt;
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

TEST(function_type) {
    optional<std::function<int()>> opt;
    ASSERT(!opt.has_value());

    opt = std::function<int()>{[]{ return 42; }};
    ASSERT(opt.has_value());
    ASSERT((*opt)() == 42);

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(move_only_function_type) {
    optional<std::move_only_function<int()>> opt;
    ASSERT(!opt.has_value());

    opt = std::move_only_function<int()>{[]{ return 99; }};
    ASSERT(opt.has_value());
    ASSERT((*opt)() == 99);

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(weak_ptr_type) {
    optional<std::weak_ptr<int>> opt;
    ASSERT(!opt.has_value());

    auto sp = std::make_shared<int>(42);
    opt = std::weak_ptr<int>{sp};
    ASSERT(opt.has_value());
    ASSERT(*opt->lock() == 42);

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(weak_ptr_expired_vs_empty) {
    // A weak_ptr whose shared_ptr has died is expired but was assigned —
    // the sentinel is a default-constructed weak_ptr (use_count == 0 && expired),
    // not merely an expired one. However, once the shared_ptr dies the weak_ptr
    // becomes indistinguishable from default-constructed in terms of observable state.
    // This is an inherent limitation: weak_ptr has no "never assigned" flag.
    optional<std::weak_ptr<int>> opt;
    {
        auto sp = std::make_shared<int>(42);
        opt = std::weak_ptr<int>{sp};
        ASSERT(opt.has_value());
    }
    // sp is gone — weak_ptr is now expired, indistinguishable from sentinel
    ASSERT(!opt.has_value());
}

TEST(coroutine_handle_type) {
    optional<std::coroutine_handle<>> opt;
    ASSERT(!opt.has_value());
    ASSERT(sizeof(opt) == sizeof(std::coroutine_handle<>));

    opt.reset();
    ASSERT(!opt.has_value());
}

TEST(any_type) {
    optional<std::any> opt;
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
// sizeof tests
// ============================================================================

TEST(sizeof_all) {
    // All types should be same size as the underlying type
    ASSERT(sizeof(optional<int>) == sizeof(int));
    ASSERT(sizeof(optional<unsigned>) == sizeof(unsigned));
    ASSERT(sizeof(optional<int64_t>) == sizeof(int64_t));
    ASSERT(sizeof(optional<float>) == sizeof(float));
    ASSERT(sizeof(optional<double>) == sizeof(double));
    ASSERT(sizeof(optional<int*>) == sizeof(int*));
    ASSERT(sizeof(optional<std::string_view>) == sizeof(std::string_view));
    ASSERT(sizeof(optional<std::unique_ptr<int>>) == sizeof(std::unique_ptr<int>));

    // All should be smaller than std::optional
    ASSERT(sizeof(optional<int>) < sizeof(std::optional<int>));
    ASSERT(sizeof(optional<double>) < sizeof(std::optional<double>));
    ASSERT(sizeof(optional<int*>) < sizeof(std::optional<int*>));
    ASSERT(sizeof(optional<std::string_view>) < sizeof(std::optional<std::string_view>));

    std::cout << "  sizeof(optional<int>): " << sizeof(optional<int>) << "\n";
    std::cout << "  sizeof(std::optional<int>): " << sizeof(std::optional<int>) << "\n";
    std::cout << "  sizeof(optional<double>): " << sizeof(optional<double>) << "\n";
    std::cout << "  sizeof(std::optional<double>): " << sizeof(std::optional<double>) << "\n";
    std::cout << "  sizeof(optional<int*>): " << sizeof(optional<int*>) << "\n";
    std::cout << "  sizeof(std::optional<int*>): " << sizeof(std::optional<int*>) << "\n";
    std::cout << "  sizeof(optional<string_view>): " << sizeof(optional<std::string_view>) << "\n";
    std::cout << "  sizeof(std::optional<string_view>): " << sizeof(std::optional<std::string_view>) << "\n";
    std::cout << "  sizeof(optional<unique_ptr<int>>): " << sizeof(optional<std::unique_ptr<int>>) << "\n";
    std::cout << "  sizeof(std::optional<unique_ptr<int>>): " << sizeof(std::optional<std::unique_ptr<int>>) << "\n";
}

// ============================================================================
// Constexpr tests
// ============================================================================

constexpr bool constexpr_all_test() {
    optional<int> i;
    if (i.has_value()) return false;
    i = 42;
    if (*i != 42) return false;
    i.reset();
    if (i.has_value()) return false;

    optional<double> d;
    if (d.has_value()) return false;
    d = 3.14;
    if (*d != 3.14) return false;

    optional<int*> p;
    if (p.has_value()) return false;

    return true;
}

TEST(constexpr_all) {
    static_assert(constexpr_all_test());
    ASSERT(constexpr_all_test());
}

// ============================================================================
// sentinel_traits customization point tests
// ============================================================================

TEST(sentinel_traits_user_type_basic) {
    optional<Color> c;
    ASSERT(!c.has_value());

    c = Color{255, 0, 0, 255};
    ASSERT(c.has_value());
    ASSERT(c->r == 255);
    ASSERT(c->a == 255);

    c.reset();
    ASSERT(!c.has_value());

    // Construct with value
    optional<Color> c2{Color{0, 128, 0, 255}};
    ASSERT(c2.has_value());
    ASSERT(c2->g == 128);

    // value_or
    optional<Color> empty;
    Color fallback{1, 1, 1, 1};
    ASSERT(empty.value_or(fallback) == fallback);
}

TEST(sentinel_traits_user_type_rejects_sentinel) {
    try {
        optional<Color> c{Color{0, 0, 0, 0}};
        ASSERT(false); // should not reach
    } catch (const slim::bad_optional_access&) {
        // expected
    }
}

TEST(sentinel_traits_user_type_sizeof) {
    ASSERT(sizeof(optional<Color>) == sizeof(Color));
    ASSERT(sizeof(std::optional<Color>) > sizeof(Color));
}

TEST(sentinel_traits_concept_check) {
    // Types with sentinel_traits should satisfy the concept
    static_assert(slim::has_sentinel_traits<int>);
    static_assert(slim::has_sentinel_traits<float>);
    static_assert(slim::has_sentinel_traits<double>);
    static_assert(slim::has_sentinel_traits<int*>);
    static_assert(slim::has_sentinel_traits<std::string_view>);
    static_assert(slim::has_sentinel_traits<Color>);
    static_assert(slim::has_sentinel_traits<Status>);
    static_assert(slim::has_sentinel_traits<Point>);

    // Types without sentinel_traits should not
    struct Unregistered { int x; };
    static_assert(!slim::has_sentinel_traits<Unregistered>);
    static_assert(!slim::has_sentinel_traits<std::string>);
}

TEST(sentinel_traits_nullopt_construction) {
    optional<Color> c1(slim::nullopt);
    ASSERT(!c1.has_value());

    optional<Color> c2(std::nullopt);
    ASSERT(!c2.has_value());
}

TEST(sentinel_traits_copy_move) {
    optional<Color> c1{Color{10, 20, 30, 40}};
    optional<Color> c2 = c1;  // copy
    ASSERT(c2.has_value());
    ASSERT(c2->r == 10);

    optional<Color> c3 = std::move(c1);  // move
    ASSERT(c3.has_value());
    ASSERT(c3->r == 10);

    optional<Color> empty;
    optional<Color> c4 = empty;  // copy empty
    ASSERT(!c4.has_value());
}

TEST(sentinel_traits_std_optional_interop) {
    // Convert to std::optional
    optional<Color> c{Color{1, 2, 3, 4}};
    std::optional<Color> stdopt = c;
    ASSERT(stdopt.has_value());
    ASSERT(stdopt->r == 1);

    // Empty conversion
    optional<Color> empty;
    std::optional<Color> stdopt2 = empty;
    ASSERT(!stdopt2.has_value());
}

// ============================================================================
// Main test runner
// ============================================================================

int main() {
    std::cout << "Running optional Tests\n";
    std::cout << "======================================\n\n";

    std::cout << "Basic tests (int*):\n";
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

    std::cout << "\nInterop with std::optional:\n";
    RUN_TEST(interop_construct_from_std_optional);
    RUN_TEST(interop_assign_from_std_optional);
    RUN_TEST(interop_convert_to_std_optional);
    RUN_TEST(interop_compare_with_std_optional);

    std::cout << "\nEnum sentinel (via sentinel_traits):\n";
    RUN_TEST(enum_sentinel_basic);
    RUN_TEST(enum_sentinel_rejects_sentinel);
    RUN_TEST(enum_sentinel_sizeof);

    std::cout << "\nConstexpr:\n";
    RUN_TEST(constexpr_sentinel);

    std::cout << "\nSize comparisons:\n";
    RUN_TEST(sizeof_comparisons);

    std::cout << "\nCustom type (via sentinel_traits):\n";
    RUN_TEST(custom_type_sentinel);
    RUN_TEST(custom_type_rejects_sentinel);

    std::cout << "\nHash:\n";
    RUN_TEST(hash_support);

    std::cout << "\nmake_optional:\n";
    RUN_TEST(make_optional_test);

    std::cout << "\nContainers:\n";
    RUN_TEST(vector_of_optionals);

    std::cout << "\nScalar types:\n";
    RUN_TEST(signed_int);
    RUN_TEST(signed_int_rejects_sentinel);
    RUN_TEST(unsigned_int);
    RUN_TEST(unsigned_rejects_sentinel);
    RUN_TEST(int8);
    RUN_TEST(int16);
    RUN_TEST(int64);
    RUN_TEST(uint64);
    RUN_TEST(size_t_type);

    std::cout << "\nPointer types:\n";
    RUN_TEST(pointer);
    RUN_TEST(pointer_rejects_nullptr);
    RUN_TEST(const_pointer);

    std::cout << "\nFloating point types:\n";
    RUN_TEST(float_type);
    RUN_TEST(double_type);
    RUN_TEST(float_rejects_nan);
    RUN_TEST(double_rejects_nan);
    RUN_TEST(long_double_type);
    RUN_TEST(long_double_rejects_nan);

    std::cout << "\nchar16/32 types:\n";
    RUN_TEST(char16);
    RUN_TEST(char32);

    std::cout << "\nStandard library types:\n";
    RUN_TEST(unique_ptr_type);
    RUN_TEST(shared_ptr_type);
    RUN_TEST(string_view_type);
    RUN_TEST(span_type);
    RUN_TEST(function_type);
    RUN_TEST(move_only_function_type);
    RUN_TEST(weak_ptr_type);
    RUN_TEST(weak_ptr_expired_vs_empty);
    RUN_TEST(coroutine_handle_type);
    RUN_TEST(any_type);

    std::cout << "\nsizeof all types:\n";
    RUN_TEST(sizeof_all);

    std::cout << "\nConstexpr all:\n";
    RUN_TEST(constexpr_all);

    std::cout << "\nsentinel_traits customization:\n";
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
