package cache

import (
	"errors"
	"sync"
)

// SimpleMapCache implements Cache interface using a simple map with global lock
type SimpleMapCache struct {
	mu    sync.RWMutex
	store map[string]string
}

// NewSimpleMapCache creates a new SimpleMapCache instance
func NewSimpleMapCache() *SimpleMapCache {
	return &SimpleMapCache{
		store: make(map[string]string),
	}
}

// Set stores a key-value pair
func (c *SimpleMapCache) Set(key, value string) error {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.store[key] = value
	return nil
}

// Get retrieves a value by key
func (c *SimpleMapCache) Get(key string) (string, bool, error) {
	c.mu.RLock()
	defer c.mu.RUnlock()
	value, exists := c.store[key]
	return value, exists, nil
}

// Del deletes a key
func (c *SimpleMapCache) Del(key string) (int, error) {
	c.mu.Lock()
	defer c.mu.Unlock()

	_, existed := c.store[key]
	delete(c.store, key)

	if existed {
		return 1, nil
	}
	return 0, nil
}

// HSet stores a field-value pair in a hash
func (c *SimpleMapCache) HSet(key, field, value string) (bool, error) {
	compositeKey := key + ":" + field

	c.mu.Lock()
	defer c.mu.Unlock()

	_, existed := c.store[compositeKey]
	c.store[compositeKey] = value

	return !existed, nil
}

// HGet retrieves a field value from a hash
func (c *SimpleMapCache) HGet(key, field string) (string, bool, error) {
	compositeKey := key + ":" + field

	c.mu.RLock()
	defer c.mu.RUnlock()

	value, exists := c.store[compositeKey]
	return value, exists, nil
}

// HDel deletes a field from a hash
func (c *SimpleMapCache) HDel(key, field string) (int, error) {
	compositeKey := key + ":" + field

	c.mu.Lock()
	defer c.mu.Unlock()

	_, existed := c.store[compositeKey]
	delete(c.store, compositeKey)

	if existed {
		return 1, nil
	}
	return 0, nil
}

// SAdd adds a member to a set
func (c *SimpleMapCache) SAdd(key, member string) (bool, error) {
	compositeKey := key + ":set:" + member

	c.mu.Lock()
	defer c.mu.Unlock()

	_, existed := c.store[compositeKey]
	c.store[compositeKey] = member

	return !existed, nil
}

// SMembers returns all members of a set
func (c *SimpleMapCache) SMembers(key string) ([]string, error) {
	prefix := key + ":set:"

	c.mu.RLock()
	defer c.mu.RUnlock()

	var members []string
	for k, v := range c.store {
		if len(k) > len(prefix) && k[:len(prefix)] == prefix {
			members = append(members, v)
		}
	}

	return members, nil
}

// SRem removes a member from a set
func (c *SimpleMapCache) SRem(key, member string) (int, error) {
	compositeKey := key + ":set:" + member

	c.mu.Lock()
	defer c.mu.Unlock()

	_, existed := c.store[compositeKey]
	delete(c.store, compositeKey)

	if existed {
		return 1, nil
	}
	return 0, nil
}

// LPush pushes a value to the head of a list
func (c *SimpleMapCache) LPush(key, value string) (int, error) {
	return 0, errors.New("LPush requires additional list metadata implementation")
}

// LRange returns a range of elements from a list
func (c *SimpleMapCache) LRange(key string, start, stop int) ([]string, error) {
	return []string{}, errors.New("LRange requires additional list metadata implementation")
}

// LRem removes elements from a list
func (c *SimpleMapCache) LRem(key string, count int, value string) (int, error) {
	return 0, errors.New("LRem requires additional list metadata implementation")
}

// Close cleans up resources
func (c *SimpleMapCache) Close() error {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.store = nil
	return nil
}
