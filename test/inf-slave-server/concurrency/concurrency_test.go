package concurrency_test

import (
	"strconv"
	"sync"
	"sync/atomic"
	"testing"

	"github.com/yoitsuholo/relax/inf-SlaveNode/cache/simpleCache"
)

// TestConcurrentKVOperations tests concurrent KV operations
func TestConcurrentKVOperations(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	numGoroutines := 100
	numOperations := 1000

	var wg sync.WaitGroup
	wg.Add(numGoroutines)

	// Concurrent Set operations
	for i := 0; i < numGoroutines; i++ {
		go func(id int) {
			defer wg.Done()
			for j := 0; j < numOperations; j++ {
				key := "key-" + strconv.Itoa(id) + "-" + strconv.Itoa(j)
				value := "value-" + strconv.Itoa(id) + "-" + strconv.Itoa(j)
				err := cache.Set(key, value)
				if err != nil {
					t.Errorf("Goroutine %d: Set() error = %v", id, err)
				}
			}
		}(i)
	}

	wg.Wait()

	// Verify all keys exist
	for i := 0; i < numGoroutines; i++ {
		for j := 0; j < numOperations; j++ {
			key := "key-" + strconv.Itoa(i) + "-" + strconv.Itoa(j)
			exists, err := cache.Exists(key)
			if err != nil {
				t.Errorf("Exists(%v) error = %v", key, err)
			}
			if !exists {
				t.Errorf("Exists(%v) = false, want true", key)
			}
		}
	}
}

// TestConcurrentReadWrite tests concurrent reads and writes
func TestConcurrentReadWrite(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	numKeys := 100
	numReaders := 50
	numWriters := 10

	// Initialize keys
	for i := 0; i < numKeys; i++ {
		key := "key-" + strconv.Itoa(i)
		err := cache.Set(key, "initial-value")
		if err != nil {
			t.Fatalf("Set(%v) error = %v", key, err)
		}
	}

	var wg sync.WaitGroup
	var readErrors, writeErrors atomic.Int64

	// Start readers
	wg.Add(numReaders)
	for i := 0; i < numReaders; i++ {
		go func() {
			defer wg.Done()
			for j := 0; j < 1000; j++ {
				key := "key-" + strconv.Itoa(j%numKeys)
				_, exists, err := cache.Get(key)
				if err != nil {
					readErrors.Add(1)
				}
				if !exists {
					readErrors.Add(1)
				}
			}
		}()
	}

	// Start writers
	wg.Add(numWriters)
	for i := 0; i < numWriters; i++ {
		go func(id int) {
			defer wg.Done()
			for j := 0; j < 1000; j++ {
				key := "key-" + strconv.Itoa(j%numKeys)
				value := "writer-" + strconv.Itoa(id) + "-" + strconv.Itoa(j)
				err := cache.Set(key, value)
				if err != nil {
					writeErrors.Add(1)
				}
			}
		}(i)
	}

	wg.Wait()

	if readErrors.Load() > 0 {
		t.Errorf("Read errors: %d", readErrors.Load())
	}
	if writeErrors.Load() > 0 {
		t.Errorf("Write errors: %d", writeErrors.Load())
	}
}

// TestConcurrentHashOperations tests concurrent hash operations
func TestConcurrentHashOperations(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	numGoroutines := 50
	numOperations := 500

	var wg sync.WaitGroup
	wg.Add(numGoroutines)

	// Concurrent HSet operations
	for i := 0; i < numGoroutines; i++ {
		go func(id int) {
			defer wg.Done()
			for j := 0; j < numOperations; j++ {
				key := "hash-" + strconv.Itoa(id%10)
				field := "field-" + strconv.Itoa(id) + "-" + strconv.Itoa(j)
				value := "value-" + strconv.Itoa(id) + "-" + strconv.Itoa(j)
				_, err := cache.HSet(key, field, value)
				if err != nil {
					t.Errorf("Goroutine %d: HSet() error = %v", id, err)
				}
			}
		}(i)
	}

	wg.Wait()

	// Verify hash integrity
	for i := 0; i < 10; i++ {
		key := "hash-" + strconv.Itoa(i)
		length, err := cache.HLen(key)
		if err != nil {
			t.Errorf("HLen(%v) error = %v", key, err)
		}
		if length == 0 {
			t.Errorf("HLen(%v) = 0, expected non-zero", key)
		}
	}
}

