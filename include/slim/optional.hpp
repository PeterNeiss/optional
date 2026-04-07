// slim::optional — sentinel-based optional for C++23
// Copyright (c) 2026 Peter Neiss
// SPDX-License-Identifier: MIT

#pragma once

#include <compare>
#include <concepts>
#include <exception>
#include <limits>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

#ifndef SLIM_OPTIONAL_LEAN_AND_MEAN
#include <any>
#include <chrono>
#include <complex>
#include <coroutine>
#include <functional>
#include <span>
#include <stop_token>
#include <string_view>
#include <thread>
#endif

namespace slim {

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
// sentinel_traits<T>: customization point for sentinel values.
//
// Users specialize this for their own types to enable slim::optional support.
// Each specialization must provide:
//   static [constexpr] T sentinel() noexcept;
//   static [constexpr] bool is_sentinel(const T& v) noexcept;
// ============================================================================

// Primary template — undefined (opt-in via specialization)
template<class T>
struct sentinel_traits;

// Concept: is sentinel_traits<T> defined (i.e. a complete type)?
// The primary template is declared but never defined, so completeness
// tracks opt-in specializations without needing access to their (now
// protected) members. Declared below after all specializations so that
// users can add their own specializations and then refer to this concept.
template<class T>
concept has_sentinel_traits = requires { sizeof(sentinel_traits<T>); };

// Escape-hatch specialization: a "never-empty" traits. When used as the
// Traits parameter of slim::optional, is_sentinel is always false, so the
// optional always reports has_value()==true and collapses to sizeof(T).
template<typename T>
struct never_empty {
protected:
    static constexpr T sentinel() { throw "never_empty"; }
    static constexpr bool is_sentinel(const T&) noexcept { return false; }
};

// ── Scalar specializations ──
//
// Every built-in specialization makes sentinel()/is_sentinel() protected.
// They are accessed only by slim::optional<T, Traits>, which inherits
// publicly from Traits. Users adding their own sentinel_traits<T>
// specialization should do the same — nothing else is required.

// Signed integers (excluding bool)
template<class T>
    requires (std::signed_integral<T> && !std::same_as<T, bool>)
struct sentinel_traits<T>
{
protected:
    static constexpr T sentinel() noexcept { return std::numeric_limits<T>::min(); }
    static constexpr bool is_sentinel(const T& v) noexcept { return v == std::numeric_limits<T>::min(); }
};

// Unsigned integers (excluding bool, char8_t)
template<class T>
    requires (std::unsigned_integral<T> && !std::same_as<T, bool> && !std::same_as<T, char8_t>)
struct sentinel_traits<T>
{
protected:
    static constexpr T sentinel() noexcept { return std::numeric_limits<T>::max(); }
    static constexpr bool is_sentinel(const T& v) noexcept { return v == std::numeric_limits<T>::max(); }
};

// float/double/long double — NaN sentinel, all NaN values disallowed.
// v != v is true iff v is any NaN, and is constexpr everywhere.
template<>
struct sentinel_traits<float> {
protected:
    static constexpr float sentinel() noexcept { return std::numeric_limits<float>::quiet_NaN(); }
    static constexpr bool is_sentinel(const float& v) noexcept { return v != v; }
};

template<>
struct sentinel_traits<double> {
protected:
    static constexpr double sentinel() noexcept { return std::numeric_limits<double>::quiet_NaN(); }
    static constexpr bool is_sentinel(const double& v) noexcept { return v != v; }
};

template<>
struct sentinel_traits<long double> {
protected:
    static constexpr long double sentinel() noexcept { return std::numeric_limits<long double>::quiet_NaN(); }
    static constexpr bool is_sentinel(const long double& v) noexcept { return v != v; }
};

// Pointers → nullptr
template<class T>
struct sentinel_traits<T*> {
protected:
    static constexpr T* sentinel() noexcept { return nullptr; }
    static constexpr bool is_sentinel(T* const& v) noexcept { return v == nullptr; }
};

// char16_t → 0xFFFF (Unicode noncharacter)
template<>
struct sentinel_traits<char16_t> {
protected:
    static constexpr char16_t sentinel() noexcept { return 0xFFFF; }
    static constexpr bool is_sentinel(const char16_t& v) noexcept { return v == static_cast<char16_t>(0xFFFF); }
};

// char32_t → 0xFFFFFFFF (beyond Unicode range)
template<>
struct sentinel_traits<char32_t> {
protected:
    static constexpr char32_t sentinel() noexcept { return 0xFFFFFFFF; }
    static constexpr bool is_sentinel(const char32_t& v) noexcept { return v == static_cast<char32_t>(0xFFFFFFFF); }
};

// ── Standard library type specializations ──

#ifndef SLIM_OPTIONAL_LEAN_AND_MEAN

template<class T, class D>
struct sentinel_traits<std::unique_ptr<T, D>> {
protected:
    static std::unique_ptr<T, D> sentinel() noexcept { return nullptr; }
    static bool is_sentinel(const std::unique_ptr<T, D>& v) noexcept { return !v; }
};

template<class T>
struct sentinel_traits<std::shared_ptr<T>> {
protected:
    static std::shared_ptr<T> sentinel() noexcept { return nullptr; }
    static bool is_sentinel(const std::shared_ptr<T>& v) noexcept { return !v; }
};

template<class CharT, class Traits>
struct sentinel_traits<std::basic_string_view<CharT, Traits>> {
protected:
    static constexpr std::basic_string_view<CharT, Traits> sentinel() noexcept {
        return std::basic_string_view<CharT, Traits>{nullptr, 0};
    }
    static constexpr bool is_sentinel(const std::basic_string_view<CharT, Traits>& v) noexcept {
        return v.data() == nullptr;
    }
};

template<class T, std::size_t E>
struct sentinel_traits<std::span<T, E>> {
protected:
    static constexpr std::span<T, E> sentinel() noexcept { return std::span<T, E>{}; }
    static constexpr bool is_sentinel(const std::span<T, E>& v) noexcept { return v.data() == nullptr; }
};

template<class F>
struct sentinel_traits<std::function<F>> {
protected:
    static std::function<F> sentinel() noexcept { return nullptr; }
    static bool is_sentinel(const std::function<F>& v) noexcept { return !v; }
};

template<class F>
struct sentinel_traits<std::move_only_function<F>> {
protected:
    static std::move_only_function<F> sentinel() noexcept { return {}; }
    static bool is_sentinel(const std::move_only_function<F>& v) noexcept { return !v; }
};

template<class P>
struct sentinel_traits<std::coroutine_handle<P>> {
protected:
    static constexpr std::coroutine_handle<P> sentinel() noexcept { return std::coroutine_handle<P>{}; }
    static constexpr bool is_sentinel(const std::coroutine_handle<P>& v) noexcept { return !v; }
};

template<>
struct sentinel_traits<std::any> {
protected:
    static std::any sentinel() noexcept { return std::any{}; }
    static bool is_sentinel(const std::any& v) noexcept { return !v.has_value(); }
};

template<>
struct sentinel_traits<std::thread::id> {
protected:
    static constexpr std::thread::id sentinel() noexcept { return std::thread::id{}; }
    static constexpr bool is_sentinel(const std::thread::id& v) noexcept { return v == std::thread::id{}; }
};

template<>
struct sentinel_traits<std::stop_token> {
protected:
    static std::stop_token sentinel() noexcept { return std::stop_token{}; }
    static bool is_sentinel(const std::stop_token& v) noexcept { return !v.stop_possible(); }
};

// chrono::duration — uses the underlying Rep's sentinel (min() for integers, NaN for floats).
// Accesses the (protected) members of sentinel_traits<Rep> via inheritance.
template<class Rep, class Period>
    requires has_sentinel_traits<Rep>
struct sentinel_traits<std::chrono::duration<Rep, Period>> : private sentinel_traits<Rep> {
protected:
    static constexpr std::chrono::duration<Rep, Period> sentinel() noexcept {
        return std::chrono::duration<Rep, Period>{sentinel_traits<Rep>::sentinel()};
    }
    static constexpr bool is_sentinel(const std::chrono::duration<Rep, Period>& v) noexcept {
        return sentinel_traits<Rep>::is_sentinel(v.count());
    }
};

// chrono::time_point — uses the underlying Duration's sentinel
template<class Clock, class Duration>
    requires has_sentinel_traits<Duration>
struct sentinel_traits<std::chrono::time_point<Clock, Duration>> : private sentinel_traits<Duration> {
protected:
    static constexpr std::chrono::time_point<Clock, Duration> sentinel() noexcept {
        return std::chrono::time_point<Clock, Duration>{sentinel_traits<Duration>::sentinel()};
    }
    static constexpr bool is_sentinel(const std::chrono::time_point<Clock, Duration>& v) noexcept {
        return sentinel_traits<Duration>::is_sentinel(v.time_since_epoch());
    }
};

template<class T>
struct sentinel_traits<std::complex<T>> {
protected:
    static constexpr std::complex<T> sentinel() noexcept {
        return std::complex<T>{std::numeric_limits<T>::quiet_NaN(), std::numeric_limits<T>::quiet_NaN()};
    }
    static constexpr bool is_sentinel(const std::complex<T>& v) noexcept {
        return v.real() != v.real();
    }
};

#endif // !SLIM_OPTIONAL_LEAN_AND_MEAN

// exception_ptr — available in both modes (uses <exception> which is always included)
template<>
struct sentinel_traits<std::exception_ptr> {
protected:
    static constexpr std::exception_ptr sentinel() noexcept { return std::exception_ptr{}; }
    static constexpr bool is_sentinel(const std::exception_ptr& v) noexcept { return !v; }
};

// ── Types deliberately NOT supported ──
//
// std::string, std::u8string, etc.  — empty string is a valid value
// std::error_code                   — default (0) means "success", a valid value
// std::weak_ptr<T>                  — expired state is indistinguishable from sentinel
// std::variant<Ts...>               — cannot deliberately construct valueless state
// std::future<T>, std::shared_future<T> — move-only, rarely stored in containers
// std::filesystem::path             — empty path is a valid value
// std::regex                        — heavyweight, niche use case
// Containers (vector, map, etc.)    — empty is a valid state
// std::reference_wrapper<T>         — always holds a reference, no empty state
// Mutex/thread types                — not copyable or movable

// ============================================================================
// optional: sentinel-based optional with no bool flag
// ============================================================================

template<class T, class Traits = sentinel_traits<T>>
class optional;

// Helper trait to detect optional types (slim or std)
namespace detail {
template<class> inline constexpr bool is_optional_v = false;
template<class T, class Tr> inline constexpr bool is_optional_v<optional<T, Tr>> = true;
template<class T> inline constexpr bool is_optional_v<std::optional<T>> = true;
}

// Publicly inherits from Traits so any public members users attach to
// their sentinel_traits specialization (constants, typedefs, helpers) are
// reachable through the optional. Empty-base optimization keeps
// sizeof(optional<T>) == sizeof(T) whenever Traits has no data members.
template<class T, class Traits>
class optional : public Traits {
    T value_;

