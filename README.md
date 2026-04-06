# slim::optional

A sentinel-based `optional` for C++23. Same API as `std::optional`, half the memory.

```cpp
#include <slim/optional.hpp>

slim::optional<int*> ptr;          // 8 bytes  (std::optional<int*> = 16)
slim::optional<int>  val;          // 4 bytes  (std::optional<int>  = 8)
slim::optional<double> d;          // 8 bytes  (std::optional<double> = 16)
```

Instead of storing a `bool` flag alongside the value, `slim::optional<T>` reserves one value from `T`'s domain as a sentinel (e.g. `nullptr` for pointers, `NaN` for floats, `numeric_limits::min()` for signed integers). The sentinel is chosen automatically via `sentinel_traits<T>`.

## Building

Requires a C++23 compiler (GCC 13+, Clang 17+).

```bash
cmake -B build
cmake --build build
ctest --test-dir build
```

## Usage

### Drop-in header

Copy `include/slim/optional.hpp` into your project and add the include path.

### CMake subdirectory

```cmake
add_subdirectory(slim-optional)
target_link_libraries(myapp PRIVATE slim_optional)
```

Then:

```cpp
#include <slim/optional.hpp>

slim::optional<int*> ptr;       // nullptr sentinel, automatic
slim::optional<int> val;        // INT_MIN sentinel, automatic
slim::optional<float> f;        // NaN sentinel, automatic

// Full std::optional API
ptr = &some_int;
if (ptr) std::cout << **ptr;
ptr.reset();
auto v = val.value_or(42);
auto mapped = ptr.transform([](int* p) { return *p; });
```

### Built-in sentinel_traits

These types work out of the box:

| Type | Sentinel |
|------|----------|
| Signed integers (`int`, `int64_t`, ...) | `numeric_limits::min()` |
| Unsigned integers (`unsigned`, `size_t`, ...) | `numeric_limits::max()` |
| `float`, `double` | `NaN` (bit-exact) |
| `long double` | `NaN` |
| Pointers (`T*`) | `nullptr` |
| `char16_t`, `char32_t` | `0xFFFF`, `0xFFFFFFFF` |
| `unique_ptr<T>`, `shared_ptr<T>`, `weak_ptr<T>` | null/empty |
| `string_view`, `span<T>` | null data pointer |
| `function<F>`, `move_only_function<F>` | empty callable |
| `coroutine_handle<P>` | null handle |
| `any` | empty |

### Custom types

Specialize `sentinel_traits` to enable `slim::optional` for your own types:

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

slim::optional<FileHandle> fd;                  // 4 bytes (std::optional = 8)
fd = FileHandle::STDOUT;
// fd = FileHandle::INVALID;                    // throws bad_optional_access
```

### Interop with std::optional

```cpp
std::optional<int*> std_opt = &x;
slim::optional<int*> slim_opt(std_opt);         // construct from std::optional
std::optional<int*> back = slim_opt;            // implicit conversion back
assert(slim_opt == std_opt);                    // comparison works
```

## Safety

Assigning the sentinel value directly is a programming error and throws `slim::bad_optional_access`. Use `reset()` or `nullopt` to clear.

```cpp
slim::optional<int*> opt;
opt = nullptr;      // throws slim::bad_optional_access
opt.reset();        // OK
opt = slim::nullopt; // OK
```

## Project layout

```
include/slim/optional.hpp    # the library (header-only)
tests/tests.cpp              # test suite
examples/examples.cpp        # usage examples
benchmarks/                  # memory and performance benchmarks
```