// TestConcurrentSetOperations tests concurrent set operations
func TestConcurrentSetOperations(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	numGoroutines := 50
	numOperations := 500
	key := "shared-set"

	var wg sync.WaitGroup
	wg.Add(numGoroutines)

	// Concurrent SAdd operations
	for i := 0; i < numGoroutines; i++ {
		go func(id int) {
			defer wg.Done()
			for j := 0; j < numOperations; j++ {
				member := "member-" + strconv.Itoa(id) + "-" + strconv.Itoa(j)
				_, err := cache.SAdd(key, member)
				if err != nil {
					t.Errorf("Goroutine %d: SAdd() error = %v", id, err)
				}
			}
		}(i)
	}

	wg.Wait()

	// Verify cardinality
	card, err := cache.SCard(key)
	if err != nil {
		t.Errorf("SCard() error = %v", err)
	}
	expected := numGoroutines * numOperations
	if card != expected {
		t.Errorf("SCard() = %d, want %d", card, expected)
	}
}

// TestConcurrentListOperations tests concurrent list operations
func TestConcurrentListOperations(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	numGoroutines := 50
	numOperations := 100

	var wg sync.WaitGroup
	wg.Add(numGoroutines * 2)

	// Concurrent LPush operations
	for i := 0; i < numGoroutines; i++ {
		go func(id int) {
			defer wg.Done()
			for j := 0; j < numOperations; j++ {
				key := "list-" + strconv.Itoa(id%5)
				value := "lpush-" + strconv.Itoa(id) + "-" + strconv.Itoa(j)
				_, err := cache.LPush(key, value)
				if err != nil {
					t.Errorf("Goroutine %d: LPush() error = %v", id, err)
				}
			}
		}(i)
	}

	// Concurrent RPush operations
	for i := 0; i < numGoroutines; i++ {
		go func(id int) {
			defer wg.Done()
			for j := 0; j < numOperations; j++ {
				key := "list-" + strconv.Itoa(id%5)
				value := "rpush-" + strconv.Itoa(id) + "-" + strconv.Itoa(j)
				_, err := cache.RPush(key, value)
				if err != nil {
					t.Errorf("Goroutine %d: RPush() error = %v", id, err)
				}
			}
		}(i)
	}

	wg.Wait()

	// Verify list lengths
	for i := 0; i < 5; i++ {
		key := "list-" + strconv.Itoa(i)
		length, err := cache.LLen(key)
		if err != nil {
			t.Errorf("LLen(%v) error = %v", key, err)
		}
		expected := (numGoroutines / 5) * numOperations * 2
		if length != expected {
			t.Errorf("LLen(%v) = %d, want %d", key, length, expected)
		}
	}
}

// TestConcurrentMixedOperations tests concurrent operations across different data types
func TestConcurrentMixedOperations(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	numGoroutines := 20
	numOperations := 200

	var wg sync.WaitGroup
	wg.Add(numGoroutines * 4)

	// KV operations
	for i := 0; i < numGoroutines; i++ {
		go func(id int) {
			defer wg.Done()
			for j := 0; j < numOperations; j++ {
				key := "kv-" + strconv.Itoa(id)
				value := "value-" + strconv.Itoa(j)
				cache.Set(key, value)
			}
		}(i)
	}

	// Hash operations
	for i := 0; i < numGoroutines; i++ {
		go func(id int) {
			defer wg.Done()
			for j := 0; j < numOperations; j++ {
				key := "hash-" + strconv.Itoa(id)
				field := "field-" + strconv.Itoa(j)
				value := "value-" + strconv.Itoa(j)
				cache.HSet(key, field, value)
			}
		}(i)
	}

	// Set operations
	for i := 0; i < numGoroutines; i++ {
		go func(id int) {
			defer wg.Done()
			for j := 0; j < numOperations; j++ {
				key := "set-" + strconv.Itoa(id)
				member := "member-" + strconv.Itoa(j)
				cache.SAdd(key, member)
			}
		}(i)
	}

	// List operations
	for i := 0; i < numGoroutines; i++ {
		go func(id int) {
			defer wg.Done()
			for j := 0; j < numOperations; j++ {
				key := "list-" + strconv.Itoa(id)
				value := "value-" + strconv.Itoa(j)
				cache.RPush(key, value)
			}
		}(i)
	}

	wg.Wait()

	// Verify data integrity
	for i := 0; i < numGoroutines; i++ {
		// Verify KV
		key := "kv-" + strconv.Itoa(i)
		exists, err := cache.Exists(key)
		if err != nil || !exists {
			t.Errorf("KV key %v verification failed", key)
		}

		// Verify Hash
		key = "hash-" + strconv.Itoa(i)
		length, err := cache.HLen(key)
		if err != nil || length != numOperations {
			t.Errorf("Hash key %v length = %d, want %d", key, length, numOperations)
		}

		// Verify Set
		key = "set-" + strconv.Itoa(i)
		card, err := cache.SCard(key)
		if err != nil || card != numOperations {
			t.Errorf("Set key %v card = %d, want %d", key, card, numOperations)
		}

		// Verify List
		key = "list-" + strconv.Itoa(i)
		listLen, err := cache.LLen(key)
		if err != nil || listLen != numOperations {
			t.Errorf("List key %v length = %d, want %d", key, listLen, numOperations)
		}
	}
}

