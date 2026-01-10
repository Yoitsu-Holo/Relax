package benchmark_test

import (
	"strconv"
	"testing"

	"github.com/yoitsuholo/relax/inf-SingleNode/server/cache/simpleCache"
)

// ========== KV Benchmarks ==========

// BenchmarkKVSet benchmarks Set operation
func BenchmarkKVSet(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		key := "key-" + strconv.Itoa(i)
		cache.Set(key, "value")
	}
}

// BenchmarkKVGet benchmarks Get operation
func BenchmarkKVGet(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	// Prepare data
	numKeys := 10000
	for i := 0; i < numKeys; i++ {
		key := "key-" + strconv.Itoa(i)
		cache.Set(key, "value")
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		key := "key-" + strconv.Itoa(i%numKeys)
		cache.Get(key)
	}
}

// BenchmarkKVSetParallel benchmarks parallel Set operations
func BenchmarkKVSetParallel(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	b.ResetTimer()
	b.RunParallel(func(pb *testing.PB) {
		i := 0
		for pb.Next() {
			key := "key-" + strconv.Itoa(i)
			cache.Set(key, "value")
			i++
		}
	})
}

// BenchmarkKVGetParallel benchmarks parallel Get operations
func BenchmarkKVGetParallel(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	// Prepare data
	numKeys := 10000
	for i := 0; i < numKeys; i++ {
		key := "key-" + strconv.Itoa(i)
		cache.Set(key, "value")
	}

	b.ResetTimer()
	b.RunParallel(func(pb *testing.PB) {
		i := 0
		for pb.Next() {
			key := "key-" + strconv.Itoa(i%numKeys)
			cache.Get(key)
			i++
		}
	})
}

// BenchmarkKVMixed benchmarks mixed read/write operations
func BenchmarkKVMixed(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	// Prepare data
	numKeys := 10000
	for i := 0; i < numKeys; i++ {
		key := "key-" + strconv.Itoa(i)
		cache.Set(key, "value")
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		key := "key-" + strconv.Itoa(i%numKeys)
		if i%4 == 0 {
			cache.Set(key, "new-value")
		} else {
			cache.Get(key)
		}
	}
}

// ========== Hash Benchmarks ==========

// BenchmarkHashHSet benchmarks HSet operation
func BenchmarkHashHSet(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "hash-key"

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		field := "field-" + strconv.Itoa(i)
		cache.HSet(key, field, "value")
	}
}

// BenchmarkHashHGet benchmarks HGet operation
func BenchmarkHashHGet(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "hash-key"
	numFields := 10000

	// Prepare data
	for i := 0; i < numFields; i++ {
		field := "field-" + strconv.Itoa(i)
		cache.HSet(key, field, "value")
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		field := "field-" + strconv.Itoa(i%numFields)
		cache.HGet(key, field)
	}
}

// BenchmarkHashHSetParallel benchmarks parallel HSet operations
func BenchmarkHashHSetParallel(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	b.ResetTimer()
	b.RunParallel(func(pb *testing.PB) {
		i := 0
		for pb.Next() {
			key := "hash-" + strconv.Itoa(i%100)
			field := "field-" + strconv.Itoa(i)
			cache.HSet(key, field, "value")
			i++
		}
	})
}

// BenchmarkHashHGetParallel benchmarks parallel HGet operations
func BenchmarkHashHGetParallel(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	// Prepare data
	numHashes := 100
	numFields := 100
	for i := 0; i < numHashes; i++ {
		key := "hash-" + strconv.Itoa(i)
		for j := 0; j < numFields; j++ {
			field := "field-" + strconv.Itoa(j)
			cache.HSet(key, field, "value")
		}
	}

	b.ResetTimer()
	b.RunParallel(func(pb *testing.PB) {
		i := 0
		for pb.Next() {
			key := "hash-" + strconv.Itoa(i%numHashes)
			field := "field-" + strconv.Itoa((i/numHashes)%numFields)
			cache.HGet(key, field)
			i++
		}
	})
}

// BenchmarkHashHMSet benchmarks HMSet operation
func BenchmarkHashHMSet(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	fields := make(map[string]string)
	for i := 0; i < 10; i++ {
		fields["field-"+strconv.Itoa(i)] = "value-" + strconv.Itoa(i)
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		key := "hash-" + strconv.Itoa(i)
		cache.HMSet(key, fields)
	}
}

// BenchmarkHashHGetAll benchmarks HGetAll operation
func BenchmarkHashHGetAll(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "hash-key"
	numFields := 100

	// Prepare data
	for i := 0; i < numFields; i++ {
		field := "field-" + strconv.Itoa(i)
		cache.HSet(key, field, "value-"+strconv.Itoa(i))
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		cache.HGetAll(key)
	}
}

// ========== Set Benchmarks ==========

