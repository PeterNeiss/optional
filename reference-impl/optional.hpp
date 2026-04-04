#pragma once

#include <concepts>
#include <exception>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace std_proposal {

// Exception type
class bad_optional_access : public std::exception {
    const char* msg_;
public:
    explicit bad_optional_access(const char* msg = "bad optional access")
        : msg_(msg) {}

    const char* what() const noexcept override {
        return msg_;
    }
};

// Nullopt type
struct nullopt_t {
    explicit constexpr nullopt_t(int) noexcept {}
};

inline constexpr nullopt_t nullopt{0};

// In-place construction tag
struct in_place_t {
    explicit in_place_t() = default;
};

inline constexpr in_place_t in_place{};

// Sentinel storage marker
struct use_bool_storage_t {
    explicit constexpr use_bool_storage_t() = default;

    // Make it comparable but always unequal to any T
    template<class T>
    friend constexpr bool operator==(const use_bool_storage_t&, const T&) noexcept {
        return false;
    }
};

inline constexpr use_bool_storage_t use_bool_storage{};

// Forward declaration
template<class T, auto SentinelValue = use_bool_storage>
class optional;

// ============================================================================
// Bool-based specialization (backwards compatible with std::optional)
// ============================================================================

template<class T>
class optional<T, use_bool_storage> {
    bool has_value_;
    union {
        char dummy_;
        T value_;
    };

public:
    using value_type = T;

    // Constructors
    constexpr optional() noexcept
        : has_value_(false), dummy_() {}

    constexpr optional(nullopt_t) noexcept
        : has_value_(false), dummy_() {}

    constexpr optional(const optional& other)
        requires std::is_copy_constructible_v<T>
    {
        has_value_ = other.has_value_;
        if (has_value_) {
            std::construct_at(std::addressof(value_), other.value_);
        }
    }

    constexpr optional(optional&& other)
        noexcept(std::is_nothrow_move_constructible_v<T>)
        requires std::is_move_constructible_v<T>
    {
        has_value_ = other.has_value_;
        if (has_value_) {
            std::construct_at(std::addressof(value_), std::move(other.value_));
        }
    }

    template<class... Args>
        requires std::is_constructible_v<T, Args...>
    constexpr explicit optional(in_place_t, Args&&... args)
        : has_value_(true), value_(std::forward<Args>(args)...) {}

    template<class U = T>
        requires (!std::same_as<std::remove_cvref_t<U>, optional> &&
                  !std::same_as<std::remove_cvref_t<U>, in_place_t> &&
                  !std::same_as<std::remove_cvref_t<U>, nullopt_t> &&
                  std::is_constructible_v<T, U>)
    constexpr explicit(!std::is_convertible_v<U, T>)
    optional(U&& value)
        : has_value_(true), value_(std::forward<U>(value)) {}

    template<class U, auto S>
        requires std::is_constructible_v<T, const U&>
    constexpr explicit(!std::is_convertible_v<const U&, T>)
    optional(const optional<U, S>& other) {
        has_value_ = other.has_value();
        if (has_value_) {
            std::construct_at(std::addressof(value_), *other);
        }
    }

    template<class U, auto S>
        requires std::is_constructible_v<T, U&&>
    constexpr explicit(!std::is_convertible_v<U&&, T>)
    optional(optional<U, S>&& other) {
        has_value_ = other.has_value();
        if (has_value_) {
            std::construct_at(std::addressof(value_), std::move(*other));
        }
    }

    // Destructor
    constexpr ~optional() {
        if (has_value_) {
            value_.~T();
        }
    }

    // Assignment
    constexpr optional& operator=(nullopt_t) noexcept {
        reset();
        return *this;
    }

    constexpr optional& operator=(const optional& other)
        requires std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>
    {
        if (this == &other) return *this;

        if (other.has_value_) {
            if (has_value_) {
                value_ = other.value_;
            } else {
                std::construct_at(std::addressof(value_), other.value_);
                has_value_ = true;
            }
        } else {
            reset();
        }
        return *this;
    }

