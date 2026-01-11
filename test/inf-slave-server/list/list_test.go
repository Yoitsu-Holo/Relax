package list_test

import (
	"reflect"
	"testing"

	"github.com/yoitsuholo/relax/inf-SlaveNode/cache/simpleCache"
)

// TestListBasicLPush tests basic LPush operation
func TestListBasicLPush(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "list-key"

	// Push first element
	length, err := cache.LPush(key, "value1")
	if err != nil {
		t.Errorf("LPush() first error = %v", err)
		return
	}
	if length != 1 {
		t.Errorf("LPush() first length = %v, want 1", length)
	}

	// Push second element
	length, err = cache.LPush(key, "value2")
	if err != nil {
		t.Errorf("LPush() second error = %v", err)
		return
	}
	if length != 2 {
		t.Errorf("LPush() second length = %v, want 2", length)
	}
}

// TestListBasicRPush tests basic RPush operation
func TestListBasicRPush(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "list-key"

	// Push first element
	length, err := cache.RPush(key, "value1")
	if err != nil {
		t.Errorf("RPush() first error = %v", err)
		return
	}
	if length != 1 {
		t.Errorf("RPush() first length = %v, want 1", length)
	}

	// Push second element
	length, err = cache.RPush(key, "value2")
	if err != nil {
		t.Errorf("RPush() second error = %v", err)
		return
	}
	if length != 2 {
		t.Errorf("RPush() second length = %v, want 2", length)
	}
}

// TestListLPushRPushOrder tests that LPush and RPush maintain correct order
func TestListLPushRPushOrder(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "list-key"

	// Push elements with RPush
	cache.RPush(key, "1")
	cache.RPush(key, "2")
	cache.RPush(key, "3")

	// Verify order with LRange
	values, err := cache.LRange(key, 0, -1)
	if err != nil {
		t.Fatalf("LRange() error = %v", err)
	}
	expected := []string{"1", "2", "3"}
	if !reflect.DeepEqual(values, expected) {
		t.Errorf("LRange() after RPush = %v, want %v", values, expected)
	}

	// Test LPush order
	cache.Del(key)
	cache.LPush(key, "3")
	cache.LPush(key, "2")
	cache.LPush(key, "1")

	values, err = cache.LRange(key, 0, -1)
	if err != nil {
		t.Fatalf("LRange() error = %v", err)
	}
	if !reflect.DeepEqual(values, expected) {
		t.Errorf("LRange() after LPush = %v, want %v", values, expected)
	}
}

// TestListLRange tests LRange with various ranges
func TestListLRange(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "list-key"

	// Create a list with 5 elements
	for i := 1; i <= 5; i++ {
		_, err := cache.RPush(key, string(rune('0'+i)))
		if err != nil {
			t.Fatalf("RPush() iteration %v error = %v", i, err)
		}
	}

	tests := []struct {
		name     string
		start    int
		stop     int
		expected []string
	}{
		{"full range", 0, -1, []string{"1", "2", "3", "4", "5"}},
		{"first three", 0, 2, []string{"1", "2", "3"}},
		{"last three", -3, -1, []string{"3", "4", "5"}},
		{"middle range", 1, 3, []string{"2", "3", "4"}},
		{"single element", 2, 2, []string{"3"}},
		{"negative indices", -4, -2, []string{"2", "3", "4"}},
		{"out of range", 0, 100, []string{"1", "2", "3", "4", "5"}},
		{"invalid range", 3, 1, []string{}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			values, err := cache.LRange(key, tt.start, tt.stop)
			if err != nil {
				t.Errorf("LRange() error = %v", err)
				return
			}
			if !reflect.DeepEqual(values, tt.expected) {
				t.Errorf("LRange(%v, %v) = %v, want %v", tt.start, tt.stop, values, tt.expected)
			}
		})
	}
}

