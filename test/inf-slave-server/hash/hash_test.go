package hash_test

import (
	"reflect"
	"sort"
	"testing"

	"github.com/yoitsuholo/relax/inf-SingleNode/server/cache/simpleCache"
)

// TestHashBasicHSetHGet tests basic HSet and HGet operations
func TestHashBasicHSetHGet(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "hash-key"
	field := "field1"
	value := "value1"

	// Test HSet (new field)
	isNew, err := cache.HSet(key, field, value)
	if err != nil {
		t.Errorf("HSet() error = %v", err)
		return
	}
	if !isNew {
		t.Errorf("HSet() new field isNew = false, want true")
	}

	// Test HGet
	gotValue, exists, err := cache.HGet(key, field)
	if err != nil {
		t.Errorf("HGet() error = %v", err)
		return
	}
	if !exists {
		t.Errorf("HGet() exists = false, want true")
		return
	}
	if gotValue != value {
		t.Errorf("HGet() value = %v, want %v", gotValue, value)
	}
}

// TestHashHSetOverwrite tests overwriting hash fields
func TestHashHSetOverwrite(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "hash-key"
	field := "field1"

	// Set initial value
	isNew, err := cache.HSet(key, field, "value1")
	if err != nil || !isNew {
		t.Fatalf("HSet() initial error = %v, isNew = %v", err, isNew)
	}

	// Overwrite with new value
	isNew, err = cache.HSet(key, field, "value2")
	if err != nil {
		t.Errorf("HSet() overwrite error = %v", err)
	}
	if isNew {
		t.Errorf("HSet() overwrite isNew = true, want false")
	}

	// Verify new value
	value, exists, err := cache.HGet(key, field)
	if err != nil {
		t.Errorf("HGet() after overwrite error = %v", err)
	}
	if !exists {
		t.Errorf("HGet() after overwrite exists = false, want true")
	}
	if value != "value2" {
		t.Errorf("HGet() after overwrite value = %v, want value2", value)
	}
}

// TestHashHGetNonExistent tests HGet on non-existent keys/fields
func TestHashHGetNonExistent(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	// Non-existent key
	value, exists, err := cache.HGet("non-existent-key", "field")
	if err != nil {
		t.Errorf("HGet() non-existent key error = %v", err)
	}
	if exists {
		t.Errorf("HGet() non-existent key exists = true, want false")
	}
	if value != "" {
		t.Errorf("HGet() non-existent key value = %v, want empty", value)
	}

	// Existent key but non-existent field
	key := "hash-key"
	_, err = cache.HSet(key, "field1", "value1")
	if err != nil {
		t.Fatalf("HSet() error = %v", err)
	}

	value, exists, err = cache.HGet(key, "non-existent-field")
	if err != nil {
		t.Errorf("HGet() non-existent field error = %v", err)
	}
	if exists {
		t.Errorf("HGet() non-existent field exists = true, want false")
	}
	if value != "" {
		t.Errorf("HGet() non-existent field value = %v, want empty", value)
	}
}

// TestHashHDel tests HDel operation
func TestHashHDel(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "hash-key"

	// Delete non-existent key
	deleted, err := cache.HDel(key, "field")
	if err != nil {
		t.Errorf("HDel() non-existent key error = %v", err)
	}
	if deleted != 0 {
		t.Errorf("HDel() non-existent key deleted = %v, want 0", deleted)
	}

	// Set some fields
	_, err = cache.HSet(key, "field1", "value1")
	if err != nil {
		t.Fatalf("HSet() field1 error = %v", err)
	}
	_, err = cache.HSet(key, "field2", "value2")
	if err != nil {
		t.Fatalf("HSet() field2 error = %v", err)
	}

	// Delete existing field
	deleted, err = cache.HDel(key, "field1")
	if err != nil {
		t.Errorf("HDel() existing field error = %v", err)
	}
	if deleted != 1 {
		t.Errorf("HDel() existing field deleted = %v, want 1", deleted)
	}

	// Verify field is deleted
	_, exists, err := cache.HGet(key, "field1")
	if err != nil {
		t.Errorf("HGet() after HDel error = %v", err)
	}
	if exists {
		t.Errorf("HGet() after HDel exists = true, want false")
	}

	// Verify other field still exists
	value, exists, err := cache.HGet(key, "field2")
	if err != nil {
		t.Errorf("HGet() field2 after HDel error = %v", err)
	}
	if !exists {
		t.Errorf("HGet() field2 after HDel exists = false, want true")
	}
	if value != "value2" {
		t.Errorf("HGet() field2 after HDel value = %v, want value2", value)
	}
}

