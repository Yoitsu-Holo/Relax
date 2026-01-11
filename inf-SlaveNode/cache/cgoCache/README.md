# CGO Cache - High-Performance Cache Implementation

This package provides a Go wrapper around the C++ TypeDriver cache implementation using CGO.

## Features

- **High Performance**: Leverages C++ implementation for better performance
- **KV Operations**: Full support for key-value operations (Set, Get, Del, Exists)
- **Hash Operations**: Complete hash operations including single field and batch operations
- **Set Operations**: Full set operations support (Add, Remove, Members, etc.)
- **Memory Safe**: Proper memory management with defer cleanup

## Architecture

```
┌─────────────────┐
│  Go Service     │
│  (gRPC/HTTP)    │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  cgoCache       │  ← This package
│  (Go CGO)       │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ type_driver_c.h │  (C Interface)
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  C++ TypeDriver │  (High-performance implementation)
│  cache-Interface│
└─────────────────┘
```

## Building

### Prerequisites

1. Build the C++ library first:

```bash
cd /path/to/Relax
mkdir -p build && cd build
cmake -DBUILD_SHARED_LIBS=ON ..
make
```

This will create:
- `build/cache-Interface/libcache_interface.so` (shared library)
- `build/cache-Interface/libcache_interface.a` (static library)

### Build Go Package

The package uses CGO directives to link against the C++ library:

```go
/*
#cgo CFLAGS: -I${SRCDIR}/../../../cache-Interface
#cgo LDFLAGS: -L${SRCDIR}/../../../build/cache-Interface -lcache_interface -lstdc++
*/
```

To build:

```bash
cd inf-SlaveNode/cache/cgoCache
go build
```

### Runtime

Set `LD_LIBRARY_PATH` before running:

```bash
export LD_LIBRARY_PATH=/path/to/Relax/build/cache-Interface:$LD_LIBRARY_PATH
go run main.go
```

Or use `ldconfig` to add the library path permanently (requires root):

```bash
sudo echo "/path/to/Relax/build/cache-Interface" > /etc/ld.so.conf.d/relax.conf
sudo ldconfig
```

## Usage

### Basic Example

```go
package main

import (
    "log"
    "github.com/yoitsuholo/relax/inf-SlaveNode/cache/cgoCache"
)

func main() {
    // Create cache with 16 max buddy allocators
    cache, err := cgoCache.NewCGOCache(16)
    if err != nil {
        log.Fatal(err)
    }
    defer cache.Close()

    // KV operations
    cache.Set("user:1", "John Doe")
    value, exists, _ := cache.Get("user:1")
    if exists {
        log.Printf("Found: %s", value)
    }

    // Hash operations
    cache.HSet("user:profile:1", "name", "John")
    cache.HSet("user:profile:1", "age", "30")
    profile, _ := cache.HGetAll("user:profile:1")

    // Set operations
    cache.SAdd("tags", "golang")
    cache.SAdd("tags", "cache")
    members, _ := cache.SMembers("tags")
}
```

### Integration with CacheServer

```go
package main

import (
    "github.com/yoitsuholo/relax/inf-SlaveNode/cache/cgoCache"
    "github.com/yoitsuholo/relax/inf-SlaveNode/service"
)

func main() {
    // Create CGO cache
    cache, err := cgoCache.NewCGOCache(32)
    if err != nil {
        log.Fatal(err)
    }
    defer cache.Close()

    // Use with CacheServer
    cacheServer := service.NewCacheServerWithCache(cache)

    // Start gRPC server with cacheServer...
}
```

## API Reference

### Cache Creation

- `NewCGOCache(maxBuddies uint) (*CGOCache, error)`: Create new cache instance
  - `maxBuddies`: Maximum number of buddy allocators (affects memory pools)

### KV Operations

- `Set(key, value string) error`
- `Get(key string) (string, bool, error)`
- `Del(key string) (int, error)`
- `Exists(key string) (bool, error)`

### Hash Operations

- `HSet(key, field, value string) (bool, error)`
- `HGet(key, field string) (string, bool, error)`
- `HDel(key, field string) (int, error)`
- `HMSet(key string, fields map[string]string) (int, error)`
- `HMGet(key string, fields []string) (map[string]string, error)`
- `HGetAll(key string) (map[string]string, error)`
- `HLen(key string) (int, error)`
- `HExists(key, field string) (bool, error)`

### Set Operations

- `SAdd(key, member string) (bool, error)`
- `SMembers(key string) ([]string, error)`
- `SRem(key, member string) (int, error)`
- `SIsMember(key, member string) (bool, error)`
- `SCard(key string) (int, error)`

### Resource Management

- `Close() error`: Release all resources

## Performance Considerations

1. **Buddy Allocators**: The `maxBuddies` parameter affects memory allocation performance. Higher values allow more concurrent allocations but use more memory.

2. **Hash Function**: The current implementation uses a simple hash function. For production use, consider using a better hash function for better distribution.

3. **Memory Management**: All C strings are properly freed using `defer C.free()` to prevent memory leaks.

## Limitations

- **List Operations**: Not implemented in the C interface (LPush, LPop, RPush, RPop, LRange, etc.)
- **Thread Safety**: Depends on the underlying C++ implementation's thread safety
- **CGO Overhead**: Some overhead from Go↔C boundary crossing

## Troubleshooting

### Library Not Found

```
error while loading shared libraries: libcache_interface.so: cannot open shared object file
```

**Solution**: Set `LD_LIBRARY_PATH`:
```bash
export LD_LIBRARY_PATH=/path/to/Relax/build/cache-Interface:$LD_LIBRARY_PATH
```

### Compilation Errors

```
fatal error: type_driver_c.h: No such file or directory
```

**Solution**: Ensure the C++ library is built and the paths in CGO directives are correct.

### Undefined Reference Errors

```
undefined reference to `type_driver_create'
```

**Solution**: Make sure all required C++ libraries are linked in the CGO LDFLAGS.

## Testing

Tests are located in `test/inf-slave-server/cgoCache/`.

Run tests:

```bash
cd test/inf-slave-server/cgoCache
export LD_LIBRARY_PATH=/path/to/Relax/build/cache-Interface:$LD_LIBRARY_PATH
go test -v
```

Run with race detector:

```bash
go test -race -v
```

## Examples

Example code is located in `example/inf-slave-server/cgoCache/`.

Run examples:

```bash
cd example/inf-slave-server/cgoCache
export LD_LIBRARY_PATH=/path/to/Relax/build/cache-Interface:$LD_LIBRARY_PATH
go test -v
```

## License

Part of the Relax project.
