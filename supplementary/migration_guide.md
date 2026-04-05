# Migration Guide: Adopting `slim_optional`

## Introduction

This guide helps you adopt `std::slim_optional` in your codebase. The key principle: **opt-in gradually, starting with new code.**

## Quick Start

### When to Use `slim_optional`

Use `std::slim_optional<T, sentinel>` when:

- T has a natural invalid value (pointers, handles, enums)
- Memory efficiency matters (containers, embedded systems)
- You control both definition and usage (not crossing ABI boundaries)
- New code or internal implementation (safe to change)

Stick with `std::optional<T>` when:

- T uses all its value space (no spare value for sentinel)
- Public API with ABI stability guarantees (changing types breaks ABI)
- Uncertainty about the best sentinel value
- Legacy code not worth changing

### Basic Migration Pattern

```cpp
// Before:
std::optional<int*> ptr;

// After:
std::slim_optional<int*, nullptr> ptr;

// API is compatible - minimal changes needed!
// Conversions between std::optional and slim_optional are implicit.
```

## Step-by-Step Migration

### Step 1: Identify Candidates

Look for these patterns in your codebase:

```cpp
// Pointers (best candidates)
std::optional<Widget*>
std::optional<const char*>

// File handles / system resources
std::optional<int> file_descriptor;  // If -1 means invalid
std::optional<HANDLE> windows_handle;  // If INVALID_HANDLE_VALUE exists

// Enums with reserved values
enum class Status { Invalid = 0, OK = 1, Error = 2 };
std::optional<Status> status;

// Integers with reserved values
std::optional<int32_t> id;  // If -1 means "no id"
std::optional<size_t> index;  // If SIZE_MAX means "no index"
```

### Step 2: Verify Sentinel Value

Ensure your sentinel value truly represents "invalid":

```cpp
// Good: nullptr is universally invalid for pointers
std::slim_optional<T*, nullptr>

// Good: -1 is the standard "invalid file descriptor"
std::slim_optional<int, -1> fd;

// Bad: 0 might be a valid value!
std::slim_optional<unsigned, 0> count;  // Don't do this - 0 is valid!

// Good: Reserved enum value
enum class State { INVALID = -1, IDLE = 0, RUNNING = 1 };
std::slim_optional<State, State::INVALID> state;
```

### Step 3: Update Type Declarations

Use type aliases for easier migration:

```cpp
// Before:
class Connection {
    std::optional<Socket*> socket_;
public:
    std::optional<Socket*> get_socket() const;
};

// Step 1: Introduce alias (no behavior change yet)
class Connection {
    using SocketOpt = std::optional<Socket*>;
    SocketOpt socket_;
public:
    SocketOpt get_socket() const;
};

// Step 2: Change alias definition (all usage updates automatically)
class Connection {
    using SocketOpt = std::slim_optional<Socket*, nullptr>;
    SocketOpt socket_;
public:
    SocketOpt get_socket() const;
};
```

### Step 4: Update Initializations

The API is compatible with `std::optional`:

```cpp
std::slim_optional<int*, nullptr> opt;        // Empty
opt = &value;                                  // Set value
opt.reset();                                   // Clear
opt = std::nullopt;                            // Clear
if (opt) { /* ... */ }                         // Check
auto val = opt.value_or(default_ptr);          // Get with default
```

### Step 5: Handle Sentinel Assignment (New Safety Feature)

Sentinel values are protected:

```cpp
std::slim_optional<int*, nullptr> opt;

// This throws bad_optional_access
opt = nullptr;  // ERROR - be explicit!

// Use instead:
opt.reset();    // Clear and explicit
opt = std::nullopt;  // Also clear and explicit
```

**Migration Action:** Search for assignments of sentinel values and replace:

```bash
# Find potential issues:
grep -r "opt.*=.*nullptr" .
grep -r "handle.*=.*-1" .
```

```cpp
// Fix pattern:
// Before:
pointer_opt = nullptr;
handle_opt = -1;

// After:
pointer_opt.reset();
handle_opt.reset();
```

## Migration Strategies

### Strategy 1: New Code Only (Safest)

Start using `slim_optional` in all new code:

```cpp
// New class - use slim_optional from start
class NewFeature {
    std::slim_optional<Resource*, nullptr> resource_;  // Optimized
public:
    // ...
};

// Old class - don't change
class LegacyCode {
    std::optional<Resource*> resource_;  // Leave as-is
public:
    // ...
};
```