    constexpr optional& operator=(optional&& other)
        noexcept(std::is_nothrow_move_constructible_v<T> &&
                 std::is_nothrow_move_assignable_v<T>)
        requires std::is_move_constructible_v<T> && std::is_move_assignable_v<T>
    {
        if (this == &other) return *this;

        if (other.has_value_) {
            if (has_value_) {
                value_ = std::move(other.value_);
            } else {
                std::construct_at(std::addressof(value_), std::move(other.value_));
                has_value_ = true;
            }
        } else {
            reset();
        }
        return *this;
    }

    template<class U = T>
        requires (!std::same_as<std::remove_cvref_t<U>, optional> &&
                  std::is_constructible_v<T, U> &&
                  std::is_assignable_v<T&, U>)
    constexpr optional& operator=(U&& value) {
        if (has_value_) {
            value_ = std::forward<U>(value);
        } else {
            std::construct_at(std::addressof(value_), std::forward<U>(value));
            has_value_ = true;
        }
        return *this;
    }

    template<class U, auto S>
        requires std::is_constructible_v<T, const U&> &&
                 std::is_assignable_v<T&, const U&>
    constexpr optional& operator=(const optional<U, S>& other) {
        if (other.has_value()) {
            if (has_value_) {
                value_ = *other;
            } else {
                std::construct_at(std::addressof(value_), *other);
                has_value_ = true;
            }
        } else {
            reset();
        }
        return *this;
    }

    template<class U, auto S>
        requires std::is_constructible_v<T, U> &&
                 std::is_assignable_v<T&, U>
    constexpr optional& operator=(optional<U, S>&& other) {
        if (other.has_value()) {
            if (has_value_) {
                value_ = std::move(*other);
            } else {
                std::construct_at(std::addressof(value_), std::move(*other));
                has_value_ = true;
            }
        } else {
            reset();
        }
        return *this;
    }

    // Observers
    constexpr bool has_value() const noexcept {
        return has_value_;
    }

    constexpr explicit operator bool() const noexcept {
        return has_value_;
    }

    constexpr const T& value() const & {
        if (!has_value_) {
            throw bad_optional_access("optional has no value");
        }
        return value_;
    }

    constexpr T& value() & {
        if (!has_value_) {
            throw bad_optional_access("optional has no value");
        }
        return value_;
    }

    constexpr const T&& value() const && {
        if (!has_value_) {
            throw bad_optional_access("optional has no value");
        }
        return std::move(value_);
    }

    constexpr T&& value() && {
        if (!has_value_) {
            throw bad_optional_access("optional has no value");
        }
        return std::move(value_);
    }

    constexpr const T* operator->() const noexcept {
        return std::addressof(value_);
    }

    constexpr T* operator->() noexcept {
        return std::addressof(value_);
    }

    constexpr const T& operator*() const & noexcept {
        return value_;
    }

    constexpr T& operator*() & noexcept {
        return value_;
    }

    constexpr const T&& operator*() const && noexcept {
        return std::move(value_);
    }

    constexpr T&& operator*() && noexcept {
        return std::move(value_);
    }

    template<class U>
        requires std::is_copy_constructible_v<T> &&
                 std::is_convertible_v<U, T>
    constexpr T value_or(U&& default_value) const & {
        return has_value_ ? value_ : static_cast<T>(std::forward<U>(default_value));
    }

    template<class U>
        requires std::is_move_constructible_v<T> &&
                 std::is_convertible_v<U, T>
    constexpr T value_or(U&& default_value) && {
        return has_value_ ? std::move(value_) : static_cast<T>(std::forward<U>(default_value));
    }

    // Modifiers
    constexpr void reset() noexcept {
        if (has_value_) {
            value_.~T();
            has_value_ = false;
        }
    }

