#include "../reference-impl/optional.hpp"
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <optional>
#include <vector>

using namespace slim;

// Helper to print table rows
void print_row(const char* type, size_t std_size, size_t slim_size) {
    double savings = 100.0 * (1.0 - static_cast<double>(slim_size) / std_size);
    std::cout << std::left << std::setw(40) << type
              << std::right << std::setw(12) << std_size
              << std::setw(12) << slim_size
              << std::setw(12) << std::fixed << std::setprecision(1) << savings << "%\n";
}

// Enum for testing
enum class FileHandle : int32_t {
    INVALID = -1,
    STDIN = 0,
    STDOUT = 1,
    STDERR = 2
};

enum class ByteEnum : int8_t {
    INVALID = 0,
    VALUE1 = 1,
    VALUE2 = 2,
    VALUE3 = 3
};

// Custom struct for testing
struct Handle {
    int32_t id;
    bool operator==(const Handle&) const = default;
};
constexpr Handle INVALID_HANDLE{-1};

int main() {
    std::cout << "optional: Memory Usage Benchmarks\n";
    std::cout << "=================================================\n\n";

    std::cout << "Testing on " << (sizeof(void*) == 8 ? "64-bit" : "32-bit") << " system\n\n";

    std::cout << std::left << std::setw(40) << "Type"
              << std::right << std::setw(12) << "std::optional"
              << std::setw(12) << "optional"
              << std::setw(12) << "Savings\n";
    std::cout << std::string(76, '-') << "\n";

    // Pointer types
    print_row("optional<int*>",
              sizeof(std::optional<int*>),
              sizeof(optional<int*, nullptr>));

    print_row("optional<void*>",
              sizeof(std::optional<void*>),
              sizeof(optional<void*, nullptr>));

    print_row("optional<const char*>",
              sizeof(std::optional<const char*>),
              sizeof(optional<const char*, nullptr>));

    // Integer types
    print_row("optional<int32_t>",
              sizeof(std::optional<int32_t>),
              sizeof(optional<int32_t, -1>));

    print_row("optional<int64_t>",
              sizeof(std::optional<int64_t>),
              sizeof(optional<int64_t, -1L>));

    print_row("optional<size_t>",
              sizeof(std::optional<size_t>),
              sizeof(optional<size_t, static_cast<size_t>(-1)>));

    // Enum types
    print_row("optional<FileHandle>",
              sizeof(std::optional<FileHandle>),
              sizeof(optional<FileHandle, FileHandle::INVALID>));

    print_row("optional<ByteEnum>",
              sizeof(std::optional<ByteEnum>),
              sizeof(optional<ByteEnum, ByteEnum::INVALID>));

    // Custom struct
    print_row("optional<Handle>",
              sizeof(std::optional<Handle>),
              sizeof(optional<Handle, INVALID_HANDLE>));

    std::cout << "\n";
    std::cout << "Container Memory Usage\n";
    std::cout << "======================\n\n";

    const size_t container_size = 10000;

    std::cout << "For a container of " << container_size << " elements:\n\n";

    // Vector of optional pointers
    {
        size_t std_total = container_size * sizeof(std::optional<int*>);
        size_t slim_total = container_size * sizeof(optional<int*, nullptr>);

        std::cout << "vector<optional<int*>>:\n";
        std::cout << "  std::optional:   " << std_total << " bytes ("
                  << (std_total / 1024.0) << " KB)\n";
        std::cout << "  optional:   " << slim_total << " bytes ("
                  << (slim_total / 1024.0) << " KB)\n";
        std::cout << "  Savings:         " << (std_total - slim_total) << " bytes ("
                  << ((std_total - slim_total) / 1024.0) << " KB, "
                  << (100.0 * (std_total - slim_total) / std_total) << "%)\n\n";
    }

    // Vector of optional int32_t
    {
        size_t std_total = container_size * sizeof(std::optional<int32_t>);
        size_t slim_total = container_size * sizeof(optional<int32_t, -1>);

        std::cout << "vector<optional<int32_t>>:\n";
        std::cout << "  std::optional:   " << std_total << " bytes ("
                  << (std_total / 1024.0) << " KB)\n";
        std::cout << "  optional:   " << slim_total << " bytes ("
                  << (slim_total / 1024.0) << " KB)\n";
        std::cout << "  Savings:         " << (std_total - slim_total) << " bytes ("
                  << ((std_total - slim_total) / 1024.0) << " KB, "
                  << (100.0 * (std_total - slim_total) / std_total) << "%)\n\n";
    }

    // Vector of optional enums
    {
        size_t std_total = container_size * sizeof(std::optional<FileHandle>);
        size_t slim_total = container_size * sizeof(optional<FileHandle, FileHandle::INVALID>);

        std::cout << "vector<optional<FileHandle>>:\n";
        std::cout << "  std::optional:   " << std_total << " bytes ("
                  << (std_total / 1024.0) << " KB)\n";
        std::cout << "  optional:   " << slim_total << " bytes ("
                  << (slim_total / 1024.0) << " KB)\n";
        std::cout << "  Savings:         " << (std_total - slim_total) << " bytes ("
                  << ((std_total - slim_total) / 1024.0) << " KB, "
                  << (100.0 * (std_total - slim_total) / std_total) << "%)\n\n";
    }

    std::cout << "\n";
    std::cout << "Large-Scale Impact\n";
    std::cout << "==================\n\n";

    const size_t million = 1000000;
    const size_t billion = 1000000000;

    // 1 million pointers
    {
        size_t std_total = million * sizeof(std::optional<int*>);
        size_t slim_total = million * sizeof(optional<int*, nullptr>);
        double std_mb = std_total / (1024.0 * 1024.0);
        double slim_mb = slim_total / (1024.0 * 1024.0);

        std::cout << "1 million optional<int*>:\n";
        std::cout << "  std::optional:   " << std::fixed << std::setprecision(2)
                  << std_mb << " MB\n";
        std::cout << "  optional:   " << slim_mb << " MB\n";
        std::cout << "  Savings:         " << (std_mb - slim_mb) << " MB\n\n";
    }

    // 1 billion int32_t
    {
        size_t std_total = billion * sizeof(std::optional<int32_t>);
        size_t slim_total = billion * sizeof(optional<int32_t, -1>);
        double std_gb = std_total / (1024.0 * 1024.0 * 1024.0);
        double slim_gb = slim_total / (1024.0 * 1024.0 * 1024.0);

        std::cout << "1 billion optional<int32_t>:\n";
        std::cout << "  std::optional:   " << std::fixed << std::setprecision(2)
                  << std_gb << " GB\n";
        std::cout << "  optional:   " << slim_gb << " GB\n";
        std::cout << "  Savings:         " << (std_gb - slim_gb) << " GB\n\n";
    }

    std::cout << "\n";
    std::cout << "Cache Line Analysis\n";
    std::cout << "===================\n\n";

    const size_t cache_line_size = 64;

    std::cout << "Assuming " << cache_line_size << " byte cache lines:\n\n";

    // Pointers
    {
        size_t std_per_line = cache_line_size / sizeof(std::optional<int*>);
        size_t slim_per_line = cache_line_size / sizeof(optional<int*, nullptr>);

        std::cout << "optional<int*> per cache line:\n";
        std::cout << "  std::optional:   " << std_per_line << "\n";
        std::cout << "  optional:   " << slim_per_line << "\n";
        std::cout << "  Improvement:     " << (100.0 * (slim_per_line - std_per_line) / std_per_line)
                  << "% more items per cache line\n\n";
    }

    // int32_t
    {
        size_t std_per_line = cache_line_size / sizeof(std::optional<int32_t>);
        size_t slim_per_line = cache_line_size / sizeof(optional<int32_t, -1>);

        std::cout << "optional<int32_t> per cache line:\n";
        std::cout << "  std::optional:   " << std_per_line << "\n";
        std::cout << "  optional:   " << slim_per_line << "\n";
        std::cout << "  Improvement:     " << (100.0 * (slim_per_line - std_per_line) / std_per_line)
                  << "% more items per cache line\n\n";
    }

    std::cout << "\n";
    std::cout << "Detailed Size Breakdown\n";
    std::cout << "=======================\n\n";

    // Show alignment and padding
    std::cout << "std::optional<int*>:\n";
    std::cout << "  total:          " << sizeof(std::optional<int*>) << " bytes\n";
    std::cout << "  (bool + padding + pointer)\n\n";

    std::cout << "optional<int*, nullptr>:\n";
    std::cout << "  sizeof(int*):   " << sizeof(int*) << " bytes\n";
    std::cout << "  total:          " << sizeof(optional<int*, nullptr>) << " bytes\n";
    std::cout << "  padding:        0 bytes (no overhead!)\n\n";

    std::cout << "std::optional<int32_t>:\n";
    std::cout << "  total:          " << sizeof(std::optional<int32_t>) << " bytes\n";
    std::cout << "  (bool + padding + int32_t)\n\n";

    std::cout << "optional<int32_t, -1>:\n";
    std::cout << "  sizeof(int32_t):" << sizeof(int32_t) << " bytes\n";
    std::cout << "  total:          " << sizeof(optional<int32_t, -1>) << " bytes\n";
    std::cout << "  padding:        0 bytes (no overhead!)\n\n";

    return 0;
}
