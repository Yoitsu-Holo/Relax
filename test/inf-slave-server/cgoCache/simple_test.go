package cgoCache_test

import (
	"testing"

	"github.com/yoitsuholo/relax/inf-SlaveNode/cache/cgoCache"
)

// TestSimpleSetGet tests basic set and get
func TestSimpleSetGet(t *testing.T) {
	cache, err := cgoCache.NewCGOCache(16)
	if err != nil {
		t.Fatalf("Failed to create cache: %v", err)
	}
	defer cache.Close()

	// Test Set
	err = cache.Set("key1", "value1")
	if err != nil {
		t.Errorf("Set failed: %v", err)
	}

	// Test Get
	value, exists, err := cache.Get("key1")
	if err != nil {
		t.Errorf("Get failed: %v", err)
	}
	if !exists {
		t.Error("Key should exist")
	}
	if value != "value1" {
		t.Errorf("Expected value1, got %s", value)
	}
}