**Pros:**
- Zero risk to existing code
- Immediate benefits in new code
- No testing burden for old code

**Cons:**
- Inconsistent usage across codebase
- Existing code misses optimization

**Best for:** Large codebases, risk-averse teams

### Strategy 2: Internal First (Balanced)

Migrate internal implementation details first, keep public APIs unchanged:

```cpp
// Public API - keep stable
class MyLibrary {
public:
    std::optional<Data*> get_data() const;  // Unchanged for ABI
private:
    std::slim_optional<Data*, nullptr> data_;  // Optimized internally

    std::slim_optional<Data*, nullptr> find_internal(int id) const;
};
```

Note: `slim_optional` implicitly converts to `std::optional`, so returning from
internal `slim_optional` to public `std::optional` works seamlessly.

**Pros:**
- Public API stability maintained
- Internal optimization benefits
- Low risk

**Cons:**
- Mixed types at API boundary (implicit conversion handles this)

**Best for:** Libraries with public APIs, gradual adoption

### Strategy 3: Whole Module (Aggressive)

Migrate entire modules at once:

```cpp
// Before migration: Module A uses std::optional
namespace module_a {
    std::optional<Widget*> create_widget();
    void process(std::optional<Widget*> w);
}

// After migration: All of Module A uses slim_optional
namespace module_a {
    std::slim_optional<Widget*, nullptr> create_widget();
    void process(std::slim_optional<Widget*, nullptr> w);
}
```

**Pros:**
- Consistent within module
- Full optimization benefits
- Clean boundaries

**Cons:**
- Requires coordinated changes
- More testing needed
- API breaks if module is a library

**Best for:** Applications, internal modules

## Real-World Examples

### Example 1: Game Engine Entity System

**Before:**
```cpp
class Entity {
    std::optional<Transform*> transform_;
    std::optional<Renderer*> renderer_;
    std::optional<Physics*> physics_;
    // Thousands of entities x 3 optionals x 16 bytes = lots of memory!
};
```

**After:**
```cpp
class Entity {
    std::slim_optional<Transform*, nullptr> transform_;
    std::slim_optional<Renderer*, nullptr> renderer_;
    std::slim_optional<Physics*, nullptr> physics_;
    // Thousands of entities x 3 optionals x 8 bytes = 50% savings!
};
```

**Impact:** With 10,000 entities, saves ~240 KB.

### Example 2: Network Connection Pool

**Before:**
```cpp
class ConnectionPool {
    std::array<std::optional<Connection*>, 1000> connections_;
    // 1000 x 16 bytes = 16 KB
};
```

**After:**
```cpp
class ConnectionPool {
    std::array<std::slim_optional<Connection*, nullptr>, 1000> connections_;
    // 1000 x 8 bytes = 8 KB (50% reduction)
};
```

### Example 3: File Handle Manager

**Before:**
```cpp
class FileManager {
    std::unordered_map<std::string, std::optional<int>> handles_;
    // Each optional is 8 bytes (4 for int + 4 for bool+padding)
};
```

**After:**
```cpp
class FileManager {
    std::unordered_map<std::string, std::slim_optional<int, -1>> handles_;
    // Each optional is 4 bytes (just the int!)
    // Sentinel is safe because -1 is POSIX standard for invalid fd
};
```

### Example 4: Enum State Machine

**Before:**
```cpp
enum class State : int8_t {
    IDLE = 0,
    RUNNING = 1,
    STOPPED = 2
};

class StateMachine {
    std::optional<State> current_;  // 2-4 bytes (bool + enum + padding)
    std::optional<State> next_;
};
```

**After:**
```cpp
enum class State : int8_t {
    INVALID = -1,  // Reserve sentinel value
    IDLE = 0,
    RUNNING = 1,
    STOPPED = 2
};

class StateMachine {
    std::slim_optional<State, State::INVALID> current_;  // 1 byte!
    std::slim_optional<State, State::INVALID> next_;
};
```

## Common Pitfalls and Solutions

### Pitfall 1: Forgetting Sentinel Protection

```cpp
// Wrong:
std::slim_optional<int*, nullptr> opt;
if (some_condition) {
    opt = nullptr;  // Throws! Forgot this is protected now
}

// Right:
std::slim_optional<int*, nullptr> opt;
if (some_condition) {
    opt.reset();  // Explicit - clearly means "no value"
}
```

**Solution:** Search for `= nullptr` and `= -1` patterns in your codebase.

### Pitfall 2: Mixing std::optional and slim_optional

