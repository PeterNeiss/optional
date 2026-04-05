#pragma once

#include <concepts>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
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

// ============================================================================
// slim_optional: sentinel-based optional with no bool flag
// ============================================================================

template<class T, auto SentinelValue>
    requires (std::same_as<std::remove_cv_t<decltype(SentinelValue)>, std::remove_cv_t<T>> ||
              std::convertible_to<decltype(SentinelValue), T>)
class slim_optional {
    T value_;

    // Helper to validate not constructing with sentinel
    constexpr void validate_not_sentinel() const {
        if (value_ == SentinelValue) {
            if (std::is_constant_evaluated()) {
                throw bad_optional_access("Cannot construct slim_optional with sentinel value");
            } else {
                throw bad_optional_access("Cannot construct slim_optional with sentinel value");
            }
        }
    }

public:
    using value_type = T;

    // Constructors
    constexpr slim_optional() noexcept(std::is_nothrow_copy_constructible_v<T>)
        : value_(SentinelValue) {}

    constexpr slim_optional(nullopt_t) noexcept(std::is_nothrow_copy_constructible_v<T>)
        : value_(SentinelValue) {}

    constexpr slim_optional(std::nullopt_t) noexcept(std::is_nothrow_copy_constructible_v<T>)
        : value_(SentinelValue) {}

    constexpr slim_optional(const slim_optional& other) = default;
    constexpr slim_optional(slim_optional&& other) noexcept = default;

    template<class... Args>
        requires std::is_constructible_v<T, Args...>
    constexpr explicit slim_optional(in_place_t, Args&&... args)
        : value_(std::forward<Args>(args)...)
    {
        validate_not_sentinel();
    }

    template<class... Args>
        requires std::is_constructible_v<T, Args...>
    constexpr explicit slim_optional(std::in_place_t, Args&&... args)
        : value_(std::forward<Args>(args)...)
    {
        validate_not_sentinel();
    }

    template<class U = T>
        requires (!std::same_as<std::remove_cvref_t<U>, slim_optional> &&
                  !std::same_as<std::remove_cvref_t<U>, in_place_t> &&
                  !std::same_as<std::remove_cvref_t<U>, std::in_place_t> &&
                  !std::same_as<std::remove_cvref_t<U>, nullopt_t> &&
                  !std::same_as<std::remove_cvref_t<U>, std::nullopt_t> &&
                  std::is_constructible_v<T, U>)
    constexpr explicit(!std::is_convertible_v<U, T>)
    slim_optional(U&& value)
        : value_(std::forward<U>(value))
    {
        validate_not_sentinel();
    }

    // Construct from another slim_optional with different T/Sentinel
    template<class U, auto S>
        requires std::is_constructible_v<T, const U&>
    constexpr explicit(!std::is_convertible_v<const U&, T>)
    slim_optional(const slim_optional<U, S>& other)
        : value_(other.has_value() ? *other : SentinelValue)
    {
        if (has_value()) {
            validate_not_sentinel();
        }
    }

    template<class U, auto S>
        requires std::is_constructible_v<T, U&&>
    constexpr explicit(!std::is_convertible_v<U&&, T>)
    slim_optional(slim_optional<U, S>&& other)
        : value_(other.has_value() ? std::move(*other) : SentinelValue)
    {
        if (has_value()) {
            validate_not_sentinel();
        }
    }

    // Construct from std::optional
    template<class U = T>
        requires std::is_constructible_v<T, const U&>
    constexpr explicit(!std::is_convertible_v<const U&, T>)
    slim_optional(const std::optional<U>& other)
        : value_(other.has_value() ? *other : SentinelValue)
    {
        if (has_value()) {
            validate_not_sentinel();
        }
    }

    template<class U = T>
        requires std::is_constructible_v<T, U&&>
    constexpr explicit(!std::is_convertible_v<U&&, T>)
    slim_optional(std::optional<U>&& other)
        : value_(other.has_value() ? std::move(*other) : SentinelValue)
    {
        if (has_value()) {
            validate_not_sentinel();
        }
    }

    // Destructor (trivial for sentinel-based)
    constexpr ~slim_optional() = default;