    static constexpr void validate_not_sentinel(const T& v) {
        if (Traits::is_sentinel(v)) {
            throw bad_optional_access("Cannot construct optional with sentinel value");
        }
    }

public:
    using value_type = T;
    using traits_type = Traits;

    // Default constructor — trivial whenever T is trivially default
    // constructible. In that case value_ is left in T's default-initialized
    // state (indeterminate for scalars, value-initialized for class types);
    // use `optional o = nullopt;` if you need a guaranteed-empty optional.
    constexpr optional() requires std::is_trivially_default_constructible_v<T> = default;

    // Fallback: sentinel-initializing default constructor for T that is not
    // trivially default constructible (only available under sentinel traits,
    // since the never-empty variant has no sentinel to fall back to).
    constexpr optional() noexcept(noexcept(T(Traits::sentinel())))
        requires (!std::is_trivially_default_constructible_v<T>)
        : value_(Traits::sentinel()) {}

    constexpr optional(nullopt_t) noexcept(noexcept(T(Traits::sentinel())))
      requires (!std::same_as<Traits, never_empty<T>>)
        : value_(Traits::sentinel()) {}

    constexpr optional(std::nullopt_t) noexcept(noexcept(T(Traits::sentinel())))
      requires (!std::same_as<Traits, never_empty<T>>)
        : value_(Traits::sentinel()) {}

