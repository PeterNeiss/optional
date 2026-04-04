# Migration Guide: Adopting Sentinel-Based Optional

## Introduction

This guide helps you adopt the sentinel-based `std::optional` in your codebase. The key principle: **opt-in gradually, starting with new code.**

## Quick Start

### When to Use Sentinel-Based Optional

Use `std::optional<T, sentinel>` when:

- ✅ **T has a natural invalid value** (pointers, handles, enums)
- ✅ **Memory efficiency matters** (containers, embedded systems)
- ✅ **You control both definition and usage** (not crossing ABI boundaries)
- ✅ **New code or internal implementation** (safe to change)

Stick with `std::optional<T>` when:

- ❌ **T uses all its value space** (no spare value for sentinel)
- ❌ **Public API with ABI stability guarantees** (changing types breaks ABI)
- ❌ **Uncertainty about the best sentinel value**
- ❌ **Legacy code not worth changing**

### Basic Migration Pattern

```cpp
// Before:
std::optional<int*> ptr;

// After:
std::optional<int*, nullptr> ptr;

// API is identical - no other changes needed!
```

## Step-by-Step Migration

### Step 1: Identify Candidates

Look for these patterns in your codebase:

```cpp
// Pointers (best candidates)
std::optional<Widget*>
std::optional<const char*>
std::optional<std::shared_ptr<T>>  // Note: shared_ptr already optimized

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
std::optional<T*, nullptr>

// Good: -1 is the standard "invalid file descriptor"
std::optional<int, -1> fd;

// Bad: 0 might be a valid value!
std::optional<unsigned, 0> count;  // Don't do this - 0 is valid!

// Good: Reserved enum value
enum class State { INVALID = -1, IDLE = 0, RUNNING = 1 };
std::optional<State, State::INVALID> state;
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
    using SocketOpt = std::optional<Socket*, nullptr>;  // Changed!
    SocketOpt socket_;
public:
    SocketOpt get_socket() const;
};
```

### Step 4: Update Initializations

No changes needed - API is identical:

```cpp
// Before and After - same code works:
std::optional<int*, nullptr> opt;        // Empty
opt = &value;                             // Set value
opt.reset();                              // Clear
opt = nullopt;                            // Clear
if (opt) { /* ... */ }                    // Check
auto val = opt.value_or(default_ptr);     // Get with default
```

### Step 5: Handle Sentinel Assignment (New Safety Feature)

Sentinel values are now protected:

```cpp
std::optional<int*, nullptr> opt;

// Before: This was allowed (bug-prone!)
opt = nullptr;  // Meant "no value" or "value is nullptr"? Unclear!

// After: This throws bad_optional_access
opt = nullptr;  // ERROR - be explicit!

// Use instead:
opt.reset();    // Clear and explicit
opt = nullopt;  // Also clear and explicit
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

Start using sentinel-based optional in all new code:

```cpp
// New class - use sentinel-based from start
class NewFeature {
    std::optional<Resource*, nullptr> resource_;  // Optimized
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
    std::optional<Data*, nullptr> data_;     // Optimized internally

    std::optional<Data*, nullptr> find_internal(int id) const;  // Internal
};
```

**Pros:**
- Public API stability maintained
- Internal optimization benefits
- Low risk

**Cons:**
- Conversions between types at API boundary
- Some overhead from conversions

**Best for:** Libraries with public APIs, gradual adoption

### Strategy 3: Whole Module (Aggressive)

Migrate entire modules at once:

```cpp
// Before migration: Module A uses bool-based
namespace module_a {
    std::optional<Widget*> create_widget();
    void process(std::optional<Widget*> w);
}