    template<class... Args>
        requires std::is_constructible_v<T, Args...>
    constexpr T& emplace(Args&&... args) {
        reset();
        std::construct_at(std::addressof(value_), std::forward<Args>(args)...);
        has_value_ = true;
        return value_;
    }

    // Swap
    constexpr void swap(optional& other)
        noexcept(std::is_nothrow_move_constructible_v<T> &&
                 std::is_nothrow_swappable_v<T>)
    {
        if (has_value_ && other.has_value_) {
            using std::swap;
            swap(value_, other.value_);
        } else if (has_value_) {
            std::construct_at(std::addressof(other.value_), std::move(value_));
            value_.~T();
            other.has_value_ = true;
            has_value_ = false;
        } else if (other.has_value_) {
            std::construct_at(std::addressof(value_), std::move(other.value_));
            other.value_.~T();
            has_value_ = true;
            other.has_value_ = false;
        }
    }

    // Monadic operations (C++23)
    template<class F>
    constexpr auto and_then(F&& f) & {
        using U = std::remove_cvref_t<std::invoke_result_t<F, T&>>;
        if (has_value()) {
            return std::invoke(std::forward<F>(f), value_);
        } else {
            return U{};
        }
    }

    template<class F>
    constexpr auto and_then(F&& f) const & {
        using U = std::remove_cvref_t<std::invoke_result_t<F, const T&>>;
        if (has_value()) {
            return std::invoke(std::forward<F>(f), value_);
        } else {
            return U{};
        }
    }

    template<class F>
    constexpr auto and_then(F&& f) && {
        using U = std::remove_cvref_t<std::invoke_result_t<F, T&&>>;
        if (has_value()) {
            return std::invoke(std::forward<F>(f), std::move(value_));
        } else {
            return U{};
        }
    }

    template<class F>
    constexpr auto and_then(F&& f) const && {
        using U = std::remove_cvref_t<std::invoke_result_t<F, const T&&>>;
        if (has_value()) {
            return std::invoke(std::forward<F>(f), std::move(value_));
        } else {
            return U{};
        }
    }

    template<class F>
    constexpr auto transform(F&& f) & {
        using U = std::remove_cvref_t<std::invoke_result_t<F, T&>>;
        if (has_value()) {
            return optional<U>{in_place, std::invoke(std::forward<F>(f), value_)};
        } else {
            return optional<U>{};
        }
    }

    template<class F>
    constexpr auto transform(F&& f) const & {
        using U = std::remove_cvref_t<std::invoke_result_t<F, const T&>>;
        if (has_value()) {
            return optional<U>{in_place, std::invoke(std::forward<F>(f), value_)};
        } else {
            return optional<U>{};
        }
    }

    template<class F>
    constexpr auto transform(F&& f) && {
        using U = std::remove_cvref_t<std::invoke_result_t<F, T&&>>;
        if (has_value()) {
            return optional<U>{in_place, std::invoke(std::forward<F>(f), std::move(value_))};
        } else {
            return optional<U>{};
        }
    }

    template<class F>
    constexpr auto transform(F&& f) const && {
        using U = std::remove_cvref_t<std::invoke_result_t<F, const T&&>>;
        if (has_value()) {
            return optional<U>{in_place, std::invoke(std::forward<F>(f), std::move(value_))};
        } else {
            return optional<U>{};
        }
    }

    template<class F>
    constexpr optional or_else(F&& f) const & {
        if (has_value()) {
            return *this;
        } else {
            return std::invoke(std::forward<F>(f));
        }
    }

    template<class F>
    constexpr optional or_else(F&& f) && {
        if (has_value()) {
            return std::move(*this);
        } else {
            return std::invoke(std::forward<F>(f));
        }
    }
};

// ============================================================================
// Sentinel-based specialization (optimized, no bool flag)
// ============================================================================

template<class T, auto SentinelValue>
    requires (!std::same_as<std::remove_cv_t<decltype(SentinelValue)>, use_bool_storage_t>) &&
             (std::same_as<std::remove_cv_t<decltype(SentinelValue)>, std::remove_cv_t<T>> ||
              std::convertible_to<decltype(SentinelValue), T>)