// TestHashHMSet tests HMSet operation
func TestHashHMSet(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "hash-key"
	fields := map[string]string{
		"field1": "value1",
		"field2": "value2",
		"field3": "value3",
	}

	// Set multiple fields
	count, err := cache.HMSet(key, fields)
	if err != nil {
		t.Fatalf("HMSet() error = %v", err)
	}
	if count != len(fields) {
		t.Errorf("HMSet() count = %v, want %v", count, len(fields))
	}

	// Verify all fields
	for field, wantValue := range fields {
		value, exists, err := cache.HGet(key, field)
		if err != nil {
			t.Errorf("HGet(%v) error = %v", field, err)
		}
		if !exists {
			t.Errorf("HGet(%v) exists = false, want true", field)
		}
		if value != wantValue {
			t.Errorf("HGet(%v) value = %v, want %v", field, value, wantValue)
		}
	}
}

// TestHashHMGet tests HMGet operation
func TestHashHMGet(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "hash-key"

	// Set some fields
	fields := map[string]string{
		"field1": "value1",
		"field2": "value2",
		"field3": "value3",
	}
	_, err := cache.HMSet(key, fields)
	if err != nil {
		t.Fatalf("HMSet() error = %v", err)
	}

	// Get multiple fields (including non-existent)
	requestFields := []string{"field1", "field2", "non-existent"}
	result, err := cache.HMGet(key, requestFields)
	if err != nil {
		t.Fatalf("HMGet() error = %v", err)
	}

	// Verify results
	if len(result) != 2 {
		t.Errorf("HMGet() result length = %v, want 2", len(result))
	}
	if result["field1"] != "value1" {
		t.Errorf("HMGet() field1 = %v, want value1", result["field1"])
	}
	if result["field2"] != "value2" {
		t.Errorf("HMGet() field2 = %v, want value2", result["field2"])
	}
	if _, exists := result["non-existent"]; exists {
		t.Errorf("HMGet() non-existent field should not exist")
	}
}

// TestHashHLen tests HLen operation
func TestHashHLen(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "hash-key"

	// Check length of non-existent key
	length, err := cache.HLen(key)
	if err != nil {
		t.Errorf("HLen() non-existent key error = %v", err)
	}
	if length != 0 {
		t.Errorf("HLen() non-existent key = %v, want 0", length)
	}

	// Add fields one by one
	for i := 1; i <= 5; i++ {
		field := "field" + string(rune('0'+i))
		_, err := cache.HSet(key, field, "value")
		if err != nil {
			t.Fatalf("HSet() iteration %v error = %v", i, err)
		}

		length, err := cache.HLen(key)
		if err != nil {
			t.Errorf("HLen() iteration %v error = %v", i, err)
		}
		if length != i {
			t.Errorf("HLen() iteration %v = %v, want %v", i, length, i)
		}
	}
}

// TestHashHGetAll tests HGetAll operation
func TestHashHGetAll(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "hash-key"

	// Get all from non-existent key
	result, err := cache.HGetAll(key)
	if err != nil {
		t.Errorf("HGetAll() non-existent key error = %v", err)
	}
	if len(result) != 0 {
		t.Errorf("HGetAll() non-existent key length = %v, want 0", len(result))
	}

	// Set some fields
	expected := map[string]string{
		"field1": "value1",
		"field2": "value2",
		"field3": "value3",
	}
	_, err = cache.HMSet(key, expected)
	if err != nil {
		t.Fatalf("HMSet() error = %v", err)
	}

	// Get all fields
	result, err = cache.HGetAll(key)
	if err != nil {
		t.Errorf("HGetAll() error = %v", err)
	}

	if !reflect.DeepEqual(result, expected) {
		t.Errorf("HGetAll() = %v, want %v", result, expected)
	}
}

// TestHashHExists tests HExists operation
func TestHashHExists(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "hash-key"

	// Check non-existent key
	exists, err := cache.HExists(key, "field")
	if err != nil {
		t.Errorf("HExists() non-existent key error = %v", err)
	}
	if exists {
		t.Errorf("HExists() non-existent key = true, want false")
	}

	// Set a field
	_, err = cache.HSet(key, "field1", "value1")
	if err != nil {
		t.Fatalf("HSet() error = %v", err)
	}

	// Check existing field
	exists, err = cache.HExists(key, "field1")
	if err != nil {
		t.Errorf("HExists() existing field error = %v", err)
	}
	if !exists {
		t.Errorf("HExists() existing field = false, want true")
	}

	// Check non-existent field
	exists, err = cache.HExists(key, "non-existent")
	if err != nil {
		t.Errorf("HExists() non-existent field error = %v", err)
	}
	if exists {
		t.Errorf("HExists() non-existent field = true, want false")
	}
}

