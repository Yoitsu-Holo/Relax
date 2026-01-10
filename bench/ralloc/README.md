# RAlloc Benchmarks

Performance benchmarks for the RAlloc unified memory allocator.

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Running Benchmarks

### Basic Performance Test
```bash
./bench_ralloc
```

This tests:
- Small allocations (64B - Slab allocator)
- Medium allocations (8KiB - Buddy system)
- Large allocations (512KiB - malloc fallback)
- Mixed size allocations

### Profiling Version
```bash
./profile_ralloc
```

Simpler version suitable for profiling with `perf` or `valgrind`.

## Profiling with perf

```bash
# Record performance data
perf record -g ./profile_ralloc

# View the report
perf report

# Generate flamegraph
perf script | stackcollapse-perf.pl | flamegraph.pl > ralloc_flame.svg
```

## Profiling with Valgrind

### Cachegrind (cache performance)
```bash
valgrind --tool=cachegrind ./profile_ralloc
cg_annotate cachegrind.out.<pid>
```

### Callgrind (call graph)
```bash
valgrind --tool=callgrind ./profile_ralloc
kcachegrind callgrind.out.<pid>
```

## Expected Results

RAlloc should show:
- **Slab range (16B-4KiB)**: 2-5x faster than malloc
- **Buddy range (4KiB-256KiB)**: 1.5-3x faster than malloc
- **Large range (>256KiB)**: Similar to malloc (direct fallback)
- **Mixed workload**: Overall 2-4x faster than malloc

## Build Options

### Release build (maximum performance)
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

### RelWithDebInfo (profiling with debug symbols)
```bash
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make
```
