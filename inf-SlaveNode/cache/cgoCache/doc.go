// Package cgoCache provides a high-performance cache implementation
// using C++ TypeDriver via CGO.
//
// This package implements the cache.Cache interface by wrapping the
// C++ TypeDriver library, allowing Go code to leverage the high-performance
// C++ cache implementation.
//
// Usage:
//
//	cache, err := cgoCache.NewCGOCache(16) // 16 max buddy allocators
//	if err != nil {
//	    log.Fatal(err)
//	}
//	defer cache.Close()
//
//	// Use the cache
//	cache.Set("key", "value")
//	value, exists, err := cache.Get("key")
//
// Build Requirements:
//
// The C++ library must be built first using CMake:
//
//	cd /path/to/Relax
//	mkdir -p build && cd build
//	cmake ..
//	make
//
// Environment Variables:
//
// You may need to set LD_LIBRARY_PATH to include the library path:
//
//	export LD_LIBRARY_PATH=/path/to/Relax/build/cache-Interface:$LD_LIBRARY_PATH
//
// Limitations:
//
// - List operations (LPush, LPop, etc.) are not implemented as they are
//   not available in the C interface.
package cgoCache
