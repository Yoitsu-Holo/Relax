package cgoCache_test

import (
	"fmt"
	"log"

	"github.com/yoitsuholo/relax/inf-SlaveNode/cache/cgoCache"
)

// Example_basic demonstrates basic cache operations
func Example_basic() {
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
		fmt.Printf("User: %s\n", value)
	}

	// Check existence
	exists, _ = cache.Exists("user:1")
	fmt.Printf("Exists: %v\n", exists)

	// Delete
	count, _ := cache.Del("user:1")
	fmt.Printf("Deleted: %d\n", count)

	// Output:
	// User: John Doe
	// Exists: true
	// Deleted: 1
}

// Example_hash demonstrates hash operations
func Example_hash() {
	cache, err := cgoCache.NewCGOCache(16)
	if err != nil {
		log.Fatal(err)
	}
	defer cache.Close()

	// Set individual fields
	cache.HSet("user:profile:1", "name", "John")
	cache.HSet("user:profile:1", "age", "30")
	cache.HSet("user:profile:1", "city", "NYC")

	// Get single field
	name, exists, _ := cache.HGet("user:profile:1", "name")
	if exists {
		fmt.Printf("Name: %s\n", name)
	}

	// Get all fields
	profile, _ := cache.HGetAll("user:profile:1")
	fmt.Printf("Fields count: %d\n", len(profile))

	// Get hash length
	length, _ := cache.HLen("user:profile:1")
	fmt.Printf("Length: %d\n", length)

	// Check field existence
	exists, _ = cache.HExists("user:profile:1", "name")
	fmt.Printf("Name exists: %v\n", exists)

	// Output:
	// Name: John
	// Fields count: 3
	// Length: 3
	// Name exists: true
}

// Example_hashBatch demonstrates batch hash operations
func Example_hashBatch() {
	cache, err := cgoCache.NewCGOCache(16)
	if err != nil {
		log.Fatal(err)
	}
	defer cache.Close()

	// Batch set multiple fields
	fields := map[string]string{
		"name":  "Alice",
		"email": "alice@example.com",
		"phone": "+1234567890",
		"city":  "San Francisco",
	}

	count, _ := cache.HMSet("user:contact:2", fields)
	fmt.Printf("Set %d fields\n", count)

	// Batch get specific fields
	fieldNames := []string{"name", "email", "city"}
	result, _ := cache.HMGet("user:contact:2", fieldNames)
	fmt.Printf("Retrieved %d fields\n", len(result))
	fmt.Printf("Name: %s\n", result["name"])

	// Output:
	// Set 4 fields
	// Retrieved 3 fields
	// Name: Alice
}

// Example_set demonstrates set operations
func Example_set() {
	cache, err := cgoCache.NewCGOCache(16)
	if err != nil {
		log.Fatal(err)
	}
	defer cache.Close()

	// Add members to set
	cache.SAdd("tags", "golang")
	cache.SAdd("tags", "cache")
	cache.SAdd("tags", "performance")
	cache.SAdd("tags", "cgo")

	// Get set cardinality
	count, _ := cache.SCard("tags")
	fmt.Printf("Tag count: %d\n", count)

	// Check membership
	isMember, _ := cache.SIsMember("tags", "golang")
	fmt.Printf("Has golang: %v\n", isMember)

	// Get all members
	members, _ := cache.SMembers("tags")
	fmt.Printf("Total members: %d\n", len(members))

	// Remove a member
	removed, _ := cache.SRem("tags", "cgo")
	fmt.Printf("Removed: %d\n", removed)

	// Check after removal
	count, _ = cache.SCard("tags")
	fmt.Printf("Tag count after removal: %d\n", count)

	// Output:
	// Tag count: 4
	// Has golang: true
	// Total members: 4
	// Removed: 1
	// Tag count after removal: 3
}

// Example_cacheServerIntegration demonstrates integration with CacheServer
func Example_cacheServerIntegration() {
	// This is a simplified example showing how to integrate with CacheServer
	// In a real application, you would use this with the service package

	cache, err := cgoCache.NewCGOCache(32)
	if err != nil {
		log.Fatal(err)
	}
	defer cache.Close()

	// The cache implements the cache.Cache interface
	// so it can be used with service.NewCacheServerWithCache(cache)

	// Example operations
	cache.Set("session:abc123", "user_id=42")
	cache.HMSet("user:42", map[string]string{
		"username": "john_doe",
		"email":    "john@example.com",
	})
	cache.SAdd("online_users", "42")

	fmt.Println("Cache ready for service integration")

	// Output:
	// Cache ready for service integration
}
