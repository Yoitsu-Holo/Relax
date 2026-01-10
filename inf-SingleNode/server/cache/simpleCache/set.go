package simpleCache

import "errors"

// ========== Set Operations ==========

// SAdd adds a member to a set
func (sc *ShardedCache) SAdd(key, member string) (bool, error) {
	hash := hashKey(key)
	checksum := checksumKey(key)

	shard := sc.getShard(hash)
	shard.mu.Lock()
	defer shard.mu.Unlock()

	val, exists := shard.data[hash]

	if !exists {
		// Create new set
		val = &cacheValue{
			valueType: typeSet,
			checksum:  checksum,
			setValue:  make(map[string]struct{}),
		}
		shard.data[hash] = val
		val.setValue[member] = struct{}{}
		return true, nil
	}

	// Verify checksum
	if val.checksum != checksum {
		return false, errors.New("checksum mismatch: key collision detected")
	}

	if val.valueType != typeSet {
		return false, errors.New("WRONGTYPE: operation against a key holding the wrong kind of value")
	}

	_, memberExisted := val.setValue[member]
	val.setValue[member] = struct{}{}

	return !memberExisted, nil
}

// SMembers returns all members of a set
func (sc *ShardedCache) SMembers(key string) ([]string, error) {
	hash := hashKey(key)
	checksum := checksumKey(key)

	shard := sc.getShard(hash)
	shard.mu.RLock()
	val, exists := shard.data[hash]
	shard.mu.RUnlock()

	if !exists {
		return []string{}, nil
	}

	if val.checksum != checksum {
		return nil, errors.New("checksum mismatch: key collision detected")
	}

	if val.valueType != typeSet {
		return nil, errors.New("WRONGTYPE: operation against a key holding the wrong kind of value")
	}

	// Convert set to slice
	members := make([]string, 0, len(val.setValue))
	for member := range val.setValue {
		members = append(members, member)
	}

	return members, nil
}

// SRem removes a member from a set
func (sc *ShardedCache) SRem(key, member string) (int, error) {
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

	if val.valueType != typeSet {
		return 0, errors.New("WRONGTYPE: operation against a key holding the wrong kind of value")
	}

	_, memberExisted := val.setValue[member]
	delete(val.setValue, member)

	// Clean up if set is empty
	if len(val.setValue) == 0 {
		delete(shard.data, hash)
	}

	if memberExisted {
		return 1, nil
	}
	return 0, nil
}

// SIsMember checks if a member exists in a set
func (sc *ShardedCache) SIsMember(key, member string) (bool, error) {
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

	if val.valueType != typeSet {
		return false, errors.New("WRONGTYPE: operation against a key holding the wrong kind of value")
	}

	_, isMember := val.setValue[member]
	return isMember, nil
}

// SCard returns the number of members in a set
func (sc *ShardedCache) SCard(key string) (int, error) {
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

	if val.valueType != typeSet {
		return 0, errors.New("WRONGTYPE: operation against a key holding the wrong kind of value")
	}

	return len(val.setValue), nil
}
