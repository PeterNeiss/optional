# Sentinel-Based std::optional - C++ Standard Proposal

This repository contains a complete C++ standards proposal for extending `std::optional<T>` with a sentinel-based storage optimization.

## Overview

The current `std::optional<T>` stores a boolean flag plus the value, resulting in significant memory overhead. This proposal adds an optional template parameter to specify a sentinel value, eliminating the bool flag for types that have natural invalid values.

```cpp
// Current: 16 bytes (bool + padding + pointer)
std::optional<int*> ptr;

// Proposed: 8 bytes (just the pointer!)
std::optional<int*, nullptr> ptr;

// API is identical - just more efficient!
```

## Key Benefits

- **50% memory savings** for pointers, handles, and many other types
- **Better cache utilization** due to smaller size
- **Zero overhead abstraction** - matches hand-rolled implementations
- **Fully backwards compatible** - existing code unchanged
- **Same API** - all std::optional operations work identically

## Repository Structure

```
optional-cpp-paper/
├── proposal.md                      # Main WG21 proposal (C++29 target)
├── README.md                        # This file
├── SUMMARY.md                       # Project summary and status
├── BENCHMARK_RESULTS.md             # Detailed benchmark data
├── VERIFICATION.md                  # Multi-compiler verification
├── Makefile                         # Build system (GCC + Clang)
├── reference-impl/
│   ├── optional.hpp                 # Complete implementation (857 lines)
│   ├── examples.cpp                 # 13 usage examples (350 lines)
│   └── tests.cpp                   # 51 comprehensive tests (1100 lines)
├── benchmarks/
│   ├── memory_usage.cpp            # Memory analysis (250 lines)
│   └── performance.cpp             # Performance benchmarks (450 lines)
└── supplementary/
    ├── abi_analysis.md             # ABI compatibility (350 lines)
    └── migration_guide.md          # Migration guide (600 lines)

Total: ~5,200 lines of code and documentation
```

## Quick Start

### Building and Running

Requires C++23 compiler (GCC 13+, Clang 17+, or MSVC 19.36+):

```bash
# Compile and run examples
g++ -std=c++23 -O2 reference-impl/examples.cpp -o examples
./examples

# Compile and run tests
g++ -std=c++23 -O2 reference-impl/tests.cpp -o tests
./tests

# Compile and run memory benchmark
g++ -std=c++23 -O2 benchmarks/memory_usage.cpp -o memory_bench
./memory_bench

# Compile and run performance benchmark
g++ -std=c++23 -O2 benchmarks/performance.cpp -o perf_bench
./perf_bench
```

Or use the provided Makefile:

```bash
make all        # Build everything (uses GCC by default)
make gcc        # Build with GCC
make clang      # Build with Clang
make run        # Run examples
make test       # Run tests
make bench      # Run benchmarks
make clean      # Clean build artifacts
```

**Verified to compile and run successfully with:**
- ✅ GCC 15.2.0
- ✅ Clang 18.1.3

### Basic Usage

```cpp
#include "reference-impl/optional.hpp"
using namespace std_proposal;

int main() {
    // Backwards compatible - no changes
    optional<int> old_style(42);

    // Sentinel-based - optimized
    optional<int*, nullptr> ptr_opt;
    int value = 100;
    ptr_opt = &value;

    if (ptr_opt) {
        std::cout << **ptr_opt << '\n';  // 100
    }

    ptr_opt.reset();  // Clear
}
```

## Technical Design

### Template Signature

```cpp
namespace std {
    struct use_bool_storage_t { /* marker type */ };
    inline constexpr use_bool_storage_t use_bool_storage{};

    template<class T, auto SentinelValue = use_bool_storage>
    class optional;
}
```

### Two Specializations

1. **Bool-based (default):** `optional<T, use_bool_storage>`
   - Current std::optional behavior
   - Stores bool flag + value
   - Fully backwards compatible

2. **Sentinel-based:** `optional<T, SentinelValue>`
   - No bool flag needed
   - Stores just the value
   - Sentinel value represents "no value"

### Safety Features

The sentinel value is protected:

```cpp
optional<int*, nullptr> opt;

// Error - can't use sentinel value!
opt = nullptr;  // Throws bad_optional_access

// Correct - explicitly set "no value"
opt.reset();    // OK
opt = nullopt;  // OK
```

## Use Cases

### Pointers
```cpp
optional<Widget*, nullptr> widget;  // 8 bytes vs 16
```

### File Handles
```cpp
optional<int, -1> fd;  // 4 bytes vs 8 (POSIX invalid fd = -1)
```

### Enums
```cpp
enum class Status : int8_t { INVALID = -1, OK = 0, ERROR = 1 };
optional<Status, Status::INVALID> status;  // 1 byte vs 2-4!
```

### Resource Handles
```cpp
struct Handle { int id; bool operator==(const Handle&) const = default; };
constexpr Handle INVALID{-1};
optional<Handle, INVALID> handle;  // 4 bytes vs 8
```

## Proposal Document

The main proposal document (`proposal.md`) follows WG21 format and includes:

