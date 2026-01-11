package simpleCache

import "errors"

// ========== List Operations ==========

// LPush pushes a value to the head of a list
func (sc *ShardedCache) LPush(key, value string) (int, error) {
	hash := hashKey(key)
	checksum := checksumKey(key)

	shard := sc.getShard(hash)
	shard.mu.Lock()
	defer shard.mu.Unlock()

	val, exists := shard.data[hash]

	if !exists {
		// Create new list
		val = &cacheValue{
			valueType: typeList,
			checksum:  checksum,
			listValue: []string{value},
		}
		shard.data[hash] = val
		return 1, nil
	}

	// Verify checksum
	if val.checksum != checksum {
		return 0, errors.New("checksum mismatch: key collision detected")
	}

	if val.valueType != typeList {
		return 0, errors.New("WRONGTYPE: operation against a key holding the wrong kind of value")
	}

	// Prepend to list
	val.listValue = append([]string{value}, val.listValue...)

	return len(val.listValue), nil
}

// LRange returns a range of elements from a list
func (sc *ShardedCache) LRange(key string, start, stop int) ([]string, error) {
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

	if val.valueType != typeList {
		return nil, errors.New("WRONGTYPE: operation against a key holding the wrong kind of value")
	}

	listLen := len(val.listValue)
	if listLen == 0 {
		return []string{}, nil
	}

	// Convert negative indices
	if start < 0 {
		start = listLen + start
	}
	if stop < 0 {
		stop = listLen + stop
	}

	// Clamp to valid range
	if start < 0 {
		start = 0
	}
	if start >= listLen {
		return []string{}, nil
	}
	if stop >= listLen {
		stop = listLen - 1
	}
	if stop < start {
		return []string{}, nil
	}

	// Extract range
	result := make([]string, stop-start+1)
	copy(result, val.listValue[start:stop+1])

	return result, nil
}

// LRem removes elements from a list
func (sc *ShardedCache) LRem(key string, count int, value string) (int, error) {
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

	if val.valueType != typeList {
		return 0, errors.New("WRONGTYPE: operation against a key holding the wrong kind of value")
	}

	removed := 0
	newList := []string{}

	if count == 0 {
		// Remove all occurrences
		for _, item := range val.listValue {
			if item != value {
				newList = append(newList, item)
			} else {
				removed++
			}
		}
	} else if count > 0 {
		// Remove first N occurrences
		toRemove := count
		for _, item := range val.listValue {
			if item == value && toRemove > 0 {
				toRemove--
				removed++
			} else {
				newList = append(newList, item)
			}
		}
	} else {
		// Remove last N occurrences (count is negative)
		toRemove := -count
		// Traverse from end
		for i := len(val.listValue) - 1; i >= 0; i-- {
			item := val.listValue[i]
			if item == value && toRemove > 0 {
				toRemove--
				removed++
			} else {
				newList = append([]string{item}, newList...)
			}
		}
	}

	val.listValue = newList

	// Clean up if list is empty
	if len(val.listValue) == 0 {
		delete(shard.data, hash)
	}

	return removed, nil
}

// LPop removes and returns the first element from a list
func (sc *ShardedCache) LPop(key string) (string, bool, error) {
	hash := hashKey(key)
	checksum := checksumKey(key)

	shard := sc.getShard(hash)
	shard.mu.Lock()
	defer shard.mu.Unlock()

	val, exists := shard.data[hash]
	if !exists {
		return "", false, nil
	}

	if val.checksum != checksum {
		return "", false, errors.New("checksum mismatch: key collision detected")
	}

	if val.valueType != typeList {
		return "", false, errors.New("WRONGTYPE: operation against a key holding the wrong kind of value")
	}

	if len(val.listValue) == 0 {
		return "", false, nil
	}

	// Pop first element
	result := val.listValue[0]
	val.listValue = val.listValue[1:]

	// Clean up if list is empty
	if len(val.listValue) == 0 {
		delete(shard.data, hash)
	}

	return result, true, nil
}

// RPush pushes a value to the tail of a list
func (sc *ShardedCache) RPush(key, value string) (int, error) {
	hash := hashKey(key)
	checksum := checksumKey(key)

	shard := sc.getShard(hash)
	shard.mu.Lock()
	defer shard.mu.Unlock()

	val, exists := shard.data[hash]

	if !exists {
		// Create new list
		val = &cacheValue{
			valueType: typeList,
			checksum:  checksum,
			listValue: []string{value},
		}
		shard.data[hash] = val
		return 1, nil
	}

	// Verify checksum
	if val.checksum != checksum {
		return 0, errors.New("checksum mismatch: key collision detected")
	}

	if val.valueType != typeList {
		return 0, errors.New("WRONGTYPE: operation against a key holding the wrong kind of value")
	}

	// Append to list
	val.listValue = append(val.listValue, value)

	return len(val.listValue), nil
}

// RPop removes and returns the last element from a list
func (sc *ShardedCache) RPop(key string) (string, bool, error) {
	hash := hashKey(key)
	checksum := checksumKey(key)

	shard := sc.getShard(hash)
	shard.mu.Lock()
	defer shard.mu.Unlock()

	val, exists := shard.data[hash]
	if !exists {
		return "", false, nil
	}

	if val.checksum != checksum {
		return "", false, errors.New("checksum mismatch: key collision detected")
	}

	if val.valueType != typeList {
		return "", false, errors.New("WRONGTYPE: operation against a key holding the wrong kind of value")
	}

	if len(val.listValue) == 0 {
		return "", false, nil
	}

	// Pop last element
	lastIdx := len(val.listValue) - 1
	result := val.listValue[lastIdx]
	val.listValue = val.listValue[:lastIdx]

	// Clean up if list is empty
	if len(val.listValue) == 0 {
		delete(shard.data, hash)
	}

	return result, true, nil
}

// LLen returns the length of a list
func (sc *ShardedCache) LLen(key string) (int, error) {
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

	if val.valueType != typeList {
		return 0, errors.New("WRONGTYPE: operation against a key holding the wrong kind of value")
	}

	return len(val.listValue), nil
}

// LIndex returns the element at index in a list
func (sc *ShardedCache) LIndex(key string, index int) (string, bool, error) {
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

	if val.valueType != typeList {
		return "", false, errors.New("WRONGTYPE: operation against a key holding the wrong kind of value")
	}

	listLen := len(val.listValue)
	if listLen == 0 {
		return "", false, nil
	}

	// Convert negative index
	if index < 0 {
		index = listLen + index
	}

	// Check bounds
	if index < 0 || index >= listLen {
		return "", false, nil
	}

	return val.listValue[index], true, nil
}

// LSet sets the value at index in a list
func (sc *ShardedCache) LSet(key string, index int, value string) error {
	hash := hashKey(key)
	checksum := checksumKey(key)

	shard := sc.getShard(hash)
	shard.mu.Lock()
	defer shard.mu.Unlock()

	val, exists := shard.data[hash]
	if !exists {
		return errors.New("no such key")
	}

	if val.checksum != checksum {
		return errors.New("checksum mismatch: key collision detected")
	}

	if val.valueType != typeList {
		return errors.New("WRONGTYPE: operation against a key holding the wrong kind of value")
	}

	listLen := len(val.listValue)
	if listLen == 0 {
		return errors.New("index out of range")
	}

	// Convert negative index
	if index < 0 {
		index = listLen + index
	}

	// Check bounds
	if index < 0 || index >= listLen {
		return errors.New("index out of range")
	}

	val.listValue[index] = value
	return nil
}
