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

The full `std::optional` interface — constructors, `value()`, `value_or()`, `operator*`, `operator->`, `reset()`, `emplace()`, `swap()`, and the C++23 monadic operations (`and_then`, `transform`, `or_else`). Plus CTAD:

```cpp
slim::optional opt(42);            // deduces optional<int>
auto opt2 = slim::make_optional(&x);
```

## Supported types

These work out of the box via built-in `sentinel_traits` specializations:

| Type | Sentinel value |
|------|---------------|
| Signed integers (`int`, `int64_t`, ...) | `numeric_limits::min()` |
| Unsigned integers (`unsigned`, `size_t`, ...) | `numeric_limits::max()` |
| `float`, `double`, `long double` | NaN (any) |
| Pointers (`T*`) | `nullptr` |
| `char16_t`, `char32_t` | `0xFFFF`, `0xFFFFFFFF` |
| `unique_ptr<T>`, `shared_ptr<T>` | null |
| `string_view`, `span<T>` | null data pointer |
| `function<F>`, `move_only_function<F>` | empty |
| `coroutine_handle<P>` | null handle |
| `any` | empty |

## Custom types

Specialize `sentinel_traits` in namespace `slim` to opt in your own types:

```cpp
enum class FileHandle : int { INVALID = -1, STDIN = 0, STDOUT = 1 };

namespace slim {
template<>
struct sentinel_traits<FileHandle> {
    static constexpr FileHandle sentinel() noexcept { return FileHandle::INVALID; }
    static constexpr bool is_sentinel(const FileHandle& v) noexcept {
        return v == FileHandle::INVALID;
    }
};
}

slim::optional<FileHandle> fd;           // 4 bytes (std::optional<FileHandle> = 8)
fd = FileHandle::STDOUT;
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

## Sentinel safety

Storing the sentinel value directly is a programming error and throws `slim::bad_optional_access`. Use `reset()` or `nullopt` to clear.

```cpp
slim::optional<int*> opt(&x);
opt = nullptr;       // throws slim::bad_optional_access
opt.reset();         // OK
opt = slim::nullopt; // OK
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
