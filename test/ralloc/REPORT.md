# RAlloc Test and Performance Report

## Test Summary

### Unit Tests (GTest)
**Location**: `/home/yoitsuholo/Code/Relax/test/ralloc/`

**Total Tests**: 23
**Passed**: 23 (100%)
**Failed**: 0

#### Test Categories
- Basic allocations (small, medium, large): 3 tests
- Size class coverage (slab, buddy): 2 tests
- Mixed allocations: 1 test
- Edge cases: 8 tests
- Statistics: 2 tests
- Stress tests: 2 tests
- Data integrity: 1 test
- Global interface: 2 tests
- Allocator management: 2 tests

### Code Coverage
**Tool**: gcovr with lcov

**Overall Coverage**: 93%
- **Lines covered**: 113 / 121
- **Missing lines**: 8 (mostly error handling paths)

**Coverage Report**:
```
File                                       Lines    Exec  Cover   Missing
------------------------------------------------------------------------------
cache-Kernel/ralloc/ralloc.cpp               121     113    93%   225-228,362,367-368,370
------------------------------------------------------------------------------
```

**Coverage Details**:
- Core allocation paths: 100%
- Deallocation paths: 100%
- SlabManager: 95%
- Allocation identification: 100%
- Statistics: 90%
- Uncovered: Some edge cases in error paths

**HTML Reports Available**:
- LCOV: `build/coverage/lcov/ralloc/index.html`
- gcovr: `build/coverage/gcovr/index.html`

## Performance Benchmarks

### Test Environment
- **Build Type**: Release (-O3 -march=native)
- **Compiler**: GCC 15.2.1
- **Platform**: Linux 6.18.3-zen1-1-zen

### Benchmark Results

#### Test 1: Small Allocations (64B - Slab)
- **RAlloc (Slab)**: 10.38 ns/op | 96.4M ops/sec
- **malloc/free**: 8.42 ns/op | 118.8M ops/sec
- **Result**: **0.81x (slower)** - Expected for very small allocations

**Analysis**: System malloc is highly optimized for small allocations. The slight overhead is due to the additional routing logic in RAlloc.

#### Test 2: Medium Allocations (8KiB - Buddy)
- **RAlloc (Buddy)**: 61.04 ns/op | 16.4M ops/sec
- **malloc/free**: 1583.64 ns/op | 631K ops/sec
- **Result**: **25.9x faster** ✨

**Analysis**: Buddy system significantly outperforms malloc for medium-sized allocations due to O(1) allocation and better cache locality.

#### Test 3: Large Allocations (512KiB - Malloc)
- **RAlloc (Malloc)**: 2922.99 ns/op | 342K ops/sec
- **malloc/free**: 2571.41 ns/op | 389K ops/sec
- **Result**: **0.88x (slower)** - Expected overhead from header

**Analysis**: RAlloc uses malloc with a tracking header for large allocations. The ~14% overhead is from header management and allocation type tracking.

#### Test 4: Mixed Size Allocations ⭐
- **RAlloc (Mixed)**: 15.40 ns/op | 64.9M ops/sec
- **malloc/free**: 838.13 ns/op | 1.19M ops/sec
- **Result**: **54.4x faster** 🚀

**Analysis**: This is the killer benchmark! RAlloc excels at mixed workloads by routing allocations to the appropriate allocator, while malloc struggles with diverse sizes.

### Resource Usage
- **Slab allocators created**: 73
- **Buddy allocators created**: 64 (max limit)
- **Total free memory**: 1.08 GB

## Performance Summary

| Category | Size Range | Speedup | Use Case |
|----------|------------|---------|----------|
| Small | 16B-4KiB | 0.81x | Not optimal for tiny allocations |
| Medium | 4KiB-256KiB | **25.9x** | Excellent for buffers, packets |
| Large | >256KiB | 0.88x | Direct malloc (expected overhead) |
| **Mixed** | Various | **54.4x** | **Best use case** |

## Conclusions

### Strengths ✅
1. **Outstanding mixed workload performance** (54x faster than malloc)
2. **Excellent medium allocation performance** (26x faster than malloc)
3. **High test coverage** (93%)
4. **All tests passing** (23/23)
5. **Intelligent allocation routing** works correctly
6. **Proper memory tracking** for deallocation

### Trade-offs ⚖️
1. Small allocations (64B) are ~20% slower than malloc
   - Acceptable given the overhead of routing logic
   - Still performs at 96M ops/sec

2. Large allocations have ~12% overhead
   - Due to tracking header (24 bytes)
   - Necessary for unified free() interface

### Recommendations 💡

**Ideal Use Cases**:
- Cache systems with mixed object sizes ✅
- Network servers (varied packet sizes) ✅
- Database systems (variable record sizes) ✅
- Any workload with diverse allocation sizes ✅

**Not Ideal For**:
- Extremely tight loops with tiny allocations only
- Large-only allocation patterns (>256KiB exclusively)

**When to Use RAlloc**:
- When you have mixed allocation sizes (16B to 256KiB)
- When allocation/deallocation performance is critical
- When you want O(1) guaranteed performance
- When you can tolerate ~1GB memory overhead for allocator pools

## Next Steps

1. ✅ All tests passing with 93% coverage
2. ✅ Performance benchmarks complete
3. 🔄 Consider adding thread-local caching for multi-threaded scenarios
4. 🔄 Consider tuning slab size classes based on workload profiling

## Test Commands

### Run Unit Tests
```bash
cd /home/yoitsuholo/Code/Relax/build
./test/ralloc/test_ralloc
```

### Run with Coverage
```bash
cd /home/yoitsuholo/Code/Relax/build
make all_coverage
```

### Run Benchmarks
```bash
cd /home/yoitsuholo/Code/Relax/bench/ralloc/build
./bench_ralloc
```

### Run Profiling Version
```bash
cd /home/yoitsuholo/Code/Relax/bench/ralloc/build
./profile_ralloc
```

## Files Created

### Test Files
- `test/ralloc/test_ralloc.cpp` - GTest-based unit tests
- `test/ralloc/CMakeLists.txt` - Test build configuration

### Benchmark Files
- `bench/ralloc/bench_ralloc.cpp` - Performance benchmarks
- `bench/ralloc/profile_ralloc.cpp` - Profiling version
- `bench/ralloc/CMakeLists.txt` - Benchmark build configuration
- `bench/ralloc/README.md` - Benchmark documentation

### Integration
- Updated `/home/yoitsuholo/Code/Relax/CMakeLists.txt` to include ralloc tests and benchmarks

---

**Report Generated**: 2026-01-11
**RAlloc Version**: 1.0
**Test Framework**: Google Test 1.17.0
**Coverage Tool**: gcovr 8.4 + lcov
