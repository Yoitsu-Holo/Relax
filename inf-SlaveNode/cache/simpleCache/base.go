package simpleCache

import (
	"hash/crc32"
	"sync"

	"github.com/zeebo/xxh3"
)

const (
	// DefaultShardCount is the default number of shards (must be power of 2)
	DefaultShardCount = 32
)

// Data type identifiers
type dataType int

const (
	typeString dataType = iota
	typeHash
	typeSet
	typeList
)

// cacheValue represents a value of any type
type cacheValue struct {
	valueType dataType
	checksum  uint32 // CRC32 checksum of original key

	// Type-specific data
	stringValue string
	hashValue   map[string]string
	setValue    map[string]struct{}
	listValue   []string
}

// cacheShard represents a single shard with independent lock
type cacheShard struct {
	mu   sync.RWMutex
	data map[uint64]*cacheValue
}

// ShardedCache implements Cache interface with sharding for better concurrency
type ShardedCache struct {
	shards    []*cacheShard
	shardMask uint64
	shardBits uint
}

// NewShardedCache creates a new sharded cache with specified number of shards
// shardCount must be a power of 2
func NewShardedCache(shardCount int) *ShardedCache {
	if shardCount <= 0 || (shardCount&(shardCount-1)) != 0 {
		shardCount = DefaultShardCount
	}

	shards := make([]*cacheShard, shardCount)
	for i := 0; i < shardCount; i++ {
		shards[i] = &cacheShard{
			data: make(map[uint64]*cacheValue),
		}
	}

	// Calculate shard mask and bits for fast modulo operation
	shardBits := uint(0)
	for (1 << shardBits) < shardCount {
		shardBits++
	}

	return &ShardedCache{
		shards:    shards,
		shardMask: uint64(shardCount - 1),
		shardBits: shardBits,
	}
}

// hashKey computes xxhash128 of the key and returns the lower 64 bits
func hashKey(key string) uint64 {
	hash128 := xxh3.Hash128([]byte(key))
	// Return the lower 64 bits of the 128-bit hash
	return hash128.Lo
}

// checksumKey computes CRC32 checksum of the key for data integrity validation
func checksumKey(key string) uint32 {
	return crc32.ChecksumIEEE([]byte(key))
}

// getShard returns the shard for a given hash using binary masking
func (sc *ShardedCache) getShard(hash uint64) *cacheShard {
	shardIdx := hash & sc.shardMask
	return sc.shards[shardIdx]
}

// Close cleans up resources
func (sc *ShardedCache) Close() error {
	// Clear all shards
	for _, shard := range sc.shards {
		shard.mu.Lock()
		shard.data = nil
		shard.mu.Unlock()
	}
	return nil
}