    constexpr optional(const optional& other) = default;
    constexpr optional(optional&& other) noexcept(std::is_nothrow_move_constructible_v<T>) = default;

    template<class... Args>
        requires std::is_constructible_v<T, Args...>
    constexpr explicit optional(in_place_t, Args&&... args)
        : value_(std::forward<Args>(args)...)
    {
        validate_not_sentinel(value_);
    }

    template<class... Args>
        requires std::is_constructible_v<T, Args...>
    constexpr explicit optional(std::in_place_t, Args&&... args)
        : value_(std::forward<Args>(args)...)
    {
        validate_not_sentinel(value_);
    }

    template<class U = T>
        requires (!std::same_as<std::remove_cvref_t<U>, optional> &&
                  !std::same_as<std::remove_cvref_t<U>, in_place_t> &&
                  !std::same_as<std::remove_cvref_t<U>, std::in_place_t> &&
                  !std::same_as<std::remove_cvref_t<U>, nullopt_t> &&
                  !std::same_as<std::remove_cvref_t<U>, std::nullopt_t> &&
                  std::is_constructible_v<T, U>)
    constexpr explicit(!std::is_convertible_v<U, T>)
    optional(U&& value)
        : value_(std::forward<U>(value))
    {
        validate_not_sentinel(value_);
    }

