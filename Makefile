# Makefile for Sentinel-Based Optional Proposal

CXX := g++
CXXFLAGS := -std=c++23 -Wall -Wextra -O2
CXXFLAGS_DEBUG := -std=c++23 -Wall -Wextra -g -O0

# Targets
EXAMPLES := examples
TESTS := tests
MEMORY_BENCH := memory_bench
PERF_BENCH := perf_bench

# Source files
IMPL := reference-impl/optional.hpp
EXAMPLES_SRC := reference-impl/examples.cpp
TESTS_SRC := reference-impl/tests.cpp
MEMORY_SRC := benchmarks/memory_usage.cpp
PERF_SRC := benchmarks/performance.cpp

# Markdown sources and PDF outputs
MD_SOURCES := proposal.md README.md SUMMARY.md supplementary/migration_guide.md supplementary/abi_analysis.md
PDF_OUTPUTS := $(MD_SOURCES:.md=.pdf)

.PHONY: all clean run test bench help pdf

# Default target
all: $(EXAMPLES) $(TESTS) $(MEMORY_BENCH) $(PERF_BENCH)
	@echo ""
	@echo "Build complete! Run 'make help' for available commands."

# Build examples
$(EXAMPLES): $(EXAMPLES_SRC) $(IMPL)
	@echo "Building examples..."
	$(CXX) $(CXXFLAGS) $(EXAMPLES_SRC) -o $(EXAMPLES)

# Build tests
$(TESTS): $(TESTS_SRC) $(IMPL)
	@echo "Building tests..."
	$(CXX) $(CXXFLAGS) $(TESTS_SRC) -o $(TESTS)

# Build memory benchmark
$(MEMORY_BENCH): $(MEMORY_SRC) $(IMPL)
	@echo "Building memory benchmark..."
	$(CXX) $(CXXFLAGS) $(MEMORY_SRC) -o $(MEMORY_BENCH)

# Build performance benchmark
$(PERF_BENCH): $(PERF_SRC) $(IMPL)
	@echo "Building performance benchmark..."
	$(CXX) $(CXXFLAGS) $(PERF_SRC) -o $(PERF_BENCH)

# Run examples
run: $(EXAMPLES)
	@echo ""
	@echo "Running examples..."
	@echo "===================="
	@./$(EXAMPLES)

# Run tests
test: $(TESTS)
	@echo ""
	@echo "Running tests..."
	@echo "================"
	@./$(TESTS)

# Run benchmarks
bench: $(MEMORY_BENCH) $(PERF_BENCH)
	@echo ""
	@echo "Running memory benchmark..."
	@echo "==========================="
	@./$(MEMORY_BENCH)
	@echo ""
	@echo ""
	@echo "Running performance benchmark..."
	@echo "================================"
	@./$(PERF_BENCH)

# Generate PDFs from markdown
pdf: $(PDF_OUTPUTS)
	@echo ""
	@echo "PDF generation complete."

%.pdf: %.md
	@echo "Generating $@..."
	pandoc $< -o $@ --pdf-engine=xelatex

# Run everything
check: test run bench

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(EXAMPLES) $(TESTS) $(MEMORY_BENCH) $(PERF_BENCH) $(PDF_OUTPUTS)
	@echo "Clean complete."

# Debug builds
debug-examples: $(EXAMPLES_SRC) $(IMPL)
	@echo "Building examples (debug)..."
	$(CXX) $(CXXFLAGS_DEBUG) $(EXAMPLES_SRC) -o $(EXAMPLES)

debug-tests: $(TESTS_SRC) $(IMPL)
	@echo "Building tests (debug)..."
	$(CXX) $(CXXFLAGS_DEBUG) $(TESTS_SRC) -o $(TESTS)

# Compiler variations
clang:
	@echo "Building with clang..."
	$(MAKE) CXX=clang++-18 all

gcc:
	@echo "Building with gcc..."
	$(MAKE) CXX=g++ all

msvc: CXX := cl
msvc: CXXFLAGS := /std:c++20 /EHsc /O2
msvc: CXXFLAGS_DEBUG := /std:c++20 /EHsc /Zi /Od
msvc: all

# Help
help:
	@echo "Sentinel-Based Optional - Build System"
	@echo "======================================"
	@echo ""
	@echo "Available targets:"
	@echo "  make all          - Build all executables (default)"
	@echo "  make examples     - Build examples only"
	@echo "  make tests        - Build tests only"
	@echo "  make memory_bench - Build memory benchmark only"
	@echo "  make perf_bench   - Build performance benchmark only"
	@echo ""
	@echo "Run targets:"
	@echo "  make run          - Run examples"
	@echo "  make test         - Run tests"
	@echo "  make bench        - Run all benchmarks"
	@echo "  make check        - Run tests + examples + benchmarks"
	@echo ""
	@echo "Document targets:"
	@echo "  make pdf          - Generate PDFs from markdown (requires pandoc)"
	@echo ""
	@echo "Utility targets:"
	@echo "  make clean        - Remove build artifacts and generated PDFs"
	@echo "  make clang        - Build with clang-18"
	@echo "  make gcc          - Build with g++ (default)"
	@echo "  make debug-*      - Build debug versions"
	@echo "  make help         - Show this help"
	@echo ""
	@echo "Requirements:"
	@echo "  - C++23 compatible compiler (GCC 13+, Clang 17+, MSVC 19.36+)"
	@echo "  - Standard library with C++23 support"
	@echo ""
	@echo "Quick start:"
	@echo "  make all && make check"