    // Assignment
    constexpr slim_optional& operator=(nullopt_t) noexcept(std::is_nothrow_copy_assignable_v<T>) {
        value_ = SentinelValue;
        return *this;
    }

    constexpr slim_optional& operator=(std::nullopt_t) noexcept(std::is_nothrow_copy_assignable_v<T>) {
        value_ = SentinelValue;
        return *this;
    }

    constexpr slim_optional& operator=(const slim_optional& other) = default;
    constexpr slim_optional& operator=(slim_optional&& other) noexcept = default;

    template<class U = T>
        requires (!std::same_as<std::remove_cvref_t<U>, slim_optional> &&
                  std::is_constructible_v<T, U> &&
                  std::is_assignable_v<T&, U>)
    constexpr slim_optional& operator=(U&& value) {
        value_ = std::forward<U>(value);
        validate_not_sentinel();
        return *this;
    }

    template<class U, auto S>
        requires std::is_constructible_v<T, const U&> &&
                 std::is_assignable_v<T&, const U&>
    constexpr slim_optional& operator=(const slim_optional<U, S>& other) {
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
    constexpr slim_optional& operator=(slim_optional<U, S>&& other) {
        if (other.has_value()) {
            value_ = std::move(*other);
            validate_not_sentinel();
        } else {
            value_ = SentinelValue;
        }
        return *this;
    }

    // Assign from std::optional
    template<class U = T>
        requires std::is_constructible_v<T, const U&> &&
                 std::is_assignable_v<T&, const U&>
    constexpr slim_optional& operator=(const std::optional<U>& other) {
        if (other.has_value()) {
            value_ = *other;
            validate_not_sentinel();
        } else {
            value_ = SentinelValue;
        }
        return *this;
    }

    template<class U = T>
        requires std::is_constructible_v<T, U> &&
                 std::is_assignable_v<T&, U>
    constexpr slim_optional& operator=(std::optional<U>&& other) {
        if (other.has_value()) {
            value_ = std::move(*other);
            validate_not_sentinel();
        } else {
            value_ = SentinelValue;
        }
        return *this;
    }

    // Conversion to std::optional
    constexpr operator std::optional<T>() const {
        if (has_value()) {
            return std::optional<T>{value_};
        }
        return std::nullopt;
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
            throw bad_optional_access("slim_optional has no value");
        }
        return value_;
    }

    constexpr T& value() & {
        if (!has_value()) {
            throw bad_optional_access("slim_optional has no value");
        }
        return value_;
    }

    constexpr const T&& value() const && {
        if (!has_value()) {
            throw bad_optional_access("slim_optional has no value");
        }
        return std::move(value_);
    }

    constexpr T&& value() && {
        if (!has_value()) {
            throw bad_optional_access("slim_optional has no value");
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
    constexpr void swap(slim_optional& other)
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
            return std::optional<U>{std::invoke(std::forward<F>(f), value_)};
        } else {
            return std::optional<U>{};
        }
    }

    template<class F>
    constexpr auto transform(F&& f) const & {
        using U = std::remove_cvref_t<std::invoke_result_t<F, const T&>>;
        if (has_value()) {
            return std::optional<U>{std::invoke(std::forward<F>(f), value_)};
        } else {
            return std::optional<U>{};
        }
    }

    template<class F>
    constexpr auto transform(F&& f) && {
        using U = std::remove_cvref_t<std::invoke_result_t<F, T&&>>;
        if (has_value()) {
            return std::optional<U>{std::invoke(std::forward<F>(f), std::move(value_))};
        } else {
            return std::optional<U>{};
        }
    }

    template<class F>
    constexpr auto transform(F&& f) const && {
        using U = std::remove_cvref_t<std::invoke_result_t<F, const T&&>>;
        if (has_value()) {
            return std::optional<U>{std::invoke(std::forward<F>(f), std::move(value_))};
        } else {
            return std::optional<U>{};
        }
    }

    template<class F>
    constexpr slim_optional or_else(F&& f) const & {
        if (has_value()) {
            return *this;
        } else {
            return std::invoke(std::forward<F>(f));
        }
    }

