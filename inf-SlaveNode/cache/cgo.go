package cache

import "github.com/yoitsuholo/relax/inf-SlaveNode/cache/cgoCache"

// NewCGOCache creates a new CGO-based cache instance
// This is a convenience function to create a high-performance C++ backed cache
//
// Parameters:
//   - maxBuddies: Maximum number of buddy allocators (affects memory pools)
//
// Returns:
//   - Cache: A cache instance implementing the Cache interface
//   - error: Any error that occurred during creation
//
// Example:
//
//	cache, err := cache.NewCGOCache(32)
//	if err != nil {
//	    log.Fatal(err)
//	}
//	defer cache.Close()
func NewCGOCache(maxBuddies uint) (Cache, error) {
	return cgoCache.NewCGOCache(maxBuddies)
}
