# slim::optional

A header-only C++23 library. Drop-in `optional` that uses sentinel values instead of a `bool` flag — same API as `std::optional`, half the memory.

`slim::optional` only works with types that have a sentinel value defined via `sentinel_traits`. For types without a natural sentinel (e.g., `bool`, `char`, aggregates), use `std::optional`.

```
std::optional<int*>     16 bytes     slim::optional<int*>     8 bytes
std::optional<int>       8 bytes     slim::optional<int>      4 bytes
std::optional<double>   16 bytes     slim::optional<double>   8 bytes
```

## Getting started

**Copy the header** into your project:

```
cp include/slim/optional.hpp your_project/include/slim/
```

Or **add as a CMake subdirectory**:

```cmake
add_subdirectory(slim-optional)       # path to this repo
target_link_libraries(myapp PRIVATE slim_optional)
```

Either way:

```cpp
#include <slim/optional.hpp>

int x = 42;
slim::optional<int*> ptr = &x;    // 8 bytes, not 16
if (ptr) std::cout << **ptr;      // 42
ptr.reset();                      // empty
```

Requires GCC 13+ or Clang 17+.

## API

Mostly mirrors `std::optional`: constructors, `value()`, `value_or()`, `operator*`, `operator->`, `reset()`, `emplace()`, `swap()`, and the C++23 monadic operations (`and_then`, `transform`, `or_else`). Plus CTAD:

```cpp
slim::optional opt(42);            // deduces optional<int>
auto opt2 = slim::make_optional(&x);
slim::optional in{slim::in_place, 42};  // also deduces optional<int>
```

### Differences from `std::optional`

- **`or_else` callable receives the `Traits` object.** Recovery lambdas are
  invoked as `f(const Traits&)` so they can use trait-provided helpers when
  fabricating a fallback. Migrating from `[]{ return ...; }` to
  `[](const auto&){ return ...; }` is a one-line change.
- **Move-from does not disengage the source.** Because the empty state is a
  sentinel value rather than a separate flag, moving the value out leaves the
  source holding a moved-from `T` but with `has_value() == true`. Use the new
  `take()` method when you specifically want move-and-reset semantics:
  ```cpp
  slim::optional<std::unique_ptr<Foo>> opt(std::make_unique<Foo>());
  auto p = opt.take();   // p owns the Foo; opt is now empty.
  ```
- **`bad_optional_access` inherits from `std::bad_optional_access`**, so
  `catch (const std::bad_optional_access&)` catches both.
- **`slim::nullopt_t` and `slim::in_place_t` are aliases for the std types**,
  so `std::nullopt` and `std::in_place` work interchangeably.
- **`sentinel_value()`** is a public static accessor returning the trait's
  sentinel — useful for C-API interop where you need the same magic value.
- **`can_be_empty`** is a `static constexpr bool` member that's `false` only
  for the `never_empty` escape hatch.

### Escape hatches: `never_empty` and `always_empty`

Two type-level traits let you fix an optional's state at compile time.

**`never_empty<T>`** — the optional always holds a value. Methods that would
create an empty state (`reset()`, assignment from `nullopt`) are
SFINAE-disabled.

```cpp
slim::optional<int, slim::never_empty<int>> always_engaged{42};
// always_engaged.reset();      // compile error
// always_engaged = nullopt;    // compile error
```

**`always_empty<T>`** — the optional structurally never holds a value, and
**carries no `T` storage at all**: `sizeof == 1`. Useful in `if constexpr`
branches where the templated return type is "the same shape" as a regular
optional, but on this branch you statically never produce one:

```cpp
template<class T>
auto lookup() {
    if constexpr (is_cacheable_v<T>) return slim::optional<T>{...};
    else                              return slim::optional<T, slim::always_empty<T>>{};
}
```

Value-taking constructors, `emplace()`, `operator*`, and `operator->` are
not provided — calling any of them is a compile error. `value()` throws,
`value_or()` returns the default, `transform`/`and_then` short-circuit to
empty.

## Supported types

These work out of the box via built-in `sentinel_traits` specializations:

**Always available:**

| Type | Sentinel value |
|------|---------------|
| Signed integers (`int`, `int64_t`, ...) | `numeric_limits::min()` |
| Unsigned integers (`unsigned`, `size_t`, ...) | `numeric_limits::max()` |
| `float`, `double`, `long double` | NaN (any) |
| Pointers (`T*`) | `nullptr` |
| `char16_t`, `char32_t` | `0xFFFF`, `0xFFFFFFFF` |
| `exception_ptr` | null |

**Requires full mode (disabled by `SLIM_OPTIONAL_LEAN_AND_MEAN`):**

| Type | Sentinel value |
|------|---------------|
| `unique_ptr<T>`, `shared_ptr<T>` | null |
| `string_view`, `span<T>` | null data pointer |
| `function<F>`, `move_only_function<F>` | empty |
| `coroutine_handle<P>` | null handle |
| `any` | empty |
| `thread::id` | default-constructed |
| `stop_token` | non-stoppable |
| `chrono::duration<Rep>`, `chrono::time_point<C,D>` | underlying sentinel |
| `complex<T>` | NaN |