class optional<T, SentinelValue> {
    T value_;

    // Helper to validate not constructing with sentinel
    constexpr void validate_not_sentinel() const {
        if (value_ == SentinelValue) {
            if (std::is_constant_evaluated()) {
                // In constexpr context, throw causes compilation error
                throw bad_optional_access("Cannot construct optional with sentinel value");
            } else {
                throw bad_optional_access("Cannot construct optional with sentinel value");
            }
        }
    }

public:
    using value_type = T;

    // Constructors
    constexpr optional() noexcept(std::is_nothrow_copy_constructible_v<T>)
        : value_(SentinelValue) {}

    constexpr optional(nullopt_t) noexcept(std::is_nothrow_copy_constructible_v<T>)
        : value_(SentinelValue) {}

    constexpr optional(const optional& other) = default;
    constexpr optional(optional&& other) noexcept = default;

    template<class... Args>
        requires std::is_constructible_v<T, Args...>
    constexpr explicit optional(in_place_t, Args&&... args)
        : value_(std::forward<Args>(args)...)
    {
        validate_not_sentinel();
    }

    template<class U = T>
        requires (!std::same_as<std::remove_cvref_t<U>, optional> &&
                  !std::same_as<std::remove_cvref_t<U>, in_place_t> &&
                  !std::same_as<std::remove_cvref_t<U>, nullopt_t> &&
                  std::is_constructible_v<T, U>)
    constexpr explicit(!std::is_convertible_v<U, T>)
    optional(U&& value)
        : value_(std::forward<U>(value))
    {
        validate_not_sentinel();
    }

    template<class U, auto S>
        requires std::is_constructible_v<T, const U&>
    constexpr explicit(!std::is_convertible_v<const U&, T>)
    optional(const optional<U, S>& other)
        : value_(other.has_value() ? *other : SentinelValue)
    {
        if (has_value()) {
            validate_not_sentinel();
        }
    }

    template<class U, auto S>
        requires std::is_constructible_v<T, U&&>
    constexpr explicit(!std::is_convertible_v<U&&, T>)
    optional(optional<U, S>&& other)
        : value_(other.has_value() ? std::move(*other) : SentinelValue)
    {
        if (has_value()) {
            validate_not_sentinel();
        }
    }

    // Destructor (trivial for sentinel-based)
    constexpr ~optional() = default;

    // Assignment
    constexpr optional& operator=(nullopt_t) noexcept(std::is_nothrow_copy_assignable_v<T>) {
        value_ = SentinelValue;
        return *this;
    }

    constexpr optional& operator=(const optional& other) = default;
    constexpr optional& operator=(optional&& other) noexcept = default;

    template<class U = T>
        requires (!std::same_as<std::remove_cvref_t<U>, optional> &&
                  std::is_constructible_v<T, U> &&
                  std::is_assignable_v<T&, U>)
    constexpr optional& operator=(U&& value) {
        value_ = std::forward<U>(value);
        validate_not_sentinel();
        return *this;
    }

    template<class U, auto S>
        requires std::is_constructible_v<T, const U&> &&
                 std::is_assignable_v<T&, const U&>
    constexpr optional& operator=(const optional<U, S>& other) {
        if (other.has_value()) {
            value_ = *other;
            validate_not_sentinel();
        } else {
            value_ = SentinelValue;
        }
        return *this;
    }

    template<class U, auto S>
        requires std::is_constructible_v<T, U> &&
                 std::is_assignable_v<T&, U>
    constexpr optional& operator=(optional<U, S>&& other) {
        if (other.has_value()) {
            value_ = std::move(*other);
            validate_not_sentinel();
        } else {
            value_ = SentinelValue;
        }
        return *this;
    }

    // Observers
    constexpr bool has_value() const noexcept {
        return value_ != SentinelValue;
    }