// After migration: All of Module A uses sentinel-based
namespace module_a {
    std::optional<Widget*, nullptr> create_widget();
    void process(std::optional<Widget*, nullptr> w);
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

### Strategy 4: Find-and-Replace (Risky)

Automated migration across entire codebase:

```bash
# Example: Migrate all pointer optionals (be careful!)
find . -name "*.cpp" -o -name "*.h" | \
xargs sed -i 's/std::optional<\([^,>]*\*\)>/std::optional<\1, nullptr>/g'
```

**Pros:**
- Complete migration
- Maximum benefits
- Codebase consistency

**Cons:**
- **High risk** - can break things
- Requires extensive testing
- May break public APIs

**Best for:** Applications with good test coverage, major version updates

## Real-World Examples

### Example 1: Game Engine Entity System

**Before:**
```cpp
class Entity {
    std::optional<Transform*> transform_;
    std::optional<Renderer*> renderer_;
    std::optional<Physics*> physics_;
    // Thousands of entities × 3 optionals × 16 bytes = lots of memory!
};
```

**After:**
```cpp
class Entity {
    std::optional<Transform*, nullptr> transform_;
    std::optional<Renderer*, nullptr> renderer_;
    std::optional<Physics*, nullptr> physics_;
    // Thousands of entities × 3 optionals × 8 bytes = 50% savings!
};
```

**Impact:** With 10,000 entities, saves ~240 KB.

### Example 2: Network Connection Pool

**Before:**
```cpp
class ConnectionPool {
    std::array<std::optional<Connection*>, 1000> connections_;
    // 1000 × 16 bytes = 16 KB
};
```

**After:**
```cpp
class ConnectionPool {
    std::array<std::optional<Connection*, nullptr>, 1000> connections_;
    // 1000 × 8 bytes = 8 KB (50% reduction)
};
```

**Migration:**
```cpp
// Only change needed - type alias:
// Before:
using ConnectionOpt = std::optional<Connection*>;

// After:
using ConnectionOpt = std::optional<Connection*, nullptr>;

// All code using ConnectionOpt automatically updated!
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
    std::unordered_map<std::string, std::optional<int, -1>> handles_;
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
    std::optional<State, State::INVALID> current_;  // 1 byte!
    std::optional<State, State::INVALID> next_;
};
```

## Common Pitfalls and Solutions

### Pitfall 1: Forgetting Sentinel Protection

```cpp
// Wrong:
std::optional<int*, nullptr> opt;
if (some_condition) {
    opt = nullptr;  // Throws! Forgot this is protected now
}

// Right:
std::optional<int*, nullptr> opt;
if (some_condition) {
    opt.reset();  // Explicit - clearly means "no value"
}
```

**Solution:** Search for `= nullptr` and `= -1` patterns in your codebase.

### Pitfall 2: Mixing Bool-Based and Sentinel-Based

```cpp
std::optional<int*> bool_opt;
std::optional<int*, nullptr> sentinel_opt;

// This requires conversion:
bool_opt = sentinel_opt;  // OK but involves conversion
sentinel_opt = bool_opt;  // OK but involves conversion
```

**Solution:** Use consistent types within a module. Use type aliases.

### Pitfall 3: Wrong Sentinel Value

```cpp
// Wrong: 0 might be a valid count!
std::optional<unsigned, 0> count;

// Right: Use SIZE_MAX as sentinel
std::optional<unsigned, UINT_MAX> count;

// Or: Stick with bool-based if no good sentinel
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

// library.h v2.0 - WRONG! ABI break!
class MyLib {
public:
    std::optional<Data*, nullptr> get_data() const;  // Different type!
};
```

**Solution:** Keep public APIs stable. Use sentinel-based internally only.

## Testing Your Migration

### Unit Tests

Add tests for sentinel protection:

```cpp
TEST(SentinelOptional, RejectsSentinel) {
    EXPECT_THROW({
        std::optional<int*, nullptr> opt(nullptr);
    }, std::bad_optional_access);

    EXPECT_THROW({
        std::optional<int*, nullptr> opt;
        opt = nullptr;
    }, std::bad_optional_access);
}
```

### Integration Tests

Verify behavior is unchanged:

```cpp
TEST(Migration, BehaviorUnchanged) {
    // Bool-based
    std::optional<int*> bool_opt;
    int value = 42;
    bool_opt = &value;

    // Sentinel-based
    std::optional<int*, nullptr> sentinel_opt;
    sentinel_opt = &value;

    // Same behavior
    EXPECT_EQ(bool_opt.has_value(), sentinel_opt.has_value());
    EXPECT_EQ(*bool_opt, *sentinel_opt);
}
```

### Memory Tests

Verify size reductions:

```cpp
TEST(Migration, MemorySavings) {
    EXPECT_EQ(sizeof(std::optional<int*, nullptr>), sizeof(int*));
    EXPECT_GT(sizeof(std::optional<int*>), sizeof(int*));

    // 50% savings
    EXPECT_EQ(sizeof(std::optional<int*, nullptr>) * 2,
              sizeof(std::optional<int*>));
}
```

## Rollback Plan

If you need to rollback:

### Option 1: Type Alias Revert

```cpp
// Deployed version with sentinel-based:
using ResourceOpt = std::optional<Resource*, nullptr>;

// Rollback (change one line):
using ResourceOpt = std::optional<Resource*>;

// All code automatically reverts to bool-based
```

### Option 2: Conditional Compilation

```cpp
#ifdef USE_SENTINEL_OPTIONAL
    using ResourceOpt = std::optional<Resource*, nullptr>;
#else
    using ResourceOpt = std::optional<Resource*>;
#endif
```

### Option 3: Git Revert

If using version control:

```bash
git revert <commit-that-added-sentinel-optional>
```

## Best Practices

1. **Use type aliases:**
   ```cpp
   using WidgetPtr = std::optional<Widget*, nullptr>;
   // Change definition in one place, updates everywhere
   ```

2. **Document sentinel values:**
   ```cpp
   // Use -1 as sentinel (POSIX invalid fd)
   using FileDescriptor = std::optional<int, -1>;
   ```

3. **Be consistent within modules:**
   ```cpp
   // Pick one style per type in a module
   namespace networking {
       using ConnectionOpt = std::optional<Connection*, nullptr>;
       // All code in this namespace uses this type
   }
   ```

4. **Test edge cases:**
   ```cpp
   TEST(SentinelOptional, EdgeCases) {
       std::optional<int*, nullptr> opt;
       opt.emplace(nullptr);  // Should throw
       opt.reset();
       opt.reset();  // Double reset - should be safe
   }
   ```

5. **Profile before and after:**
   - Measure actual memory usage
   - Verify performance improvements
   - Check cache miss rates

## Conclusion

Migrating to sentinel-based optional is straightforward:

1. Start with new code or internal implementation
2. Use type aliases for easy switching
3. Test thoroughly, especially sentinel protection
4. Measure benefits (memory, performance)
5. Expand gradually based on confidence

The key insight: **it's the same API, just more efficient storage**. Focus your migration effort on finding good candidates and handling the new sentinel protection safety feature.