// TestListLPop tests LPop operation
func TestListLPop(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "list-key"

	// Pop from non-existent list
	value, exists, err := cache.LPop(key)
	if err != nil {
		t.Errorf("LPop() non-existent error = %v", err)
	}
	if exists {
		t.Errorf("LPop() non-existent exists = true, want false")
	}
	if value != "" {
		t.Errorf("LPop() non-existent value = %v, want empty", value)
	}

	// Create a list
	cache.RPush(key, "1")
	cache.RPush(key, "2")
	cache.RPush(key, "3")

	// Pop elements
	value, exists, err = cache.LPop(key)
	if err != nil {
		t.Errorf("LPop() first error = %v", err)
	}
	if !exists {
		t.Errorf("LPop() first exists = false, want true")
	}
	if value != "1" {
		t.Errorf("LPop() first value = %v, want 1", value)
	}

	// Verify remaining list
	values, err := cache.LRange(key, 0, -1)
	if err != nil {
		t.Fatalf("LRange() error = %v", err)
	}
	expected := []string{"2", "3"}
	if !reflect.DeepEqual(values, expected) {
		t.Errorf("LRange() after LPop = %v, want %v", values, expected)
	}
}

// TestListRPop tests RPop operation
func TestListRPop(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "list-key"

	// Pop from non-existent list
	value, exists, err := cache.RPop(key)
	if err != nil {
		t.Errorf("RPop() non-existent error = %v", err)
	}
	if exists {
		t.Errorf("RPop() non-existent exists = true, want false")
	}
	if value != "" {
		t.Errorf("RPop() non-existent value = %v, want empty", value)
	}

	// Create a list
	cache.RPush(key, "1")
	cache.RPush(key, "2")
	cache.RPush(key, "3")

	// Pop elements
	value, exists, err = cache.RPop(key)
	if err != nil {
		t.Errorf("RPop() first error = %v", err)
	}
	if !exists {
		t.Errorf("RPop() first exists = false, want true")
	}
	if value != "3" {
		t.Errorf("RPop() first value = %v, want 3", value)
	}

	// Verify remaining list
	values, err := cache.LRange(key, 0, -1)
	if err != nil {
		t.Fatalf("LRange() error = %v", err)
	}
	expected := []string{"1", "2"}
	if !reflect.DeepEqual(values, expected) {
		t.Errorf("LRange() after RPop = %v, want %v", values, expected)
	}
}

// TestListLLen tests LLen operation
func TestListLLen(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "list-key"

	// Length of non-existent list
	length, err := cache.LLen(key)
	if err != nil {
		t.Errorf("LLen() non-existent error = %v", err)
	}
	if length != 0 {
		t.Errorf("LLen() non-existent = %v, want 0", length)
	}

	// Add elements and check length
	for i := 1; i <= 5; i++ {
		cache.RPush(key, string(rune('0'+i)))
		length, err := cache.LLen(key)
		if err != nil {
			t.Errorf("LLen() iteration %v error = %v", i, err)
		}
		if length != i {
			t.Errorf("LLen() iteration %v = %v, want %v", i, length, i)
		}
	}
}

// TestListLIndex tests LIndex operation
func TestListLIndex(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "list-key"

	// Create a list
	for i := 1; i <= 5; i++ {
		cache.RPush(key, string(rune('0'+i)))
	}

	tests := []struct {
		name     string
		index    int
		expected string
		exists   bool
	}{
		{"first element", 0, "1", true},
		{"middle element", 2, "3", true},
		{"last element", 4, "5", true},
		{"negative index -1", -1, "5", true},
		{"negative index -3", -3, "3", true},
		{"out of range positive", 10, "", false},
		{"out of range negative", -10, "", false},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			value, exists, err := cache.LIndex(key, tt.index)
			if err != nil {
				t.Errorf("LIndex() error = %v", err)
				return
			}
			if exists != tt.exists {
				t.Errorf("LIndex() exists = %v, want %v", exists, tt.exists)
			}
			if value != tt.expected {
				t.Errorf("LIndex() value = %v, want %v", value, tt.expected)
			}
		})
	}
}