// BenchmarkSetSAdd benchmarks SAdd operation
func BenchmarkSetSAdd(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "set-key"

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		member := "member-" + strconv.Itoa(i)
		cache.SAdd(key, member)
	}
}

// BenchmarkSetSIsMember benchmarks SIsMember operation
func BenchmarkSetSIsMember(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "set-key"
	numMembers := 10000

	// Prepare data
	for i := 0; i < numMembers; i++ {
		member := "member-" + strconv.Itoa(i)
		cache.SAdd(key, member)
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		member := "member-" + strconv.Itoa(i%numMembers)
		cache.SIsMember(key, member)
	}
}

// BenchmarkSetSAddParallel benchmarks parallel SAdd operations
func BenchmarkSetSAddParallel(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	b.ResetTimer()
	b.RunParallel(func(pb *testing.PB) {
		i := 0
		for pb.Next() {
			key := "set-" + strconv.Itoa(i%100)
			member := "member-" + strconv.Itoa(i)
			cache.SAdd(key, member)
			i++
		}
	})
}

// BenchmarkSetSMembers benchmarks SMembers operation
func BenchmarkSetSMembers(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "set-key"
	numMembers := 1000

	// Prepare data
	for i := 0; i < numMembers; i++ {
		member := "member-" + strconv.Itoa(i)
		cache.SAdd(key, member)
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		cache.SMembers(key)
	}
}

// ========== List Benchmarks ==========

// BenchmarkListLPush benchmarks LPush operation
func BenchmarkListLPush(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "list-key"

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		value := "value-" + strconv.Itoa(i)
		cache.LPush(key, value)
	}
}

// BenchmarkListRPush benchmarks RPush operation
func BenchmarkListRPush(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "list-key"

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		value := "value-" + strconv.Itoa(i)
		cache.RPush(key, value)
	}
}

// BenchmarkListLPushParallel benchmarks parallel LPush operations
func BenchmarkListLPushParallel(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	b.ResetTimer()
	b.RunParallel(func(pb *testing.PB) {
		i := 0
		for pb.Next() {
			key := "list-" + strconv.Itoa(i%100)
			value := "value-" + strconv.Itoa(i)
			cache.LPush(key, value)
			i++
		}
	})
}

// BenchmarkListLRange benchmarks LRange operation
func BenchmarkListLRange(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "list-key"
	numElements := 1000

	// Prepare data
	for i := 0; i < numElements; i++ {
		cache.RPush(key, "value-"+strconv.Itoa(i))
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		cache.LRange(key, 0, 99)
	}
}

// BenchmarkListLPop benchmarks LPop operation
func BenchmarkListLPop(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "list-key"

	// Prepare data
	for i := 0; i < b.N; i++ {
		cache.RPush(key, "value")
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		cache.LPop(key)
	}
}

// BenchmarkListRPop benchmarks RPop operation
func BenchmarkListRPop(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "list-key"

	// Prepare data
	for i := 0; i < b.N; i++ {
		cache.RPush(key, "value")
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		cache.RPop(key)
	}
}

// ========== Sharding Benchmarks ==========

// BenchmarkSharding8 benchmarks with 8 shards
func BenchmarkSharding8(b *testing.B) {
	cache := simpleCache.NewShardedCache(8)
	defer cache.Close()

	b.ResetTimer()
	b.RunParallel(func(pb *testing.PB) {
		i := 0
		for pb.Next() {
			key := "key-" + strconv.Itoa(i)
			cache.Set(key, "value")
			i++
		}
	})
}

// BenchmarkSharding32 benchmarks with 32 shards
func BenchmarkSharding32(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	b.ResetTimer()
	b.RunParallel(func(pb *testing.PB) {
		i := 0
		for pb.Next() {
			key := "key-" + strconv.Itoa(i)
			cache.Set(key, "value")
			i++
		}
	})
}

// BenchmarkSharding64 benchmarks with 64 shards
func BenchmarkSharding64(b *testing.B) {
	cache := simpleCache.NewShardedCache(64)
	defer cache.Close()

	b.ResetTimer()
	b.RunParallel(func(pb *testing.PB) {
		i := 0
		for pb.Next() {
			key := "key-" + strconv.Itoa(i)
			cache.Set(key, "value")
			i++
		}
	})
}

// BenchmarkSharding128 benchmarks with 128 shards
func BenchmarkSharding128(b *testing.B) {
	cache := simpleCache.NewShardedCache(128)
	defer cache.Close()

	b.ResetTimer()
	b.RunParallel(func(pb *testing.PB) {
		i := 0
		for pb.Next() {
			key := "key-" + strconv.Itoa(i)
			cache.Set(key, "value")
			i++
		}
	})
}

// ========== Memory and Scalability Benchmarks ==========