// TestHashWrongType tests type error handling
func TestHashWrongType(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "type-key"

	// Create a string at this key
	err := cache.Set(key, "string-value")
	if err != nil {
		t.Fatalf("Set() error = %v", err)
	}

	// Try hash operations
	_, err = cache.HSet(key, "field", "value")
	if err == nil {
		t.Error("HSet() on string key should return error, got nil")
	}

	_, _, err = cache.HGet(key, "field")
	if err == nil {
		t.Error("HGet() on string key should return error, got nil")
	}

	_, err = cache.HDel(key, "field")
	if err == nil {
		t.Error("HDel() on string key should return error, got nil")
	}

	_, err = cache.HLen(key)
	if err == nil {
		t.Error("HLen() on string key should return error, got nil")
	}
}

// TestHashEmptyFieldCleanup tests that empty hashes are cleaned up
func TestHashEmptyFieldCleanup(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "hash-key"

	// Set and delete a single field
	_, err := cache.HSet(key, "field", "value")
	if err != nil {
		t.Fatalf("HSet() error = %v", err)
	}

	deleted, err := cache.HDel(key, "field")
	if err != nil {
		t.Fatalf("HDel() error = %v", err)
	}
	if deleted != 1 {
		t.Errorf("HDel() deleted = %v, want 1", deleted)
	}

	// Verify hash is cleaned up (key doesn't exist)
	exists, err := cache.Exists(key)
	if err != nil {
		t.Errorf("Exists() after cleanup error = %v", err)
	}
	if exists {
		t.Errorf("Exists() after cleanup = true, want false (hash should be cleaned up)")
	}
}

// TestHashMultipleFields tests operations with many fields
func TestHashMultipleFields(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "hash-key"
	numFields := 100

	// Set many fields
	fields := make(map[string]string)
	for i := 0; i < numFields; i++ {
		field := "field" + string(rune('0'+i%10)) + string(rune('0'+i/10))
		value := "value" + string(rune('0'+i%10)) + string(rune('0'+i/10))
		fields[field] = value
	}

	_, err := cache.HMSet(key, fields)
	if err != nil {
		t.Fatalf("HMSet() error = %v", err)
	}

	// Verify length
	length, err := cache.HLen(key)
	if err != nil {
		t.Errorf("HLen() error = %v", err)
	}
	if length != numFields {
		t.Errorf("HLen() = %v, want %v", length, numFields)
	}

	// Get all and verify
	result, err := cache.HGetAll(key)
	if err != nil {
		t.Errorf("HGetAll() error = %v", err)
	}
	if len(result) != numFields {
		t.Errorf("HGetAll() length = %v, want %v", len(result), numFields)
	}

	// Verify content matches
	for field, expectedValue := range fields {
		if result[field] != expectedValue {
			t.Errorf("HGetAll()[%v] = %v, want %v", field, result[field], expectedValue)
		}
	}
}

// TestHashMixedKeys tests multiple hashes with same field names
func TestHashMixedKeys(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	keys := []string{"hash1", "hash2", "hash3"}
	field := "common-field"

	// Set same field in different hashes
	for i, key := range keys {
		value := "value-" + string(rune('0'+i))
		_, err := cache.HSet(key, field, value)
		if err != nil {
			t.Fatalf("HSet(%v) error = %v", key, err)
		}
	}

	// Verify each hash has correct value
	for i, key := range keys {
		expectedValue := "value-" + string(rune('0'+i))
		value, exists, err := cache.HGet(key, field)
		if err != nil {
			t.Errorf("HGet(%v) error = %v", key, err)
		}
		if !exists {
			t.Errorf("HGet(%v) exists = false, want true", key)
		}
		if value != expectedValue {
			t.Errorf("HGet(%v) = %v, want %v", key, value, expectedValue)
		}
	}
}

// Helper function to compare string slices (order independent)
func stringSlicesEqual(a, b []string) bool {
	if len(a) != len(b) {
		return false
	}
	aCopy := make([]string, len(a))
	bCopy := make([]string, len(b))
	copy(aCopy, a)
	copy(bCopy, b)
	sort.Strings(aCopy)
	sort.Strings(bCopy)
	for i := range aCopy {
		if aCopy[i] != bCopy[i] {
			return false
		}
	}
	return true
}