// TestListLSet tests LSet operation
func TestListLSet(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "list-key"

	// Set on non-existent list
	err := cache.LSet(key, 0, "value")
	if err == nil {
		t.Error("LSet() on non-existent list should return error, got nil")
	}

	// Create a list
	for i := 1; i <= 5; i++ {
		cache.RPush(key, string(rune('0'+i)))
	}

	// Set valid index
	err = cache.LSet(key, 2, "X")
	if err != nil {
		t.Errorf("LSet() valid index error = %v", err)
	}

	// Verify change
	value, exists, err := cache.LIndex(key, 2)
	if err != nil {
		t.Errorf("LIndex() after LSet error = %v", err)
	}
	if !exists {
		t.Errorf("LIndex() after LSet exists = false, want true")
	}
	if value != "X" {
		t.Errorf("LIndex() after LSet value = %v, want X", value)
	}

	// Set negative index
	err = cache.LSet(key, -1, "Y")
	if err != nil {
		t.Errorf("LSet() negative index error = %v", err)
	}

	value, exists, err = cache.LIndex(key, -1)
	if err != nil {
		t.Errorf("LIndex() after LSet negative error = %v", err)
	}
	if !exists {
		t.Errorf("LIndex() after LSet negative exists = false, want true")
	}
	if value != "Y" {
		t.Errorf("LIndex() after LSet negative value = %v, want Y", value)
	}

	// Set out of range index
	err = cache.LSet(key, 100, "value")
	if err == nil {
		t.Error("LSet() out of range should return error, got nil")
	}
}

// TestListLRem tests LRem operation
func TestListLRem(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "list-key"

	// Remove from non-existent list
	removed, err := cache.LRem(key, 0, "value")
	if err != nil {
		t.Errorf("LRem() non-existent error = %v", err)
	}
	if removed != 0 {
		t.Errorf("LRem() non-existent removed = %v, want 0", removed)
	}

	// Test remove all occurrences (count = 0)
	cache.RPush(key, "A")
	cache.RPush(key, "B")
	cache.RPush(key, "A")
	cache.RPush(key, "C")
	cache.RPush(key, "A")

	removed, err = cache.LRem(key, 0, "A")
	if err != nil {
		t.Errorf("LRem() count=0 error = %v", err)
	}
	if removed != 3 {
		t.Errorf("LRem() count=0 removed = %v, want 3", removed)
	}

	values, _ := cache.LRange(key, 0, -1)
	expected := []string{"B", "C"}
	if !reflect.DeepEqual(values, expected) {
		t.Errorf("LRange() after LRem count=0 = %v, want %v", values, expected)
	}

	// Test remove first N occurrences (count > 0)
	cache.Del(key)
	cache.RPush(key, "A")
	cache.RPush(key, "A")
	cache.RPush(key, "B")
	cache.RPush(key, "A")

	removed, err = cache.LRem(key, 2, "A")
	if err != nil {
		t.Errorf("LRem() count>0 error = %v", err)
	}
	if removed != 2 {
		t.Errorf("LRem() count>0 removed = %v, want 2", removed)
	}

	values, _ = cache.LRange(key, 0, -1)
	expected = []string{"B", "A"}
	if !reflect.DeepEqual(values, expected) {
		t.Errorf("LRange() after LRem count>0 = %v, want %v", values, expected)
	}

	// Test remove last N occurrences (count < 0)
	cache.Del(key)
	cache.RPush(key, "A")
	cache.RPush(key, "B")
	cache.RPush(key, "A")
	cache.RPush(key, "A")

	removed, err = cache.LRem(key, -2, "A")
	if err != nil {
		t.Errorf("LRem() count<0 error = %v", err)
	}
	if removed != 2 {
		t.Errorf("LRem() count<0 removed = %v, want 2", removed)
	}

	values, _ = cache.LRange(key, 0, -1)
	expected = []string{"A", "B"}
	if !reflect.DeepEqual(values, expected) {
		t.Errorf("LRange() after LRem count<0 = %v, want %v", values, expected)
	}
}

