package simpleCache

import "errors"

// ========== Hash Operations ==========

// HSet stores a field-value pair in a hash
func (sc *ShardedCache) HSet(key, field, value string) (bool, error) {
	hash := hashKey(key)
	checksum := checksumKey(key)

	shard := sc.getShard(hash)
	shard.mu.Lock()
	defer shard.mu.Unlock()

	val, exists := shard.data[hash]

	if !exists {
		// Create new hash
		val = &cacheValue{
			valueType: typeHash,
			checksum:  checksum,
			hashValue: make(map[string]string),
		}
		shard.data[hash] = val
		val.hashValue[field] = value
		return true, nil
	}

	// Verify checksum
	if val.checksum != checksum {
		return false, errors.New("checksum mismatch: key collision detected")
	}

	if val.valueType != typeHash {
		return false, errors.New("WRONGTYPE: operation against a key holding the wrong kind of value")
	}

	_, fieldExisted := val.hashValue[field]
	val.hashValue[field] = value

	return !fieldExisted, nil
}

// HGet retrieves a field value from a hash
func (sc *ShardedCache) HGet(key, field string) (string, bool, error) {
	hash := hashKey(key)
	checksum := checksumKey(key)

	shard := sc.getShard(hash)
	shard.mu.RLock()
	val, exists := shard.data[hash]
	shard.mu.RUnlock()

	if !exists {
		return "", false, nil
	}

	if val.checksum != checksum {
		return "", false, errors.New("checksum mismatch: key collision detected")
	}

	if val.valueType != typeHash {
		return "", false, errors.New("WRONGTYPE: operation against a key holding the wrong kind of value")
	}

	fieldValue, fieldExists := val.hashValue[field]
	return fieldValue, fieldExists, nil
}

// HDel deletes a field from a hash
func (sc *ShardedCache) HDel(key, field string) (int, error) {
	hash := hashKey(key)
	checksum := checksumKey(key)

	shard := sc.getShard(hash)
	shard.mu.Lock()
	defer shard.mu.Unlock()

	val, exists := shard.data[hash]
	if !exists {
		return 0, nil
	}

	if val.checksum != checksum {
		return 0, errors.New("checksum mismatch: key collision detected")
	}

	if val.valueType != typeHash {
		return 0, errors.New("WRONGTYPE: operation against a key holding the wrong kind of value")
	}

	_, fieldExisted := val.hashValue[field]
	delete(val.hashValue, field)

	// Clean up if hash is empty
	if len(val.hashValue) == 0 {
		delete(shard.data, hash)
	}

	if fieldExisted {
		return 1, nil
	}
	return 0, nil
}

// HMSet sets multiple field-value pairs in a hash
func (sc *ShardedCache) HMSet(key string, fields map[string]string) (int, error) {
	hash := hashKey(key)
	checksum := checksumKey(key)

	shard := sc.getShard(hash)
	shard.mu.Lock()
	defer shard.mu.Unlock()

	val, exists := shard.data[hash]

	if !exists {
		// Create new hash
		val = &cacheValue{
			valueType: typeHash,
			checksum:  checksum,
			hashValue: make(map[string]string, len(fields)),
		}
		shard.data[hash] = val
	} else {
		// Verify checksum
		if val.checksum != checksum {
			return 0, errors.New("checksum mismatch: key collision detected")
		}

		if val.valueType != typeHash {
			return 0, errors.New("WRONGTYPE: operation against a key holding the wrong kind of value")
		}
	}

	// Set all fields
	for field, value := range fields {
		val.hashValue[field] = value
	}

	return len(fields), nil
}

// HMGet retrieves multiple field values from a hash
func (sc *ShardedCache) HMGet(key string, fields []string) (map[string]string, error) {
	hash := hashKey(key)
	checksum := checksumKey(key)

	shard := sc.getShard(hash)
	shard.mu.RLock()
	val, exists := shard.data[hash]
	shard.mu.RUnlock()

	result := make(map[string]string)

	if !exists {
		return result, nil
	}

	if val.checksum != checksum {
		return nil, errors.New("checksum mismatch: key collision detected")
	}

	if val.valueType != typeHash {
		return nil, errors.New("WRONGTYPE: operation against a key holding the wrong kind of value")
	}

	// Get all requested fields that exist
	for _, field := range fields {
		if value, ok := val.hashValue[field]; ok {
			result[field] = value
		}
	}

	return result, nil
}

// HLen returns the number of fields in a hash
func (sc *ShardedCache) HLen(key string) (int, error) {
	hash := hashKey(key)
	checksum := checksumKey(key)

	shard := sc.getShard(hash)
	shard.mu.RLock()
	val, exists := shard.data[hash]
	shard.mu.RUnlock()

	if !exists {
		return 0, nil
	}

	if val.checksum != checksum {
		return 0, errors.New("checksum mismatch: key collision detected")
	}

	if val.valueType != typeHash {
		return 0, errors.New("WRONGTYPE: operation against a key holding the wrong kind of value")
	}

	return len(val.hashValue), nil
}

// HGetAll retrieves all field-value pairs from a hash
func (sc *ShardedCache) HGetAll(key string) (map[string]string, error) {
	hash := hashKey(key)
	checksum := checksumKey(key)

	shard := sc.getShard(hash)
	shard.mu.RLock()
	val, exists := shard.data[hash]
	shard.mu.RUnlock()

	result := make(map[string]string)

	if !exists {
		return result, nil
	}

	if val.checksum != checksum {
		return nil, errors.New("checksum mismatch: key collision detected")
	}

	if val.valueType != typeHash {
		return nil, errors.New("WRONGTYPE: operation against a key holding the wrong kind of value")
	}

	// Copy all fields
	for field, value := range val.hashValue {
		result[field] = value
	}

	return result, nil
}

// HExists checks if a field exists in a hash
func (sc *ShardedCache) HExists(key, field string) (bool, error) {
	hash := hashKey(key)
	checksum := checksumKey(key)

	shard := sc.getShard(hash)
	shard.mu.RLock()
	val, exists := shard.data[hash]
	shard.mu.RUnlock()

	if !exists {
		return false, nil
	}

	if val.checksum != checksum {
		return false, errors.New("checksum mismatch: key collision detected")
	}

	if val.valueType != typeHash {
		return false, errors.New("WRONGTYPE: operation against a key holding the wrong kind of value")
	}

	_, fieldExists := val.hashValue[field]
	return fieldExists, nil
}