## Custom types

Specialize `sentinel_traits` in namespace `slim` to opt in your own types.
Mark `sentinel()` and `is_sentinel()` as `protected` — `slim::optional`
inherits from your traits and accesses these as a base class, while
preventing arbitrary external callers from poking at them:

```cpp
enum class FileHandle : int { INVALID = -1, STDIN = 0, STDOUT = 1 };

namespace slim {
template<>
struct sentinel_traits<FileHandle> {
protected:
    static constexpr FileHandle sentinel() noexcept { return FileHandle::INVALID; }
    static constexpr bool is_sentinel(const FileHandle& v) noexcept {
        return v == FileHandle::INVALID;
    }
};
}

slim::optional<FileHandle> fd;           // 4 bytes (std::optional<FileHandle> = 8)
fd = FileHandle::STDOUT;
```

You can also pass a custom traits as the second template parameter without
specializing `sentinel_traits` at all:

```cpp
struct EmptyStringIsEmpty {
protected:
    static std::string sentinel() { return std::string{}; }
    static bool is_sentinel(const std::string& s) noexcept { return s.empty(); }
};
slim::optional<std::string, EmptyStringIsEmpty> name;
```

## std::optional interop

Construct from, convert to, and compare with `std::optional`. The `and_then` monadic operation accepts callbacks returning either `slim::optional` or `std::optional`.

```cpp
std::optional<int*> std_opt = &x;
slim::optional<int*> slim_opt(std_opt);  // construct from std::optional
std::optional<int*> back = slim_opt;     // implicit conversion back
assert(slim_opt == std_opt);             // comparison works

// and_then works with std::optional return type
auto result = slim_opt.and_then([](int* p) -> std::optional<int> {
    return *p * 2;
});
```

## numeric_limits

`std::numeric_limits<slim::optional<T>>` is specialized for numeric types and reflects the reduced valid range:

```cpp
// Signed: min() is the sentinel, so valid min is one higher
std::numeric_limits<slim::optional<int>>::min()    // INT_MIN + 1
std::numeric_limits<slim::optional<int>>::lowest()  // INT_MIN + 1
std::numeric_limits<slim::optional<int>>::max()     // INT_MAX (unchanged)

// Unsigned: max() is the sentinel, so valid max is one lower
std::numeric_limits<slim::optional<unsigned>>::max() // UINT_MAX - 1

// Float: NaN is the sentinel, so NaN is no longer available
std::numeric_limits<slim::optional<float>>::has_quiet_NaN    // false
std::numeric_limits<slim::optional<float>>::has_signaling_NaN // false
```

## Object lifetime

Unlike `std::optional`, which uses a union and only constructs `T` when engaged, `slim::optional` always holds a live `T` object — either the real value or the sentinel. This gives it a simpler lifetime model:

- **No placement-new or explicit destructor calls.** All state transitions use assignment. `reset()` assigns the sentinel; `emplace()` move-assigns; `operator=` assigns.
- **No engaged/empty state matrix.** `std::optional::operator=` has four cases (engaged/empty x engaged/empty), each with different construct/destroy/assign behavior. `slim::optional` always assigns.

This is safe for types with trivial constructors and destructors (integers, floats, pointers, `char16_t`/`char32_t`) — constructing an `int` with `INT_MIN` or destroying one is a no-op, so the always-alive model produces identical codegen to `std::optional`.

For `unique_ptr` and `shared_ptr`, the sentinel is `nullptr` — the same state as a default-constructed smart pointer. Assigning `nullptr` releases the owned resource just as destruction would, so `reset()` and `operator=(nullopt)` correctly free memory. An empty `slim::optional<unique_ptr<T>>` simply holds a null `unique_ptr`, which is a zero-cost, well-defined state.

For a detailed comparison of constructor/destructor semantics against `std::optional`, see [ANALYSIS.md](ANALYSIS.md).

## Sentinel safety

Storing the sentinel value directly is a programming error and throws `slim::bad_optional_access`. Use `reset()` or `nullopt` to clear.

```cpp
slim::optional<int*> opt(&x);
opt = nullptr;       // throws slim::bad_optional_access
opt.reset();         // OK
opt = slim::nullopt; // OK
```

## Lean and mean mode

Define `SLIM_OPTIONAL_LEAN_AND_MEAN` to minimize standard library includes and disable `sentinel_traits` for std library types. Core types (integers, floats, pointers, `char16_t`/`char32_t`, `exception_ptr`), custom user types, and `std::optional` interop all still work.

```cmake
# CMake
cmake -B build -DSLIM_OPTIONAL_LEAN_AND_MEAN=ON

# Or define directly
target_compile_definitions(myapp PRIVATE SLIM_OPTIONAL_LEAN_AND_MEAN)
```

## Building from source

```bash
cmake -B build
cmake --build build
ctest --test-dir build          # run tests
./build/examples/examples       # run examples
./build/benchmarks/perf_bench   # run benchmarks
```

## License

MIT