    template<class F>
    constexpr slim_optional or_else(F&& f) && {
        if (has_value()) {
            return std::move(*this);
        } else {
            return std::invoke(std::forward<F>(f));
        }
    }
};

// ============================================================================
// Comparison operators — slim_optional vs slim_optional
// ============================================================================

template<class T, auto S1, auto S2>
constexpr bool operator==(const slim_optional<T, S1>& lhs, const slim_optional<T, S2>& rhs) {
    if (lhs.has_value() != rhs.has_value()) {
        return false;
    }
    if (!lhs.has_value()) {
        return true;
    }
    return *lhs == *rhs;
}

template<class T, auto S1, auto S2>
constexpr auto operator<=>(const slim_optional<T, S1>& lhs, const slim_optional<T, S2>& rhs)
    requires std::three_way_comparable<T>
{
    if (lhs.has_value() && rhs.has_value()) {
        return *lhs <=> *rhs;
    }
    return lhs.has_value() <=> rhs.has_value();
}

// ============================================================================
// Comparison operators — slim_optional vs std::optional
// ============================================================================

template<class T, auto S>
constexpr bool operator==(const slim_optional<T, S>& lhs, const std::optional<T>& rhs) {
    if (lhs.has_value() != rhs.has_value()) {
        return false;
    }
    if (!lhs.has_value()) {
        return true;
    }
    return *lhs == *rhs;
}

template<class T, auto S>
constexpr bool operator==(const std::optional<T>& lhs, const slim_optional<T, S>& rhs) {
    return rhs == lhs;
}

template<class T, auto S>
constexpr auto operator<=>(const slim_optional<T, S>& lhs, const std::optional<T>& rhs)
    requires std::three_way_comparable<T>
{
    if (lhs.has_value() && rhs.has_value()) {
        return *lhs <=> *rhs;
    }
    return lhs.has_value() <=> rhs.has_value();
}

// ============================================================================
// Comparison with nullopt
// ============================================================================

template<class T, auto S>
constexpr bool operator==(const slim_optional<T, S>& opt, nullopt_t) noexcept {
    return !opt.has_value();
}

template<class T, auto S>
constexpr bool operator==(const slim_optional<T, S>& opt, std::nullopt_t) noexcept {
    return !opt.has_value();
}

template<class T, auto S>
constexpr std::strong_ordering operator<=>(const slim_optional<T, S>& opt, nullopt_t) noexcept {
    return opt.has_value() <=> false;
}

template<class T, auto S>
constexpr std::strong_ordering operator<=>(const slim_optional<T, S>& opt, std::nullopt_t) noexcept {
    return opt.has_value() <=> false;
}

// ============================================================================
// Comparison with T
// ============================================================================

template<class T, auto S, class U>
constexpr bool operator==(const slim_optional<T, S>& opt, const U& value) {
    return opt.has_value() && (*opt == value);
}

template<class T, auto S, class U>
constexpr auto operator<=>(const slim_optional<T, S>& opt, const U& value)
    requires std::three_way_comparable_with<T, U>
{
    return opt.has_value() ? *opt <=> value : std::strong_ordering::less;
}

// ============================================================================
// Specialized algorithms
// ============================================================================

template<class T, auto S>
constexpr void swap(slim_optional<T, S>& lhs, slim_optional<T, S>& rhs)
    noexcept(noexcept(lhs.swap(rhs)))
{
    lhs.swap(rhs);
}

template<class T, auto S>
constexpr slim_optional<std::decay_t<T>, S> make_slim_optional(T&& value) {
    return slim_optional<std::decay_t<T>, S>(std::forward<T>(value));
}

template<class T, auto S, class... Args>
constexpr slim_optional<T, S> make_slim_optional(Args&&... args) {
    return slim_optional<T, S>(in_place, std::forward<Args>(args)...);
}

// ============================================================================
// Hash support
// ============================================================================

} // namespace std_proposal

// Extend std::hash for slim_optional
namespace std {

template<class T, auto S>
struct hash<std_proposal::slim_optional<T, S>> {
    constexpr size_t operator()(const std_proposal::slim_optional<T, S>& opt) const
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