    // Construct from another optional with different T
    template<class U>
        requires (std::is_constructible_v<T, const U&>)
    constexpr explicit(!std::is_convertible_v<const U&, T>)
    optional(const optional<U>& other)
        : value_(other.has_value() ? *other : Traits::sentinel())
    {
        if (has_value()) {
            validate_not_sentinel(value_);
        }
    }

    template<class U>
        requires (std::is_constructible_v<T, U&&>)
    constexpr explicit(!std::is_convertible_v<U&&, T>)
    optional(optional<U>&& other)
        : value_(other.has_value() ? std::move(*other) : Traits::sentinel())
    {
        if (has_value()) {
            validate_not_sentinel(value_);
        }
    }

    // Construct from std::optional
    template<class U = T>
        requires (std::is_constructible_v<T, const U&>)
    constexpr explicit(!std::is_convertible_v<const U&, T>)
    optional(const std::optional<U>& other)
        : value_(other.has_value() ? *other : Traits::sentinel())
    {
        if (has_value()) {
            validate_not_sentinel(value_);
        }
    }

    template<class U = T>
        requires (std::is_constructible_v<T, U&&>)
    constexpr explicit(!std::is_convertible_v<U&&, T>)
    optional(std::optional<U>&& other)
        : value_(other.has_value() ? std::move(*other) : Traits::sentinel())
    {
        if (has_value()) {
            validate_not_sentinel(value_);
        }
    }

    constexpr ~optional() = default;

    // Assignment
    constexpr optional& operator=(nullopt_t) noexcept(noexcept(std::declval<T&>() = Traits::sentinel()))
      requires (!std::same_as<Traits, never_empty<T>>)
    {
        value_ = Traits::sentinel();
        return *this;
    }

    constexpr optional& operator=(std::nullopt_t) noexcept(noexcept(std::declval<T&>() = Traits::sentinel()))
      requires (!std::same_as<Traits, never_empty<T>>)
    {
        value_ = Traits::sentinel();
        return *this;
    }