    constexpr explicit operator bool() const noexcept {
        return has_value();
    }

    constexpr const T& value() const & {
        if (!has_value()) {
            throw bad_optional_access("optional has no value");
        }
        return value_;
    }

    constexpr T& value() & {
        if (!has_value()) {
            throw bad_optional_access("optional has no value");
        }
        return value_;
    }

    constexpr const T&& value() const && {
        if (!has_value()) {
            throw bad_optional_access("optional has no value");
        }
        return std::move(value_);
    }

    constexpr T&& value() && {
        if (!has_value()) {
            throw bad_optional_access("optional has no value");
        }
        return std::move(value_);
    }

    constexpr const T* operator->() const noexcept {
        return std::addressof(value_);
    }

    constexpr T* operator->() noexcept {
        return std::addressof(value_);
    }

    constexpr const T& operator*() const & noexcept {
        return value_;
    }

    constexpr T& operator*() & noexcept {
        return value_;
    }

    constexpr const T&& operator*() const && noexcept {
        return std::move(value_);
    }

    constexpr T&& operator*() && noexcept {
        return std::move(value_);
    }

    template<class U>
        requires std::is_copy_constructible_v<T> &&
                 std::is_convertible_v<U, T>
    constexpr T value_or(U&& default_value) const & {
        return has_value() ? value_ : static_cast<T>(std::forward<U>(default_value));
    }

    template<class U>
        requires std::is_move_constructible_v<T> &&
                 std::is_convertible_v<U, T>
    constexpr T value_or(U&& default_value) && {
        return has_value() ? std::move(value_) : static_cast<T>(std::forward<U>(default_value));
    }

    // Modifiers
    constexpr void reset() noexcept(std::is_nothrow_copy_assignable_v<T>) {
        value_ = SentinelValue;
    }

    template<class... Args>
        requires std::is_constructible_v<T, Args...>
    constexpr T& emplace(Args&&... args) {
        value_ = T(std::forward<Args>(args)...);
        validate_not_sentinel();
        return value_;
    }

    // Swap
    constexpr void swap(optional& other)
        noexcept(std::is_nothrow_move_constructible_v<T> &&
                 std::is_nothrow_swappable_v<T>)
    {
        using std::swap;
        swap(value_, other.value_);
    }

    // Monadic operations (C++23)
    template<class F>
    constexpr auto and_then(F&& f) & {
        using U = std::remove_cvref_t<std::invoke_result_t<F, T&>>;
        if (has_value()) {
            return std::invoke(std::forward<F>(f), value_);
        } else {
            return U{};
        }
    }

    template<class F>
    constexpr auto and_then(F&& f) const & {
        using U = std::remove_cvref_t<std::invoke_result_t<F, const T&>>;
        if (has_value()) {
            return std::invoke(std::forward<F>(f), value_);
        } else {
            return U{};
        }
    }

    template<class F>
    constexpr auto and_then(F&& f) && {
        using U = std::remove_cvref_t<std::invoke_result_t<F, T&&>>;
        if (has_value()) {
            return std::invoke(std::forward<F>(f), std::move(value_));
        } else {
            return U{};
        }
    }

    template<class F>
    constexpr auto and_then(F&& f) const && {
        using U = std::remove_cvref_t<std::invoke_result_t<F, const T&&>>;
        if (has_value()) {
            return std::invoke(std::forward<F>(f), std::move(value_));
        } else {
            return U{};
        }
    }

    template<class F>
    constexpr auto transform(F&& f) & {
        using U = std::remove_cvref_t<std::invoke_result_t<F, T&>>;
        if (has_value()) {
            return optional<U>{in_place, std::invoke(std::forward<F>(f), value_)};
        } else {
            return optional<U>{};
        }
    }

