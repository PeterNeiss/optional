# Analysis: Constructor/Destructor Semantics of slim::optional vs std::optional

## Context

`slim::optional<T>` achieves `sizeof(optional<T>) == sizeof(T)` by storing `T value_` directly as a class member and using a sentinel value to represent the empty state. `std::optional<T>` uses a discriminated union (union + bool flag), where T is only constructed when the optional is engaged. This architectural difference leads to fundamentally different constructor/destructor calling patterns on T.

---

## Detailed Comparison

### 1. Default / Empty Construction

| Operation | `std::optional<T>` | `slim::optional<T>` |
|---|---|---|
| `optional()` | **No T constructor runs.** The union member is left uninitialized. | **T is constructed** with the sentinel value: `T(sentinel_traits<T>::sentinel())`. |

slim::optional always has a live T object. There is no state where `value_` is uninitialized.

### 2. Destruction

| Operation | `std::optional<T>` | `slim::optional<T>` |
|---|---|---|
| `~optional()` (engaged) | T's destructor is called. | T's destructor is called (defaulted member destruction). |
| `~optional()` (empty) | **T's destructor is NOT called.** Nothing to destroy. | **T's destructor IS called** on the sentinel value. |

slim::optional always runs T's destructor, regardless of engaged state.

### 3. `reset()` (engaged -> empty)

| Operation | `std::optional<T>` | `slim::optional<T>` |
|---|---|---|
| `reset()` | **Explicitly calls T's destructor** (`value_.~T()`), then sets the bool flag to false. | **Assigns sentinel** to `value_` via `operator=`. No explicit destructor call. T continues to exist, holding the sentinel. |

### 4. `emplace(args...)` (replace contained value)

| Operation | `std::optional<T>` | `slim::optional<T>` |
|---|---|---|
| emplace (was engaged) | **Destroys** old T, then **placement-new constructs** new T in the union storage. | Constructs a `T tmp`, validates it, then **move-assigns** `tmp` into `value_`. Old value overwritten by assignment, not by explicit destruction. |
| emplace (was empty) | **Placement-new constructs** T. No destruction. | Same as above: constructs tmp, move-assigns. The sentinel value is overwritten by assignment. |

### 5. Assignment from nullopt

| Operation | `std::optional<T>` | `slim::optional<T>` |
|---|---|---|
| `opt = nullopt` (was engaged) | **Destroys** T. Sets flag to false. | **Assigns** sentinel to `value_`. No explicit destruction. |
| `opt = nullopt` (was empty) | No-op. | Assigns sentinel (self-assignment of sentinel, effectively a no-op). |

### 6. Value Assignment

| State Transition | `std::optional<T>` | `slim::optional<T>` |
|---|---|---|
| empty -> engaged | **Placement-new constructs** T from the value. | **Assigns** value to `value_` (which held sentinel). |
| engaged -> engaged | **Assigns** value to existing T. | **Assigns** value to `value_`. Same. |

### 7. Optional-to-Optional Assignment

| State Transition | `std::optional<T>` | `slim::optional<T>` |
|---|---|---|
| engaged -> empty | **Destroys** T. | **Assigns** sentinel. |
| empty -> engaged | **Placement-new constructs** T. | **Assigns** value. |
| engaged -> engaged | **Assigns** value. | **Assigns** value. |
| empty -> empty | No-op. | Assigns sentinel (effectively no-op). |

`std::optional` uses a complex matrix of destroy/construct/assign depending on both sides' engaged state. `slim::optional` **always uses assignment**.

---

## Summary of Key Differences

1. **T is always alive** in slim::optional. There is no state where the T object doesn't exist. std::optional has a true "empty" state with no live T.

2. **Construction on empty**: slim::optional pays the cost of constructing T with the sentinel value. std::optional constructs nothing.

3. **No explicit destruction**: slim::optional never explicitly calls T's destructor. Transitions to empty are done via assignment of the sentinel, not destruction. The only destructor call is the implicit one when the optional itself is destroyed.

4. **Assignment instead of destroy+construct**: Where std::optional would destroy the old T and placement-new a new one (e.g., empty->engaged transition, or emplace), slim::optional uses T's assignment operator.

---

## Is This Reasonable?

**Yes, for the types slim::optional is designed for.** The `requires has_sentinel_traits<T>` constraint is the key safeguard.

### Why it works well for sentinel-capable types

The built-in sentinel_traits specializations cover: integers, floats, pointers, `unique_ptr`, `shared_ptr`, `string_view`, `span`, `function`, `chrono` types, etc. For virtually all of these:

- **Constructors and destructors are trivial or near-trivial.** Constructing an `int` with `INT_MIN` or a pointer with `nullptr` has zero observable cost. Destroying them is a no-op.
- **Assignment and construction are semantically equivalent.** For scalar types, `value_ = 42` and placement-new with `42` produce identical codegen.
- **The sentinel is a natural "empty" state.** `nullptr` for pointers, NaN for floats, `INT_MIN` for signed integers --- these are values the type can already hold and that assignment handles naturally.

For these types, the distinction between "destroy then construct" and "assign" is purely academic. The compiler will generate identical machine code.

### For non-trivial types with sentinel_traits (e.g., `unique_ptr`, `shared_ptr`, `function`)

These types have non-trivial destructors (resource cleanup). The question is whether assignment-to-sentinel is equivalent to destruction:

- **`unique_ptr`**: `ptr = nullptr` releases the owned resource, equivalent to destruction. The sentinel IS nullptr. Assigning sentinel correctly frees the resource.
- **`shared_ptr`**: `ptr = nullptr` decrements the refcount, equivalent to destruction. Same reasoning.
- **`function`**: `fn = sentinel` would release the stored callable via assignment. The assignment operator handles cleanup.

For well-designed C++ types, `x = empty_state` and `x.~T(); new(&x) T(empty_state)` are semantically equivalent. The assignment operator is required to clean up old resources before taking on new state.

### Where it would be problematic (but these types shouldn't have sentinel_traits)

The approach breaks down for types where:
- **Construction has side effects** (e.g., registering with a global registry) --- an "empty" slim::optional still registers
- **Destruction order matters** relative to construction (RAII with strict pairing)
- **Assignment is not equivalent to destroy+construct** (rare, but possible with pathological types)

But such types would not naturally have sentinel values, and the `has_sentinel_traits` constraint prevents them from being used.

### One subtle behavioral difference worth noting

`slim::optional<unique_ptr<T>> opt;` --- this creates a `unique_ptr` holding `nullptr`. It is a live, valid, default-constructed `unique_ptr`. With `std::optional<unique_ptr<T>>`, no `unique_ptr` exists at all. For `unique_ptr` this distinction is invisible. But if someone wrote `sentinel_traits` for a type where construction itself is expensive or has side effects, the cost model differs from `std::optional`.

---

## Conclusion

slim::optional trades the construct/destroy lifecycle model of std::optional for an always-alive assignment model. This is a sound design choice because:

1. The sentinel_traits constraint naturally selects types where this tradeoff is free
2. For all built-in specializations, the compiler generates equivalent or identical code
3. The always-alive model is actually simpler and avoids the complex state matrix of std::optional
4. It enables the core value proposition: `sizeof(optional<T>) == sizeof(T)`

The only risk is user-defined `sentinel_traits` for types where the assignment model doesn't match the destroy+construct model, but that's the user's responsibility when opting in via the traits mechanism.
