package simpleCache

import "errors"

// ========== KV Operations ==========

// Set stores a key-value pair
func (sc *ShardedCache) Set(key, value string) error {
	hash := hashKey(key)
	checksum := checksumKey(key)

	val := &cacheValue{
		valueType:   typeString,
		checksum:    checksum,
		stringValue: value,
	}

	shard := sc.getShard(hash)
	shard.mu.Lock()
	shard.data[hash] = val
	shard.mu.Unlock()

	return nil
}

// Get retrieves a value by key
func (sc *ShardedCache) Get(key string) (string, bool, error) {
	hash := hashKey(key)
	checksum := checksumKey(key)

	shard := sc.getShard(hash)
	shard.mu.RLock()
	val, exists := shard.data[hash]
	shard.mu.RUnlock()

	if !exists {
		return "", false, nil
	}

	// Verify checksum to ensure key integrity
	if val.checksum != checksum {
		return "", false, errors.New("checksum mismatch: key collision detected")
	}

	if val.valueType != typeString {
		return "", false, errors.New("WRONGTYPE: operation against a key holding the wrong kind of value")
	}

	return val.stringValue, true, nil
}

// Del deletes a key
func (sc *ShardedCache) Del(key string) (int, error) {
	hash := hashKey(key)

	shard := sc.getShard(hash)
	shard.mu.Lock()
	_, existed := shard.data[hash]
	delete(shard.data, hash)
	shard.mu.Unlock()

	if existed {
		return 1, nil
	}
	return 0, nil
}

// Exists checks if a key exists
func (sc *ShardedCache) Exists(key string) (bool, error) {
	hash := hashKey(key)

	shard := sc.getShard(hash)
	shard.mu.RLock()
	_, exists := shard.data[hash]
	shard.mu.RUnlock()

	return exists, nil
}