    constexpr optional& operator=(const optional& other) = default;
    constexpr optional& operator=(optional&& other) noexcept(std::is_nothrow_move_assignable_v<T>) = default;

    template<class U = T>
        requires (!std::same_as<std::remove_cvref_t<U>, optional> &&
                  std::is_constructible_v<T, U> &&
                  std::is_assignable_v<T&, U>)
    constexpr optional& operator=(U&& value) {
        value_ = std::forward<U>(value);
        validate_not_sentinel(value_);
        return *this;
    }

    template<class U>
        requires (std::is_constructible_v<T, const U&> &&
                  std::is_assignable_v<T&, const U&>)
    constexpr optional& operator=(const optional<U>& other) {
        if (other.has_value()) {
            value_ = *other;
            validate_not_sentinel(value_);
        } else {
            value_ = Traits::sentinel();
        }
        return *this;
    }

    template<class U>
        requires (std::is_constructible_v<T, U> &&
                  std::is_assignable_v<T&, U>)
    constexpr optional& operator=(optional<U>&& other) {
        if (other.has_value()) {
            value_ = std::move(*other);
            validate_not_sentinel(value_);
        } else {
            value_ = Traits::sentinel();
        }
        return *this;
    }

    // Assign from std::optional
    template<class U = T>
        requires (std::is_constructible_v<T, const U&> &&
                  std::is_assignable_v<T&, const U&>)
    constexpr optional& operator=(const std::optional<U>& other) {
        if (other.has_value()) {
            value_ = *other;
            validate_not_sentinel(value_);
        } else {
            value_ = Traits::sentinel();
        }
        return *this;
    }