1. **Introduction & Motivation** - The problem and real-world impact
2. **Design Goals** - Backwards compatibility, safety, performance
3. **Technical Specification** - Complete API and implementation details
4. **Usage Examples** - Comprehensive code examples
5. **Design Alternatives** - Why other approaches were rejected
6. **Impact on the Standard** - Changes required, ABI considerations
7. **Implementation Experience** - Reference implementation and benchmarks
8. **Formal Wording** - Standard-ese text for [optional.optional]

## Benchmarks

### Memory Usage (64-bit system)

**Per-Type Savings:**

| Type | Bool-based | Sentinel | Savings |
|------|------------|----------|---------|
| `optional<int*>` | 16 bytes | 8 bytes | 50% |
| `optional<void*>` | 16 bytes | 8 bytes | 50% |
| `optional<int32_t>` | 8 bytes | 4 bytes | 50% |
| `optional<int64_t>` | 16 bytes | 8 bytes | 50% |
| `optional<size_t>` | 16 bytes | 8 bytes | 50% |
| `optional<FileHandle>` | 8 bytes | 4 bytes | 50% |
| `optional<ByteEnum>` | 2 bytes | 1 byte | 50% |

**Large-Scale Impact:**

```
10,000 elements (vector<optional<int*>>):
  Bool-based: 156.2 KB
  Sentinel:    78.1 KB
  Savings:     78.1 KB (50%)

1 million optional<int*>:
  Bool-based: 15.26 MB
  Sentinel:    7.63 MB
  Savings:     7.63 MB (50%)

1 billion optional<int32_t>:
  Bool-based:  7.45 GB
  Sentinel:    3.73 GB
  Savings:     3.73 GB (50%)
```

**Cache Efficiency:**

```
64-byte cache line capacity:
  optional<int*>:     4 items → 8 items (100% more)
  optional<int32_t>:  8 items → 16 items (100% more)
```

### Runtime Performance (1M operations)

**Construction:**
- Bool-based: 0.22 ns/op
- Sentinel: 0.10 ns/op
- **Sentinel is 2× faster**

**has_value() Check:**
- Bool-based: 0.21 ns/op
- Sentinel: 0.21 ns/op
- **Identical**

**Value Access:**
- Both: < 0.3 ns/op
- **Identical**

**Cache Effects (100K sequential):**
- Sentinel: **3.07× faster**
- Due to better cache utilization

### Compiler Verification

✅ **GCC 15.2.0** - 51/51 tests passing, 0 warnings
✅ **Clang 18.1.3** - 51/51 tests passing, 0 warnings

Run `make bench` to see detailed benchmarks on your system.

📊 **See [BENCHMARK_RESULTS.md](BENCHMARK_RESULTS.md) for complete benchmark analysis**

## Reference Implementation

The reference implementation in `reference-impl/optional.hpp` is:

- ✅ **Feature complete** - All std::optional operations
- ✅ **C++23 compatible** - Uses concepts, requires clauses, constexpr improvements
- ✅ **Constexpr enabled** - Works in constexpr contexts
- ✅ **Exception safe** - Proper RAII and strong exception guarantee
- ✅ **Extensively tested** - Comprehensive test suite
- ✅ **Production quality** - Ready for real-world use

## Documentation

- **[proposal.md](proposal.md)** - Full WG21 standards proposal
- **[supplementary/abi_analysis.md](supplementary/abi_analysis.md)** - Detailed ABI compatibility analysis
- **[supplementary/migration_guide.md](supplementary/migration_guide.md)** - How to adopt in your codebase

## Frequently Asked Questions

### Is this backwards compatible?

**Yes!** Existing `std::optional<T>` code works unchanged. The sentinel-based version is opt-in via the second template parameter.

### Does this change std::optional's API?

**No!** The API is identical. This is purely a storage optimization.

### What about ABI compatibility?

`optional<T>` and `optional<T, S>` are different types (like `vector<int, Alloc1>` vs `vector<int, Alloc2>`). Different types can have different layouts - this is normal and safe. See [supplementary/abi_analysis.md](supplementary/abi_analysis.md).

### What if my type has no spare value?

Use regular `std::optional<T>` (the default). The sentinel-based version is for types that *do* have spare values.

### Can I use this today?

Yes! The reference implementation in `reference-impl/optional.hpp` is ready to use. Just `#include` it.

### Will this be in the C++ standard?

This is a proposal. Its inclusion depends on WG21 review and approval.

## Contributing

This is a standards proposal. Feedback welcome:

- Issues with the design
- Additional use cases
- Benchmark results on your platform
- Documentation improvements

## License

This proposal and reference implementation are provided for standardization purposes.

## Author

[Author information would go here]

## References

- Current std::optional: [optional.optional] in C++17/20/23
- Rust's Option niche optimization: https://doc.rust-lang.org/std/option/
- Zero-overhead principle: Stroustrup, "Design and Evolution of C++"

---

**Status:** Draft Proposal (P????R0)
**Target:** C++29
**Requires:** C++20 (auto non-type template parameters)
**Implementation:** C++23 (reference implementation uses constexpr enhancements)
