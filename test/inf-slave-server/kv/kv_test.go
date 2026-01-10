package kv_test

import (
	"testing"

	"github.com/yoitsuholo/relax/inf-SingleNode/server/cache/simpleCache"
)

// TestKVBasicSetGet tests basic Set and Get operations
func TestKVBasicSetGet(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	tests := []struct {
		name  string
		key   string
		value string
	}{
		{"simple value", "key1", "value1"},
		{"empty value", "key2", ""},
		{"long value", "key3", "very long value with special characters !@#$%^&*()"},
		{"unicode value", "key4", "你好世界🌍"},
		{"numeric string", "key5", "12345"},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			// Test Set
			err := cache.Set(tt.key, tt.value)
			if err != nil {
				t.Errorf("Set() error = %v", err)
				return
			}

			// Test Get
			value, exists, err := cache.Get(tt.key)
			if err != nil {
				t.Errorf("Get() error = %v", err)
				return
			}
			if !exists {
				t.Errorf("Get() exists = false, want true")
				return
			}
			if value != tt.value {
				t.Errorf("Get() value = %v, want %v", value, tt.value)
			}
		})
	}
}

// TestKVGetNonExistent tests Get on non-existent key
func TestKVGetNonExistent(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	value, exists, err := cache.Get("non-existent-key")
	if err != nil {
		t.Errorf("Get() error = %v, want nil", err)
	}
	if exists {
		t.Errorf("Get() exists = true, want false")
	}
	if value != "" {
		t.Errorf("Get() value = %v, want empty string", value)
	}
}

// TestKVSetOverwrite tests overwriting existing keys
func TestKVSetOverwrite(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "overwrite-key"

	// Set initial value
	err := cache.Set(key, "value1")
	if err != nil {
		t.Fatalf("Set() error = %v", err)
	}

	// Verify initial value
	value, exists, err := cache.Get(key)
	if err != nil || !exists || value != "value1" {
		t.Fatalf("Get() after first Set failed: value=%v, exists=%v, err=%v", value, exists, err)
	}

	// Overwrite with new value
	err = cache.Set(key, "value2")
	if err != nil {
		t.Fatalf("Set() overwrite error = %v", err)
	}

	// Verify new value
	value, exists, err = cache.Get(key)
	if err != nil {
		t.Errorf("Get() after overwrite error = %v", err)
	}
	if !exists {
		t.Errorf("Get() after overwrite exists = false, want true")
	}
	if value != "value2" {
		t.Errorf("Get() after overwrite value = %v, want value2", value)
	}
}

// TestKVDel tests Delete operations
func TestKVDel(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "delete-key"

	// Delete non-existent key
	deleted, err := cache.Del(key)
	if err != nil {
		t.Errorf("Del() non-existent key error = %v", err)
	}
	if deleted != 0 {
		t.Errorf("Del() non-existent key deleted = %v, want 0", deleted)
	}

	// Set a value
	err = cache.Set(key, "value")
	if err != nil {
		t.Fatalf("Set() error = %v", err)
	}

	// Delete existing key
	deleted, err = cache.Del(key)
	if err != nil {
		t.Errorf("Del() existing key error = %v", err)
	}
	if deleted != 1 {
		t.Errorf("Del() existing key deleted = %v, want 1", deleted)
	}

	// Verify key is deleted
	_, exists, err := cache.Get(key)
	if err != nil {
		t.Errorf("Get() after Del error = %v", err)
	}
	if exists {
		t.Errorf("Get() after Del exists = true, want false")
	}
}

// TestKVExists tests Exists operation
func TestKVExists(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "exists-key"

	// Check non-existent key
	exists, err := cache.Exists(key)
	if err != nil {
		t.Errorf("Exists() non-existent key error = %v", err)
	}
	if exists {
		t.Errorf("Exists() non-existent key = true, want false")
	}

	// Set a value
	err = cache.Set(key, "value")
	if err != nil {
		t.Fatalf("Set() error = %v", err)
	}

	// Check existing key
	exists, err = cache.Exists(key)
	if err != nil {
		t.Errorf("Exists() existing key error = %v", err)
	}
	if !exists {
		t.Errorf("Exists() existing key = false, want true")
	}

	// Delete and check again
	_, err = cache.Del(key)
	if err != nil {
		t.Fatalf("Del() error = %v", err)
	}

	exists, err = cache.Exists(key)
	if err != nil {
		t.Errorf("Exists() after Del error = %v", err)
	}
	if exists {
		t.Errorf("Exists() after Del = true, want false")
	}
}

// TestKVWrongType tests type error handling
func TestKVWrongType(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "type-key"

	// Create a hash at this key
	_, err := cache.HSet(key, "field", "value")
	if err != nil {
		t.Fatalf("HSet() error = %v", err)
	}

	// Try to Get as string
	_, _, err = cache.Get(key)
	if err == nil {
		t.Error("Get() on hash key should return error, got nil")
	}
	if err != nil && err.Error() != "WRONGTYPE: operation against a key holding the wrong kind of value" {
		t.Errorf("Get() on hash key error = %v, want WRONGTYPE error", err)
	}
}

// TestKVMultipleKeys tests operations with multiple keys
func TestKVMultipleKeys(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	// Set multiple keys
	keys := []string{"key1", "key2", "key3", "key4", "key5"}
	for i, key := range keys {
		err := cache.Set(key, key+"-value")
		if err != nil {
			t.Fatalf("Set(%v) error = %v", i, err)
		}
	}

	// Get all keys
	for i, key := range keys {
		value, exists, err := cache.Get(key)
		if err != nil {
			t.Errorf("Get(%v) error = %v", i, err)
		}
		if !exists {
			t.Errorf("Get(%v) exists = false, want true", i)
		}
		if value != key+"-value" {
			t.Errorf("Get(%v) value = %v, want %v", i, value, key+"-value")
		}
	}

	// Delete some keys
	for i := 0; i < 3; i++ {
		deleted, err := cache.Del(keys[i])
		if err != nil {
			t.Errorf("Del(%v) error = %v", i, err)
		}
		if deleted != 1 {
			t.Errorf("Del(%v) deleted = %v, want 1", i, deleted)
		}
	}

	// Verify deleted keys
	for i := 0; i < 3; i++ {
		exists, err := cache.Exists(keys[i])
		if err != nil {
			t.Errorf("Exists(%v) after Del error = %v", i, err)
		}
		if exists {
			t.Errorf("Exists(%v) after Del = true, want false", i)
		}
	}

	// Verify remaining keys
	for i := 3; i < len(keys); i++ {
		exists, err := cache.Exists(keys[i])
		if err != nil {
			t.Errorf("Exists(%v) remaining key error = %v", i, err)
		}
		if !exists {
			t.Errorf("Exists(%v) remaining key = false, want true", i)
		}
	}
}

// TestKVSharding tests that keys are distributed across shards
func TestKVSharding(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	// Insert many keys to ensure distribution across shards
	numKeys := 1000
	for i := 0; i < numKeys; i++ {
		key := "shard-key-" + string(rune(i))
		err := cache.Set(key, "value")
		if err != nil {
			t.Fatalf("Set() iteration %v error = %v", i, err)
		}
	}

	// Verify all keys exist
	for i := 0; i < numKeys; i++ {
		key := "shard-key-" + string(rune(i))
		exists, err := cache.Exists(key)
		if err != nil {
			t.Errorf("Exists() iteration %v error = %v", i, err)
		}
		if !exists {
			t.Errorf("Exists() iteration %v = false, want true", i)
		}
	}
}
