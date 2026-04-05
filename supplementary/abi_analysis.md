# ABI Compatibility Analysis for `slim_optional`

## Executive Summary

The `std::slim_optional<T, SentinelValue>` proposal is **ABI-safe** because:

1. It introduces an entirely new type, not a modification to `std::optional`
2. `std::optional<T>` is completely unchanged — zero risk to existing code
3. `slim_optional` and `optional` are distinct types with no layout ambiguity
4. Interoperability is achieved through conversions, not shared representation

## Detailed Analysis

### Memory Layout Comparison

The two types have fundamentally different layouts:

```cpp
// std::optional<int*> — unchanged
struct optional<int*> {
    bool has_value_;     // 1 byte
    // padding: 7 bytes (on 64-bit)
    int* value_;         // 8 bytes
};
// Total: 16 bytes

// std::slim_optional<int*, nullptr> — new type
struct slim_optional<int*, nullptr> {
    int* value_;         // 8 bytes (nullptr = no value)
};
// Total: 8 bytes
```

### Why a Separate Type Eliminates ABI Concerns

Unlike approaches that extend `std::optional` with additional template parameters, `slim_optional` is a distinct class template. This means:

1. **No impact on `std::optional` whatsoever** — existing code, existing ABIs, existing mangled symbols are all untouched.

2. **No template parameter confusion** — there is no possibility of accidentally instantiating `std::optional` with a sentinel parameter, because it doesn't accept one.

3. **Clear type boundaries** — the type system makes the distinction explicit:
   ```cpp
   std::optional<int*>                      // One type
   std::slim_optional<int*, nullptr>        // A completely different type
   ```

### ABI Boundary Considerations

#### Safe Scenarios

1. **Within a single compilation unit:**
   ```cpp
   // All in one .cpp file - perfectly safe
   std::optional<int*> opt1;
   std::slim_optional<int*, nullptr> opt2;
   ```

2. **Consistent usage across libraries:**
   ```cpp
   // Library A header:
   std::slim_optional<int*, nullptr> get_pointer();

   // Library B uses it:
   auto ptr = library_a::get_pointer();  // OK - same type
   ```

3. **Interop via conversion at boundaries:**
   ```cpp
   // Library A uses std::optional in its public API
   std::optional<int*> get_value();

   // Application converts internally
   std::slim_optional<int*, nullptr> compact = get_value();  // Implicit conversion
   ```

#### Unsafe Scenarios (Explicit Mistakes)

1. **Changing a public API type:**
   ```cpp
   // Library A header (v1):
   std::optional<int*> get_value();

   // Library A header (v2) - ABI BREAK!
   std::slim_optional<int*, nullptr> get_value();
   ```

   But this is **no different** from changing:
   ```cpp
   std::vector<int> get_values();  // v1
   std::list<int> get_values();    // v2 - also breaks ABI!
   ```

   Changing a return type in a public API is always an ABI break.

2. **Unsafe reinterpret_cast (always wrong):**
   ```cpp
   std::optional<int*> opt1;
   auto* opt2 = reinterpret_cast<std::slim_optional<int*, nullptr>*>(&opt1);
   // UNDEFINED BEHAVIOR - but this is always wrong for different types!
   ```

### Name Mangling

`slim_optional` has a completely different mangled name from `optional`:

```cpp
// std::optional<int*>
// Mangled: _ZNSt8optionalIPiE...

// std::slim_optional<int*, nullptr>
// Mangled: _ZNSt13slim_optionalIPiLDn0EE... (different class name!)
```

The linker will always distinguish these as separate symbols.

### Impact on ODR (One Definition Rule)

Since `slim_optional` is its own class template, it follows normal ODR rules independently of `std::optional`. There is no risk of ODR violations between the two types.

### Migration Scenarios

#### Scenario 1: Library Update (Safe)

**Library version 1.0:**
```cpp
// mylib.h
class MyClass {
    std::optional<Resource*> get_resource();
};
```

**Library version 2.0 (ABI-compatible):**
```cpp
// mylib.h - unchanged public API
class MyClass {
    std::optional<Resource*> get_resource();  // Still uses std::optional
};

// mylib_internal.cpp - internal usage changes (OK)
void internal_function() {
    slim_optional<Resource*, nullptr> opt;  // Internal optimization
    // Convert at API boundary
    return std::optional<Resource*>(opt);
}
```

**Library version 3.0 (ABI-breaking, intentional):**
```cpp
// mylib.h - API change
class MyClass {
    std::slim_optional<Resource*, nullptr> get_resource();  // New return type
    // This is a breaking change - bump major version
};
```

#### Scenario 2: Gradual Adoption (Safe)

```cpp
// Use slim_optional for internal data structures
class Cache {
    std::vector<std::slim_optional<int*, nullptr>> entries_;  // 50% less memory

public:
    // Public API can expose either type
    std::optional<int*> get(size_t i) const {
        return entries_[i];  // Implicit conversion
    }
};
```

### Comparison with Alternative Designs

| Approach | ABI Impact on `std::optional` | Risk of Confusion |
|---|---|---|
| **`slim_optional` (this proposal)** | None | Low — distinct type name |
| Extend `optional<T, Sentinel>` | Potential — changes template signature | High — same name, different layouts |
| Specialize `optional` for specific types | None | Medium — implicit behavior change |

The standalone type approach was chosen specifically because it has the lowest ABI risk.

### Tooling and Diagnostics

Modern tools can help catch mistakes:

1. **Compiler errors:**
   ```cpp
   std::slim_optional<int*, nullptr> func();

   std::optional<int*> x = func();  // OK: implicit conversion
   // No risk of ABI mismatch — conversion happens at compile time
   ```

2. **Static analysis:**
   - Type mismatch at ABI boundaries is caught at compile time
   - No special tooling needed beyond normal C++ type checking

## Recommendations

### For Library Authors

1. **Keep `std::optional` in public APIs for stability:**
   ```cpp
   // Public API — stable ABI
   std::optional<Resource*> get_resource();

   // Internal implementation — optimized
   std::slim_optional<Resource*, nullptr> cache_entry_;
   ```

2. **Use type aliases for clarity:**
   ```cpp
   // Public API
   using ResourceOpt = std::optional<Resource*>;

   // Internal
   using CompactResourceOpt = std::slim_optional<Resource*, nullptr>;
   ```

### For Application Developers

1. **Use `slim_optional` freely in application code:**
   ```cpp
   std::slim_optional<Widget*, nullptr> widget_;
   std::slim_optional<int32_t, -1> handle_;
   ```

2. **Convert at library boundaries if needed:**
   ```cpp
   std::slim_optional<int*, nullptr> compact = library::get_pointer();
   ```

### For Standard Library Implementers

1. **`slim_optional` is a new class template — no changes to `optional` required**
2. **Ensure proper mangling of the sentinel NTTP**
3. **Consider providing `__is_slim_optional` type trait for tooling**

## Conclusion

The `slim_optional` proposal is **ABI-safe by design** because:

- `std::optional` is completely untouched
- `slim_optional` is a new, independent type
- Interoperability is through value conversions, not layout compatibility
- The type system prevents all accidental misuse
- This approach was specifically chosen over extending `std::optional` to avoid ABI concerns
