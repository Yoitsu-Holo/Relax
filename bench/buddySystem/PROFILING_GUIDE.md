# Buddy System Profiling Guide

This guide explains how to profile the Buddy System allocator and generate flame graphs to identify performance bottlenecks.

## Prerequisites

Install required tools:

```bash
# On Ubuntu/Debian
sudo apt-get install linux-tools-common linux-tools-generic

# Install FlameGraph tools
git clone https://github.com/brendangregg/FlameGraph
cd FlameGraph
# Add to PATH or note the directory location
```

## Building for Profiling

Build with debug symbols and frame pointers:

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make profile_buddy
```

The `-fno-omit-frame-pointer` flag is crucial for accurate stack traces.

## Profiling with perf

### Basic Profiling

```bash
# Record with 99 Hz sampling rate
perf record -F 99 -g ./profile_buddy

# View the report
perf report
```

### Generate Flame Graph

```bash
# Record performance data
perf record -F 99 -g -o buddy.data ./profile_buddy

# Generate flame graph
perf script -i buddy.data | \
    /path/to/FlameGraph/stackcollapse-perf.pl | \
    /path/to/FlameGraph/flamegraph.pl > buddy_flamegraph.svg

# Open in browser
firefox buddy_flamegraph.svg
```

### Different Block Sizes

Profile specific allocation patterns by modifying `profile_buddy.cpp`:

```cpp
// Focus on small allocations
size_t test_sizes[] = {4096};

// Focus on large allocations
size_t test_sizes[] = {262144};

// Mixed workload
size_t test_sizes[] = {4096, 32768, 262144};
```

## Analyzing Flame Graphs

### Key Areas to Examine

1. **allocate_from_order()** - Main allocation path
   - Look for time spent in find_free_block()
   - Check split_block() overhead
   - Examine loop unrolling effectiveness

2. **deallocate()** - Deallocation path
   - Time in try_merge_buddy()
   - Link list operations (add_to_free_list, remove_from_free_list)

3. **Bitmap Operations**
   - __builtin_ctzll usage
   - __builtin_popcountll calls
   - Bitmap search patterns

### Interpretation

**Wide bars** = Functions consuming significant CPU time
**Tall stacks** = Deep call chains (may indicate overhead)
**Flat profiles** = Efficient execution with minimal call overhead

## Advanced Profiling

### CPU Cache Analysis

```bash
# L1 cache misses
perf stat -e L1-dcache-load-misses,L1-dcache-loads ./profile_buddy

# Last-level cache analysis
perf stat -e LLC-load-misses,LLC-loads ./profile_buddy
```

### Branch Prediction

```bash
# Branch mispredictions
perf stat -e branch-misses,branches ./profile_buddy
```

### Memory Bandwidth

```bash
# Memory operations
perf stat -e cycles,instructions,cache-references,cache-misses ./profile_buddy
```

## Comparing Against malloc

### Profile malloc

```bash
# Create a malloc-only version
cat > profile_malloc.cpp << 'EOF'
#include <iostream>
#include <cstdlib>

int main() {
    const int TOTAL_OPS = 100000000;
    const int BATCH_SIZE = 10000;
    void* ptrs[BATCH_SIZE];

    for (int batch = 0; batch < TOTAL_OPS / BATCH_SIZE; ++batch) {
        for (int i = 0; i < BATCH_SIZE; ++i) {
            ptrs[i] = malloc(4096);
        }
        for (int i = 0; i < BATCH_SIZE; ++i) {
            free(ptrs[i]);
        }
    }
    return 0;
}
EOF

g++ -O2 -g -fno-omit-frame-pointer profile_malloc.cpp -o profile_malloc
perf record -F 99 -g ./profile_malloc
perf script | stackcollapse-perf.pl | flamegraph.pl > malloc_flamegraph.svg
```

### Side-by-Side Comparison

Use `perf diff` to compare profiles:

```bash
# Record both versions
perf record -o buddy.data ./profile_buddy
perf record -o malloc.data ./profile_malloc

# Compare
perf diff buddy.data malloc.data
```

## Optimization Tips

Based on flame graph analysis:

1. **Hot Path Optimization**
   - If `find_free_block()` is hot, consider optimizing bitmap search
   - If list operations are slow, check memory access patterns

2. **Loop Unrolling**
   - Verify switch cases are being optimized
   - Check for any remaining dynamic loops

3. **Cache Optimization**
   - If high cache miss rate, consider data structure layout
   - Check alignment of bitmap and list structures

4. **Branch Prediction**
   - High branch misses in allocation path may indicate need for `__builtin_expect`

## Troubleshooting

### No Stack Traces

```bash
# Ensure frame pointers
CXXFLAGS="-fno-omit-frame-pointer" make profile_buddy

# Try different unwinder
perf record --call-graph dwarf ./profile_buddy
```

### Low Sample Count

```bash
# Increase sampling frequency
perf record -F 999 -g ./profile_buddy

# Or increase iteration count in profile_buddy.cpp
```

### Permission Denied

```bash
# Temporary fix
sudo sysctl -w kernel.perf_event_paranoid=-1

# Or run with sudo
sudo perf record -F 99 -g ./profile_buddy
```

## Automated Profiling Script

```bash
#!/bin/bash
# profile_buddy.sh - Automated profiling and flame graph generation

set -e

BUILD_DIR="build"
FLAMEGRAPH_DIR="/path/to/FlameGraph"

echo "Building profiling binary..."
mkdir -p $BUILD_DIR && cd $BUILD_DIR
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo .. && make profile_buddy
cd ..

echo "Running perf record..."
perf record -F 99 -g -o buddy.data $BUILD_DIR/profile_buddy

echo "Generating flame graph..."
perf script -i buddy.data | \
    $FLAMEGRAPH_DIR/stackcollapse-perf.pl | \
    $FLAMEGRAPH_DIR/flamegraph.pl > buddy_flamegraph.svg

echo "Flame graph generated: buddy_flamegraph.svg"
echo "Open with: firefox buddy_flamegraph.svg"
```

## References

- [Flame Graphs](http://www.brendangregg.com/flamegraphs.html)
- [perf Examples](http://www.brendangregg.com/perf.html)
- [Linux perf Wiki](https://perf.wiki.kernel.org/)