    template<class U = T>
        requires (std::is_constructible_v<T, U> &&
                  std::is_assignable_v<T&, U>)
    constexpr optional& operator=(std::optional<U>&& other) {
        if (other.has_value()) {
            value_ = std::move(*other);
            validate_not_sentinel(value_);
        } else {
            value_ = Traits::sentinel();
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
        return !Traits::is_sentinel(value_);
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
    constexpr void reset() noexcept(noexcept(std::declval<T&>() = Traits::sentinel()))
    {
        value_ = Traits::sentinel();
    }

    template<class... Args>
        requires std::is_constructible_v<T, Args...>
    constexpr T& emplace(Args&&... args) {
        T tmp(std::forward<Args>(args)...);
        validate_not_sentinel(tmp);
        value_ = std::move(tmp);
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
        requires detail::is_optional_v<std::remove_cvref_t<std::invoke_result_t<F, T&>>>
    constexpr auto and_then(F&& f) & {
        using U = std::remove_cvref_t<std::invoke_result_t<F, T&>>;
        if (has_value()) {
            return std::forward<F>(f)(value_);
        } else {
            return U(std::nullopt);
        }
    }

    template<class F>
        requires detail::is_optional_v<std::remove_cvref_t<std::invoke_result_t<F, const T&>>>
    constexpr auto and_then(F&& f) const & {
        using U = std::remove_cvref_t<std::invoke_result_t<F, const T&>>;
        if (has_value()) {
            return std::forward<F>(f)(value_);
        } else {
            return U(std::nullopt);
        }
    }

    template<class F>
        requires detail::is_optional_v<std::remove_cvref_t<std::invoke_result_t<F, T&&>>>
    constexpr auto and_then(F&& f) && {
        using U = std::remove_cvref_t<std::invoke_result_t<F, T&&>>;
        if (has_value()) {
            return std::forward<F>(f)(std::move(value_));
        } else {
            return U(std::nullopt);
        }
    }

    template<class F>
        requires detail::is_optional_v<std::remove_cvref_t<std::invoke_result_t<F, const T&&>>>
    constexpr auto and_then(F&& f) const && {
        using U = std::remove_cvref_t<std::invoke_result_t<F, const T&&>>;
        if (has_value()) {
            return std::forward<F>(f)(std::move(value_));
        } else {
            return U(std::nullopt);
        }
    }

    template<class F>
    constexpr auto transform(F&& f) & {
        using U = std::remove_cvref_t<std::invoke_result_t<F, T&>>;
        if constexpr (has_sentinel_traits<U>) {
            if (has_value()) {
                return optional<U>{std::forward<F>(f)(value_)};
            } else {
                return optional<U>(nullopt);
            }
        } else {
            if (has_value()) {
                return std::optional<U>{std::forward<F>(f)(value_)};
            } else {
                return std::optional<U>(std::nullopt);
            }
        }
    }

    template<class F>
    constexpr auto transform(F&& f) const & {
        using U = std::remove_cvref_t<std::invoke_result_t<F, const T&>>;
        if constexpr (has_sentinel_traits<U>) {
            if (has_value()) {
                return optional<U>{std::forward<F>(f)(value_)};
            } else {
                return optional<U>(nullopt);
            }
        } else {
            if (has_value()) {
                return std::optional<U>{std::forward<F>(f)(value_)};
            } else {
                return std::optional<U>(std::nullopt);
            }
        }
    }

    template<class F>
    constexpr auto transform(F&& f) && {
        using U = std::remove_cvref_t<std::invoke_result_t<F, T&&>>;
        if constexpr (has_sentinel_traits<U>) {
            if (has_value()) {
                return optional<U>{std::forward<F>(f)(std::move(value_))};
            } else {
                return optional<U>(nullopt);
            }
        } else {
            if (has_value()) {
                return std::optional<U>{std::forward<F>(f)(std::move(value_))};
            } else {
                return std::optional<U>(std::nullopt);
            }
        }
    }

    template<class F>
    constexpr auto transform(F&& f) const && {
        using U = std::remove_cvref_t<std::invoke_result_t<F, const T&&>>;
        if constexpr (has_sentinel_traits<U>) {
            if (has_value()) {
                return optional<U>{std::forward<F>(f)(std::move(value_))};
            } else {
                return optional<U>(nullopt);
            }
        } else {
            if (has_value()) {
                return std::optional<U>{std::forward<F>(f)(std::move(value_))};
            } else {
                return std::optional<U>(std::nullopt);
            }
        }
    }

    template<class F>
    constexpr optional or_else(F&& f) const & {
        if (has_value()) {
            return *this;
        } else {
            return std::forward<F>(f)();
        }
    }

    template<class F>
    constexpr optional or_else(F&& f) && {
        if (has_value()) {
            return std::move(*this);
        } else {
            return std::forward<F>(f)();
        }
    }
};

// ============================================================================
// Comparison operators — optional vs optional
// ============================================================================

template<class T, class Tr>
constexpr bool operator==(const optional<T, Tr>& lhs, const optional<T, Tr>& rhs) {
    if (lhs.has_value() != rhs.has_value()) {
        return false;
    }
    if (!lhs.has_value()) {
        return true;
    }
    return *lhs == *rhs;
}

template<class T, class Tr>
constexpr std::compare_three_way_result_t<T> operator<=>(const optional<T, Tr>& lhs, const optional<T, Tr>& rhs)
    requires std::three_way_comparable<T>
{
    if (lhs.has_value() && rhs.has_value()) {
        return *lhs <=> *rhs;
    }
    return lhs.has_value() <=> rhs.has_value();
}

// ============================================================================
// Comparison operators — optional vs std::optional
// ============================================================================

template<class T, class Tr>
constexpr bool operator==(const optional<T, Tr>& lhs, const std::optional<T>& rhs) {
    if (lhs.has_value() != rhs.has_value()) {
        return false;
    }
    if (!lhs.has_value()) {
        return true;
    }
    return *lhs == *rhs;
}

template<class T, class Tr>
constexpr bool operator==(const std::optional<T>& lhs, const optional<T, Tr>& rhs) {
    return rhs == lhs;
}

template<class T, class Tr>
constexpr std::compare_three_way_result_t<T> operator<=>(const optional<T, Tr>& lhs, const std::optional<T>& rhs)
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

template<class T, class Tr>
constexpr bool operator==(const optional<T, Tr>& opt, nullopt_t) noexcept {
    return !opt.has_value();
}

template<class T, class Tr>
constexpr bool operator==(const optional<T, Tr>& opt, std::nullopt_t) noexcept {
    return !opt.has_value();
}

template<class T, class Tr>
constexpr std::strong_ordering operator<=>(const optional<T, Tr>& opt, nullopt_t) noexcept {
    return opt.has_value() <=> false;
}

template<class T, class Tr>
constexpr std::strong_ordering operator<=>(const optional<T, Tr>& opt, std::nullopt_t) noexcept {
    return opt.has_value() <=> false;
}

// ============================================================================
// Comparison with T
// ============================================================================

template<class T, class Tr, class U>
    requires (!detail::is_optional_v<std::remove_cvref_t<U>> &&
              !std::same_as<std::remove_cvref_t<U>, nullopt_t> &&
              !std::same_as<std::remove_cvref_t<U>, std::nullopt_t>)
constexpr bool operator==(const optional<T, Tr>& opt, const U& value) {
    return opt.has_value() && (*opt == value);
}

template<class T, class Tr, class U>
    requires (!detail::is_optional_v<std::remove_cvref_t<U>> &&
              !std::same_as<std::remove_cvref_t<U>, nullopt_t> &&
              !std::same_as<std::remove_cvref_t<U>, std::nullopt_t> &&
              std::three_way_comparable_with<T, U>)
constexpr std::compare_three_way_result_t<T, U> operator<=>(const optional<T, Tr>& opt, const U& value) {
    return opt.has_value() ? *opt <=> value : std::strong_ordering::less;
}

// ============================================================================
// Specialized algorithms
// ============================================================================

template<class T, class Tr>
constexpr void swap(optional<T, Tr>& lhs, optional<T, Tr>& rhs)
    noexcept(noexcept(lhs.swap(rhs)))
{
    lhs.swap(rhs);
}

template<class T>
constexpr optional<std::decay_t<T>> make_optional(T&& value) {
    return optional<std::decay_t<T>>(std::forward<T>(value));
}

template<class T, class... Args>
constexpr optional<T> make_optional(Args&&... args) {
    return optional<T>(in_place, std::forward<Args>(args)...);
}

// Deduction guide
template<class T>
optional(T) -> optional<T>;

// ============================================================================
// Hash support
// ============================================================================

} // namespace slim

// Extend std::hash for optional
namespace std {

template<class T, class Tr>
struct hash<slim::optional<T, Tr>> {
    constexpr size_t operator()(const slim::optional<T, Tr>& opt) const
        noexcept(noexcept(hash<T>{}(std::declval<T>())))
        requires requires { hash<T>{}(std::declval<T>()); }
    {
        if (!opt.has_value()) {
            return 0;
        }
        return hash<T>{}(*opt);
    }
};

// numeric_limits for slim::optional — reflects the reduced valid range
template<class T>
    requires slim::has_sentinel_traits<T> && numeric_limits<T>::is_specialized
struct numeric_limits<slim::optional<T>> : numeric_limits<T> {
    static constexpr T min() noexcept {
        if constexpr (std::signed_integral<T>)
            return numeric_limits<T>::min() + 1;
        else
            return numeric_limits<T>::min();
    }

    static constexpr T lowest() noexcept {
        if constexpr (std::signed_integral<T>)
            return numeric_limits<T>::min() + 1;
        else
            return numeric_limits<T>::lowest();
    }

    static constexpr T max() noexcept {
        if constexpr (std::unsigned_integral<T> || std::same_as<T, char16_t> || std::same_as<T, char32_t>)
            return numeric_limits<T>::max() - 1;
        else
            return numeric_limits<T>::max();
    }

    static constexpr bool has_quiet_NaN = std::floating_point<T> ? false : numeric_limits<T>::has_quiet_NaN;
    static constexpr bool has_signaling_NaN = std::floating_point<T> ? false : numeric_limits<T>::has_signaling_NaN;
};

} // namespace std