// TestConcurrentDeleteOperations tests concurrent delete operations
func TestConcurrentDeleteOperations(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	numKeys := 1000

	// Initialize keys
	for i := 0; i < numKeys; i++ {
		key := "key-" + strconv.Itoa(i)
		cache.Set(key, "value")
	}

	var wg sync.WaitGroup
	var deleteCount atomic.Int64

	// Concurrent delete operations
	wg.Add(numKeys)
	for i := 0; i < numKeys; i++ {
		go func(id int) {
			defer wg.Done()
			key := "key-" + strconv.Itoa(id)
			deleted, err := cache.Del(key)
			if err != nil {
				t.Errorf("Del(%v) error = %v", key, err)
			}
			deleteCount.Add(int64(deleted))
		}(i)
	}

	wg.Wait()

	// Verify all keys deleted
	if deleteCount.Load() != int64(numKeys) {
		t.Errorf("Delete count = %d, want %d", deleteCount.Load(), numKeys)
	}

	// Verify no keys exist
	for i := 0; i < numKeys; i++ {
		key := "key-" + strconv.Itoa(i)
		exists, err := cache.Exists(key)
		if err != nil {
			t.Errorf("Exists(%v) error = %v", key, err)
		}
		if exists {
			t.Errorf("Exists(%v) = true after delete, want false", key)
		}
	}
}

// TestConcurrentShardAccess tests that sharding distributes load correctly
func TestConcurrentShardAccess(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	numGoroutines := 100
	numKeys := 10000

	var wg sync.WaitGroup
	wg.Add(numGoroutines)

	// Concurrent operations across many keys
	for i := 0; i < numGoroutines; i++ {
		go func(id int) {
			defer wg.Done()
			for j := 0; j < numKeys/numGoroutines; j++ {
				key := "shard-key-" + strconv.Itoa(id*numKeys/numGoroutines+j)
				value := "value-" + strconv.Itoa(j)

				// Set
				err := cache.Set(key, value)
				if err != nil {
					t.Errorf("Set(%v) error = %v", key, err)
				}

				// Get
				gotValue, exists, err := cache.Get(key)
				if err != nil || !exists || gotValue != value {
					t.Errorf("Get(%v) failed: value=%v, exists=%v, err=%v", key, gotValue, exists, err)
				}
			}
		}(i)
	}

	wg.Wait()

	// Verify all keys exist
	count := 0
	for i := 0; i < numKeys; i++ {
		key := "shard-key-" + strconv.Itoa(i)
		exists, err := cache.Exists(key)
		if err != nil {
			t.Errorf("Exists(%v) error = %v", key, err)
		}
		if exists {
			count++
		}
	}

	if count != numKeys {
		t.Errorf("Key count = %d, want %d", count, numKeys)
	}
}

// TestConcurrentTypeConflicts tests handling of type conflicts under concurrency
func TestConcurrentTypeConflicts(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "conflict-key"
	numGoroutines := 50

	var wg sync.WaitGroup
	wg.Add(numGoroutines * 4)

	// Different goroutines try different operations on the same key
	for i := 0; i < numGoroutines; i++ {
		// String operations
		go func() {
			defer wg.Done()
			cache.Set(key, "string-value")
		}()

		// Hash operations
		go func() {
			defer wg.Done()
			cache.HSet(key, "field", "value")
		}()

		// Set operations
		go func() {
			defer wg.Done()
			cache.SAdd(key, "member")
		}()

		// List operations
		go func() {
			defer wg.Done()
			cache.LPush(key, "element")
		}()
	}

	wg.Wait()

	// The key should have one consistent type
	// We just verify that operations don't crash
	exists, err := cache.Exists(key)
	if err != nil {
		t.Errorf("Exists() error = %v", err)
	}
	if !exists {
		t.Error("Key should exist after concurrent operations")
	}
}

// TestRaceConditionDetection runs with race detector to catch data races
func TestRaceConditionDetection(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "race-key"
	numGoroutines := 100

	var wg sync.WaitGroup
	wg.Add(numGoroutines)

	// Rapid concurrent access to the same key
	for i := 0; i < numGoroutines; i++ {
		go func(id int) {
			defer wg.Done()

			// Mix of reads and writes
			if id%2 == 0 {
				cache.Set(key, "value-"+strconv.Itoa(id))
			} else {
				cache.Get(key)
			}
		}(i)
	}

	wg.Wait()
}
