# RAlloc Tests

Unit tests for the RAlloc unified memory allocator.

## Quick Start

### Run Tests
```bash
cd /home/yoitsuholo/Code/Relax/build
./test/ralloc/test_ralloc
```

### Run with Coverage
```bash
cd /home/yoitsuholo/Code/Relax/build
make all_coverage
```

Coverage reports will be generated in:
- `build/coverage/lcov/ralloc/index.html` (LCOV format)
- `build/coverage/gcovr/index.html` (gcovr format)

## Test Results

✅ **23/23 tests passing (100%)**
📊 **93% code coverage**

### Test Categories

1. **Basic Allocations** - Small, medium, and large allocations
2. **Size Classes** - All slab sizes and buddy orders
3. **Mixed Allocations** - Various sizes in random order
4. **Edge Cases** - Zero size, null pointers, boundary conditions
5. **Statistics** - Allocator counts and memory tracking
6. **Stress Tests** - Multiple allocations, random sizes
7. **Data Integrity** - Memory corruption checks
8. **Global Interface** - Testing ralloc_malloc/ralloc_free
9. **Allocator Management** - Multiple slab/buddy allocators

## Coverage Report

```
File                                       Lines    Exec  Cover   Missing
------------------------------------------------------------------------------
cache-Kernel/ralloc/ralloc.cpp               121     113    93%   225-228,362,367-368,370
------------------------------------------------------------------------------
```

**Uncovered lines**: Mostly error handling paths that are difficult to trigger in normal testing.

## Building

Tests are automatically built when you build the main project with tests enabled:

```bash
cd /home/yoitsuholo/Code/Relax
mkdir build && cd build
cmake -DENABLE_COVERAGE=ON -DBUILD_TESTS=ON ..
make
```

## Test Details

See [REPORT.md](REPORT.md) for:
- Detailed test results
- Performance benchmarks
- Coverage analysis
- Recommendations

## Dependencies

- Google Test (GTest) 1.17.0+
- gcovr 8.4+ (for coverage reports)
- lcov (for HTML coverage reports)