// TestListWrongType tests type error handling
func TestListWrongType(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "type-key"

	// Create a string at this key
	err := cache.Set(key, "string-value")
	if err != nil {
		t.Fatalf("Set() error = %v", err)
	}

	// Try list operations
	_, err = cache.LPush(key, "value")
	if err == nil {
		t.Error("LPush() on string key should return error, got nil")
	}

	_, err = cache.RPush(key, "value")
	if err == nil {
		t.Error("RPush() on string key should return error, got nil")
	}

	_, _, err = cache.LPop(key)
	if err == nil {
		t.Error("LPop() on string key should return error, got nil")
	}

	_, _, err = cache.RPop(key)
	if err == nil {
		t.Error("RPop() on string key should return error, got nil")
	}

	_, err = cache.LRange(key, 0, -1)
	if err == nil {
		t.Error("LRange() on string key should return error, got nil")
	}

	_, err = cache.LLen(key)
	if err == nil {
		t.Error("LLen() on string key should return error, got nil")
	}

	_, _, err = cache.LIndex(key, 0)
	if err == nil {
		t.Error("LIndex() on string key should return error, got nil")
	}

	err = cache.LSet(key, 0, "value")
	if err == nil {
		t.Error("LSet() on string key should return error, got nil")
	}
}

// TestListEmptyCleanup tests that empty lists are cleaned up
func TestListEmptyCleanup(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "list-key"

	// Create and empty a list with LPop
	cache.LPush(key, "value")
	_, _, err := cache.LPop(key)
	if err != nil {
		t.Fatalf("LPop() error = %v", err)
	}

	// Verify list is cleaned up
	exists, err := cache.Exists(key)
	if err != nil {
		t.Errorf("Exists() after LPop cleanup error = %v", err)
	}
	if exists {
		t.Errorf("Exists() after LPop cleanup = true, want false")
	}

	// Test with RPop
	cache.RPush(key, "value")
	_, _, err = cache.RPop(key)
	if err != nil {
		t.Fatalf("RPop() error = %v", err)
	}

	exists, err = cache.Exists(key)
	if err != nil {
		t.Errorf("Exists() after RPop cleanup error = %v", err)
	}
	if exists {
		t.Errorf("Exists() after RPop cleanup = true, want false")
	}

	// Test with LRem
	cache.LPush(key, "value")
	_, err = cache.LRem(key, 0, "value")
	if err != nil {
		t.Fatalf("LRem() error = %v", err)
	}

	exists, err = cache.Exists(key)
	if err != nil {
		t.Errorf("Exists() after LRem cleanup error = %v", err)
	}
	if exists {
		t.Errorf("Exists() after LRem cleanup = true, want false")
	}
}

// TestListLargeLis tests operations with many elements
func TestListLargeList(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "large-list"
	numElements := 1000

	// Add many elements
	for i := 0; i < numElements; i++ {
		_, err := cache.RPush(key, string(rune('0'+i%10)))
		if err != nil {
			t.Fatalf("RPush() iteration %v error = %v", i, err)
		}
	}

	// Verify length
	length, err := cache.LLen(key)
	if err != nil {
		t.Errorf("LLen() error = %v", err)
	}
	if length != numElements {
		t.Errorf("LLen() = %v, want %v", length, numElements)
	}

	// Verify a range
	values, err := cache.LRange(key, 0, 9)
	if err != nil {
		t.Errorf("LRange() error = %v", err)
	}
	if len(values) != 10 {
		t.Errorf("LRange() length = %v, want 10", len(values))
	}

	// Pop some elements
	for i := 0; i < 100; i++ {
		_, _, err := cache.LPop(key)
		if err != nil {
			t.Fatalf("LPop() iteration %v error = %v", i, err)
		}
	}

	// Verify new length
	length, err = cache.LLen(key)
	if err != nil {
		t.Errorf("LLen() after pops error = %v", err)
	}
	if length != numElements-100 {
		t.Errorf("LLen() after pops = %v, want %v", length, numElements-100)
	}
}