```cpp
std::optional<int*> std_opt;
std::slim_optional<int*, nullptr> slim_opt;

// These conversions are implicit:
slim_opt = std_opt;                      // OK
std::optional<int*> from_slim = slim_opt; // OK

// Comparisons work across types:
if (std_opt == slim_opt) { /* ... */ }    // OK
```

**Solution:** Use consistent types within a module. Use type aliases.

### Pitfall 3: Wrong Sentinel Value

```cpp
// Wrong: 0 might be a valid count!
std::slim_optional<unsigned, 0> count;

// Right: Use UINT_MAX as sentinel
std::slim_optional<unsigned, UINT_MAX> count;

// Or: Stick with std::optional if no good sentinel
std::optional<unsigned> count;
```

**Solution:** Carefully choose sentinel values that are truly invalid.

### Pitfall 4: Public API Changes

```cpp
// library.h v1.0
class MyLib {
public:
    std::optional<Data*> get_data() const;  // Public API
};

// library.h v2.0 - This is an API/ABI break!
class MyLib {
public:
    std::slim_optional<Data*, nullptr> get_data() const;  // Different type!
};
```

**Solution:** Keep public APIs stable. Use `slim_optional` internally only, or bump your major version.

## Testing Your Migration

### Unit Tests

Add tests for sentinel protection:

```cpp
TEST(SlimOptional, RejectsSentinel) {
    EXPECT_THROW({
        std::slim_optional<int*, nullptr> opt(nullptr);
    }, std::bad_optional_access);

    EXPECT_THROW({
        std::slim_optional<int*, nullptr> opt;
        opt = nullptr;
    }, std::bad_optional_access);
}
```

### Interop Tests

Verify conversion between `std::optional` and `slim_optional`:

```cpp
TEST(Migration, InteropWorks) {
    // std::optional -> slim_optional
    std::optional<int*> std_opt;
    int value = 42;
    std_opt = &value;

    std::slim_optional<int*, nullptr> slim(std_opt);
    EXPECT_EQ(*slim, &value);

    // slim_optional -> std::optional
    std::optional<int*> back = slim;
    EXPECT_EQ(*back, &value);

    // Comparison across types
    EXPECT_EQ(slim, std_opt);
}
```

### Memory Tests

Verify size reductions:

```cpp
TEST(Migration, MemorySavings) {
    EXPECT_EQ(sizeof(std::slim_optional<int*, nullptr>), sizeof(int*));
    EXPECT_GT(sizeof(std::optional<int*>), sizeof(int*));
}
```

## Rollback Plan

If you need to rollback:

### Option 1: Type Alias Revert

```cpp
// Deployed version with slim_optional:
using ResourceOpt = std::slim_optional<Resource*, nullptr>;

// Rollback (change one line):
using ResourceOpt = std::optional<Resource*>;

// All code automatically reverts to std::optional
```

### Option 2: Conditional Compilation

```cpp
#ifdef USE_SLIM_OPTIONAL
    using ResourceOpt = std::slim_optional<Resource*, nullptr>;
#else
    using ResourceOpt = std::optional<Resource*>;
#endif
```

## Best Practices

1. **Use type aliases:**
   ```cpp
   using WidgetPtr = std::slim_optional<Widget*, nullptr>;
   // Change definition in one place, updates everywhere
   ```

2. **Document sentinel values:**
   ```cpp
   // Use -1 as sentinel (POSIX invalid fd)
   using FileDescriptor = std::slim_optional<int, -1>;
   ```

3. **Be consistent within modules:**
   ```cpp
   namespace networking {
       using ConnectionOpt = std::slim_optional<Connection*, nullptr>;
       // All code in this namespace uses this type
   }
   ```

4. **Test edge cases:**
   ```cpp
   TEST(SlimOptional, EdgeCases) {
       std::slim_optional<int*, nullptr> opt;
       EXPECT_THROW(opt.emplace(nullptr), std::bad_optional_access);
       opt.reset();
       opt.reset();  // Double reset - should be safe
   }
   ```

5. **Profile before and after:**
   - Measure actual memory usage
   - Verify performance improvements
   - Check cache miss rates

## Conclusion

Migrating to `slim_optional` is straightforward:

1. Start with new code or internal implementation
2. Use type aliases for easy switching
3. Test thoroughly, especially sentinel protection
4. Measure benefits (memory, performance)
5. Expand gradually based on confidence

The key insight: **same API as `std::optional`, just more efficient storage, with seamless interop between the two.**