// BenchmarkMemoryFootprint benchmarks memory usage with large datasets
func BenchmarkMemoryFootprint(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		key := "key-" + strconv.Itoa(i)
		value := "value-with-some-reasonable-length-" + strconv.Itoa(i)
		cache.Set(key, value)
	}
}

// BenchmarkHighConcurrency benchmarks high concurrency scenario
func BenchmarkHighConcurrency(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	// Prepare data
	numKeys := 10000
	for i := 0; i < numKeys; i++ {
		key := "key-" + strconv.Itoa(i)
		cache.Set(key, "value")
	}

	b.ResetTimer()
	b.RunParallel(func(pb *testing.PB) {
		i := 0
		for pb.Next() {
			key := "key-" + strconv.Itoa(i%numKeys)
			if i%5 == 0 {
				cache.Set(key, "new-value")
			} else {
				cache.Get(key)
			}
			i++
		}
	})
}

// BenchmarkContention benchmarks contention on hot keys
func BenchmarkContention(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	// Use only a few hot keys
	numHotKeys := 10
	for i := 0; i < numHotKeys; i++ {
		key := "hot-key-" + strconv.Itoa(i)
		cache.Set(key, "value")
	}

	b.ResetTimer()
	b.RunParallel(func(pb *testing.PB) {
		i := 0
		for pb.Next() {
			key := "hot-key-" + strconv.Itoa(i%numHotKeys)
			if i%3 == 0 {
				cache.Set(key, "new-value")
			} else {
				cache.Get(key)
			}
			i++
		}
	})
}

// ========== Comprehensive Workload Benchmarks ==========

// BenchmarkRealisticWorkload simulates a realistic workload
func BenchmarkRealisticWorkload(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	// Prepare initial data
	numKeys := 1000
	for i := 0; i < numKeys; i++ {
		key := "key-" + strconv.Itoa(i)
		cache.Set(key, "value-"+strconv.Itoa(i))

		// Add some hashes
		if i%5 == 0 {
			hashKey := "hash-" + strconv.Itoa(i)
			cache.HSet(hashKey, "field1", "value1")
			cache.HSet(hashKey, "field2", "value2")
		}

		// Add some sets
		if i%7 == 0 {
			setKey := "set-" + strconv.Itoa(i)
			cache.SAdd(setKey, "member1")
			cache.SAdd(setKey, "member2")
		}

		// Add some lists
		if i%11 == 0 {
			listKey := "list-" + strconv.Itoa(i)
			cache.LPush(listKey, "element1")
			cache.LPush(listKey, "element2")
		}
	}

	b.ResetTimer()
	b.RunParallel(func(pb *testing.PB) {
		i := 0
		for pb.Next() {
			op := i % 20
			keyIdx := i % numKeys

			switch {
			case op < 10: // 50% KV reads
				key := "key-" + strconv.Itoa(keyIdx)
				cache.Get(key)
			case op < 12: // 10% KV writes
				key := "key-" + strconv.Itoa(keyIdx)
				cache.Set(key, "new-value")
			case op < 15: // 15% Hash operations
				hashKey := "hash-" + strconv.Itoa(keyIdx)
				cache.HGet(hashKey, "field1")
			case op < 17: // 10% Set operations
				setKey := "set-" + strconv.Itoa(keyIdx)
				cache.SIsMember(setKey, "member1")
			case op < 20: // 15% List operations
				listKey := "list-" + strconv.Itoa(keyIdx)
				cache.LRange(listKey, 0, 10)
			}
			i++
		}
	})
}

// ========== Comparative Benchmarks ==========

// BenchmarkCompareReadHeavy compares read-heavy workload
func BenchmarkCompareReadHeavy(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	// Prepare data
	numKeys := 10000
	for i := 0; i < numKeys; i++ {
		cache.Set("key-"+strconv.Itoa(i), "value")
	}

	b.ResetTimer()
	b.RunParallel(func(pb *testing.PB) {
		i := 0
		for pb.Next() {
			// 95% reads, 5% writes
			if i%20 == 0 {
				cache.Set("key-"+strconv.Itoa(i%numKeys), "new-value")
			} else {
				cache.Get("key-" + strconv.Itoa(i%numKeys))
			}
			i++
		}
	})
}

// BenchmarkCompareWriteHeavy compares write-heavy workload
func BenchmarkCompareWriteHeavy(b *testing.B) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	// Prepare initial data
	numKeys := 10000
	for i := 0; i < numKeys; i++ {
		cache.Set("key-"+strconv.Itoa(i), "value")
	}

	b.ResetTimer()
	b.RunParallel(func(pb *testing.PB) {
		i := 0
		for pb.Next() {
			// 80% writes, 20% reads
			if i%5 < 4 {
				cache.Set("key-"+strconv.Itoa(i%numKeys), "new-value")
			} else {
				cache.Get("key-" + strconv.Itoa(i%numKeys))
			}
			i++
		}
	})
}