    template<class F>
    constexpr auto transform(F&& f) const & {
        using U = std::remove_cvref_t<std::invoke_result_t<F, const T&>>;
        if (has_value()) {
            return optional<U>{in_place, std::invoke(std::forward<F>(f), value_)};
        } else {
            return optional<U>{};
        }
    }

    template<class F>
    constexpr auto transform(F&& f) && {
        using U = std::remove_cvref_t<std::invoke_result_t<F, T&&>>;
        if (has_value()) {
            return optional<U>{in_place, std::invoke(std::forward<F>(f), std::move(value_))};
        } else {
            return optional<U>{};
        }
    }

    template<class F>
    constexpr auto transform(F&& f) const && {
        using U = std::remove_cvref_t<std::invoke_result_t<F, const T&&>>;
        if (has_value()) {
            return optional<U>{in_place, std::invoke(std::forward<F>(f), std::move(value_))};
        } else {
            return optional<U>{};
        }
    }

    template<class F>
    constexpr optional or_else(F&& f) const & {
        if (has_value()) {
            return *this;
        } else {
            return std::invoke(std::forward<F>(f));
        }
    }

    template<class F>
    constexpr optional or_else(F&& f) && {
        if (has_value()) {
            return std::move(*this);
        } else {
            return std::invoke(std::forward<F>(f));
        }
    }
};

// ============================================================================
// Comparison operators
// ============================================================================

template<class T, auto S1, auto S2>
constexpr bool operator==(const optional<T, S1>& lhs, const optional<T, S2>& rhs) {
    if (lhs.has_value() != rhs.has_value()) {
        return false;
    }
    if (!lhs.has_value()) {
        return true;
    }
    return *lhs == *rhs;
}

template<class T, auto S1, auto S2>
constexpr auto operator<=>(const optional<T, S1>& lhs, const optional<T, S2>& rhs)
    requires std::three_way_comparable<T>
{
    if (lhs.has_value() && rhs.has_value()) {
        return *lhs <=> *rhs;
    }
    return lhs.has_value() <=> rhs.has_value();
}

// Comparison with nullopt
template<class T, auto S>
constexpr bool operator==(const optional<T, S>& opt, nullopt_t) noexcept {
    return !opt.has_value();
}

template<class T, auto S>
constexpr std::strong_ordering operator<=>(const optional<T, S>& opt, nullopt_t) noexcept {
    return opt.has_value() <=> false;
}

// Comparison with T
template<class T, auto S, class U>
constexpr bool operator==(const optional<T, S>& opt, const U& value) {
    return opt.has_value() && (*opt == value);
}

template<class T, auto S, class U>
constexpr auto operator<=>(const optional<T, S>& opt, const U& value)
    requires std::three_way_comparable_with<T, U>
{
    return opt.has_value() ? *opt <=> value : std::strong_ordering::less;
}

// ============================================================================
// Specialized algorithms
// ============================================================================

template<class T, auto S>
constexpr void swap(optional<T, S>& lhs, optional<T, S>& rhs)
    noexcept(noexcept(lhs.swap(rhs)))
{
    lhs.swap(rhs);
}

template<class T, auto S = use_bool_storage>
constexpr optional<std::decay_t<T>, S> make_optional(T&& value) {
    return optional<std::decay_t<T>, S>(std::forward<T>(value));
}

template<class T, auto S = use_bool_storage, class... Args>
constexpr optional<T, S> make_optional(Args&&... args) {
    return optional<T, S>(in_place, std::forward<Args>(args)...);
}

// ============================================================================
// Hash support
// ============================================================================

} // namespace std_proposal

// Extend std::hash for our optional types
namespace std {

template<class T, auto S>
struct hash<std_proposal::optional<T, S>> {
    constexpr size_t operator()(const std_proposal::optional<T, S>& opt) const
        noexcept(noexcept(hash<T>{}(std::declval<T>())))
        requires requires { hash<T>{}(std::declval<T>()); }
    {
        if (!opt.has_value()) {
            return 0;
        }
        return hash<T>{}(*opt);
    }
};

} // namespace std
