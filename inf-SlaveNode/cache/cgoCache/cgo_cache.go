package cgoCache

/*
#cgo CFLAGS: -I${SRCDIR}/../../../cache-Interface
#cgo LDFLAGS: -L${SRCDIR}/../../../build/cache-Interface -lcache_interface -lstdc++
#include "type_driver_c.h"
#include <stdlib.h>
*/
import "C"
import (
	"errors"
	"fmt"
	"hash/crc32"
	"unsafe"
)

// CGOCache implements the cache.Cache interface using C++ TypeDriver via CGO
type CGOCache struct {
	handle C.TypeDriverHandle
}

var (
	ErrNotInitialized    = errors.New("cache not initialized")
	ErrAlreadyInitialized = errors.New("cache already initialized")
	ErrInitFailed        = errors.New("cache initialization failed")
	ErrAllocationFailed  = errors.New("memory allocation failed")
	ErrNullPointer       = errors.New("null pointer error")
	ErrOperationFailed   = errors.New("operation failed")
	ErrKeyNotFound       = errors.New("key not found")
)

// NewCGOCache creates a new CGO cache instance
func NewCGOCache(maxBuddies uint) (*CGOCache, error) {
	handle := C.type_driver_create()
	if handle == nil {
		return nil, ErrAllocationFailed
	}

	cache := &CGOCache{handle: handle}

	ret := C.type_driver_init(handle, C.size_t(maxBuddies))
	if ret != C.C_TYPE_DRIVER_OK {
		C.type_driver_destroy(handle)
		return nil, fmt.Errorf("%w: error code %d", ErrInitFailed, ret)
	}

	return cache, nil
}

// Close releases the cache resources
func (c *CGOCache) Close() error {
	if c.handle != nil {
		C.type_driver_destroy(c.handle)
		c.handle = nil
	}
	return nil
}

// hashKey computes hash and CRC32 for a key string
func hashKey(key string) (uint64, uint32) {
	// Simple hash function (can be replaced with better hash)
	var hash uint64
	for _, ch := range key {
		hash = hash*31 + uint64(ch)
	}

	crc := crc32.ChecksumIEEE([]byte(key))
	return hash, crc
}

// ==================== KV Operations ====================

// Set implements KV Set operation
func (c *CGOCache) Set(key, value string) error {
	keyHash, keyCrc := hashKey(key)

	cData := C.CString(value)
	defer C.free(unsafe.Pointer(cData))

	ret := C.type_driver_kv_set(
		c.handle,
		C.uint64_t(keyHash),
		C.uint32_t(keyCrc),
		cData,
		C.size_t(len(value)),
	)

	if ret != C.C_TYPE_DRIVER_OK {
		return fmt.Errorf("%w: error code %d", ErrOperationFailed, ret)
	}

	return nil
}

// Get implements KV Get operation
func (c *CGOCache) Get(key string) (string, bool, error) {
	keyHash, keyCrc := hashKey(key)

	var retData *C.char
	var retLen C.size_t

	ret := C.type_driver_kv_get(
		c.handle,
		C.uint64_t(keyHash),
		C.uint32_t(keyCrc),
		&retData,
		&retLen,
	)

	if ret != C.C_TYPE_DRIVER_OK {
		// -4 means key not found at KVDriver level
		if ret == -4 || ret == -1 {
			return "", false, nil
		}
		return "", false, fmt.Errorf("%w: error code %d", ErrOperationFailed, ret)
	}

	if retData == nil {
		return "", false, nil
	}

	value := C.GoStringN(retData, C.int(retLen))
	return value, true, nil
}

// Del implements KV Del operation
func (c *CGOCache) Del(key string) (int, error) {
	keyHash, keyCrc := hashKey(key)

	ret := C.type_driver_kv_del(
		c.handle,
		C.uint64_t(keyHash),
		C.uint32_t(keyCrc),
	)

	if ret != C.C_TYPE_DRIVER_OK {
		// -4 means key not found
		if ret == -4 || ret == -1 {
			return 0, nil
		}
		return 0, fmt.Errorf("%w: error code %d", ErrOperationFailed, ret)
	}

	return 1, nil
}

// Exists implements KV Exists operation
func (c *CGOCache) Exists(key string) (bool, error) {
	keyHash, keyCrc := hashKey(key)

	ret := C.type_driver_kv_exists(
		c.handle,
		C.uint64_t(keyHash),
		C.uint32_t(keyCrc),
	)

	// -4 means key not found, -3 means CRC mismatch
	if ret == -4 || ret == -3 {
		return false, nil
	}

	if ret < 0 {
		return false, fmt.Errorf("%w: error code %d", ErrOperationFailed, ret)
	}

	return ret == 1, nil
}

// ==================== Hash Operations ====================

// HSet implements Hash Set single field operation
func (c *CGOCache) HSet(key, field, value string) (bool, error) {
	keyHash, keyCrc := hashKey(key)
	fieldHash, fieldCrc := hashKey(field)

	cField := C.CString(field)
	cValue := C.CString(value)
	defer C.free(unsafe.Pointer(cField))
	defer C.free(unsafe.Pointer(cValue))

	hashField := C.CHashField{
		field_hash:  C.uint64_t(fieldHash),
		field_crc32: C.uint32_t(fieldCrc),
		field:       cField,
		field_len:   C.size_t(len(field)),
		value:       cValue,
		value_len:   C.size_t(len(value)),
	}

	ret := C.type_driver_hash_set_m(
		c.handle,
		C.uint64_t(keyHash),
		C.uint32_t(keyCrc),
		&hashField,
	)

	if ret != C.C_TYPE_DRIVER_OK {
		return false, fmt.Errorf("%w: error code %d", ErrOperationFailed, ret)
	}

	return true, nil
}

// HGet implements Hash Get single field operation
func (c *CGOCache) HGet(key, field string) (string, bool, error) {
	keyHash, keyCrc := hashKey(key)
	fieldHash, fieldCrc := hashKey(field)

	cField := C.CString(field)
	defer C.free(unsafe.Pointer(cField))

	hashField := C.CHashField{
		field_hash:  C.uint64_t(fieldHash),
		field_crc32: C.uint32_t(fieldCrc),
		field:       cField,
		field_len:   C.size_t(len(field)),
		value:       nil,
		value_len:   0,
	}

	ret := C.type_driver_hash_get_m(
		c.handle,
		C.uint64_t(keyHash),
		C.uint32_t(keyCrc),
		&hashField,
	)

	if ret != C.C_TYPE_DRIVER_OK {
		// -4 means key/field not found
		if ret == -4 || ret == -1 {
			return "", false, nil
		}
		return "", false, fmt.Errorf("%w: error code %d", ErrOperationFailed, ret)
	}

	if hashField.value == nil {
		return "", false, nil
	}

	value := C.GoStringN(hashField.value, C.int(hashField.value_len))
	return value, true, nil
}

// HDel implements Hash Del single field operation
func (c *CGOCache) HDel(key, field string) (int, error) {
	keyHash, keyCrc := hashKey(key)
	fieldHash, fieldCrc := hashKey(field)

	ret := C.type_driver_hash_del_m(
		c.handle,
		C.uint64_t(keyHash),
		C.uint32_t(keyCrc),
		C.uint64_t(fieldHash),
		C.uint32_t(fieldCrc),
	)

	if ret != C.C_TYPE_DRIVER_OK {
		// -4 means not found
		if ret == -4 || ret == -1 {
			return 0, nil
		}
		return 0, fmt.Errorf("%w: error code %d", ErrOperationFailed, ret)
	}

	return 1, nil
}

// HMSet implements Hash Multi-Set operation (batch set multiple fields)
func (c *CGOCache) HMSet(key string, fields map[string]string) (int, error) {
	if len(fields) == 0 {
		return 0, nil
	}

	keyHash, keyCrc := hashKey(key)

	// Allocate C array for CHashField
	cFields := make([]C.CHashField, 0, len(fields))
	cStrings := make([]*C.char, 0, len(fields)*2)

	defer func() {
		for _, cstr := range cStrings {
			C.free(unsafe.Pointer(cstr))
		}
	}()

	for field, value := range fields {
		fieldHash, fieldCrc := hashKey(field)

		cField := C.CString(field)
		cValue := C.CString(value)
		cStrings = append(cStrings, cField, cValue)

		cFields = append(cFields, C.CHashField{
			field_hash:  C.uint64_t(fieldHash),
			field_crc32: C.uint32_t(fieldCrc),
			field:       cField,
			field_len:   C.size_t(len(field)),
			value:       cValue,
			value_len:   C.size_t(len(value)),
		})
	}

	ret := C.type_driver_hash_set(
		c.handle,
		C.uint64_t(keyHash),
		C.uint32_t(keyCrc),
		&cFields[0],
		C.size_t(len(cFields)),
	)

	if ret != C.C_TYPE_DRIVER_OK {
		return 0, fmt.Errorf("%w: error code %d", ErrOperationFailed, ret)
	}

	return len(fields), nil
}

// HMGet implements Hash Multi-Get operation
func (c *CGOCache) HMGet(key string, fields []string) (map[string]string, error) {
	result := make(map[string]string)

	for _, field := range fields {
		value, exists, err := c.HGet(key, field)
		if err != nil {
			return nil, err
		}
		if exists {
			result[field] = value
		}
	}

	return result, nil
}

// HLen implements Hash Length operation
func (c *CGOCache) HLen(key string) (int, error) {
	keyHash, keyCrc := hashKey(key)

	var length C.size_t

	ret := C.type_driver_hash_len(
		c.handle,
		C.uint64_t(keyHash),
		C.uint32_t(keyCrc),
		&length,
	)

	if ret != C.C_TYPE_DRIVER_OK {
		// -4 means not found
		if ret == -4 || ret == -1 {
			return 0, nil
		}
		return 0, fmt.Errorf("%w: error code %d", ErrOperationFailed, ret)
	}

	return int(length), nil
}

// HGetAll implements Hash GetAll operation
func (c *CGOCache) HGetAll(key string) (map[string]string, error) {
	keyHash, keyCrc := hashKey(key)

	var cFields *C.CHashField
	var fieldCount C.size_t

	ret := C.type_driver_hash_get(
		c.handle,
		C.uint64_t(keyHash),
		C.uint32_t(keyCrc),
		&cFields,
		&fieldCount,
	)

	if ret != C.C_TYPE_DRIVER_OK {
		if ret == -1 {
			return make(map[string]string), nil
		}
		return nil, fmt.Errorf("%w: error code %d", ErrOperationFailed, ret)
	}

	if cFields == nil || fieldCount == 0 {
		return make(map[string]string), nil
	}

	result := make(map[string]string, int(fieldCount))

	// Convert C array to Go slice
	fields := unsafe.Slice(cFields, int(fieldCount))

	for _, field := range fields {
		key := C.GoStringN(field.field, C.int(field.field_len))
		value := C.GoStringN(field.value, C.int(field.value_len))
		result[key] = value
	}

	return result, nil
}

// HExists implements Hash field Exists operation
func (c *CGOCache) HExists(key, field string) (bool, error) {
	keyHash, keyCrc := hashKey(key)
	fieldHash, fieldCrc := hashKey(field)

	ret := C.type_driver_hash_exists_m(
		c.handle,
		C.uint64_t(keyHash),
		C.uint32_t(keyCrc),
		C.uint64_t(fieldHash),
		C.uint32_t(fieldCrc),
	)

	// -4 means not found, -3 means CRC mismatch
	if ret == -4 || ret == -3 || ret == -5 {
		return false, nil
	}

	if ret < 0 {
		return false, fmt.Errorf("%w: error code %d", ErrOperationFailed, ret)
	}

	return ret == 1, nil
}

// ==================== Set Operations ====================

// SAdd implements Set Add operation
func (c *CGOCache) SAdd(key, member string) (bool, error) {
	keyHash, keyCrc := hashKey(key)
	memberHash, memberCrc := hashKey(member)

	cMember := C.CString(member)
	defer C.free(unsafe.Pointer(cMember))

	setMember := C.CSetMember{
		member_hash:  C.uint64_t(memberHash),
		member_crc32: C.uint32_t(memberCrc),
		member:       cMember,
		member_len:   C.size_t(len(member)),
	}

	ret := C.type_driver_set_add_m(
		c.handle,
		C.uint64_t(keyHash),
		C.uint32_t(keyCrc),
		&setMember,
	)

	if ret != C.C_TYPE_DRIVER_OK {
		return false, fmt.Errorf("%w: error code %d", ErrOperationFailed, ret)
	}

	return true, nil
}

// SMembers implements Set Members operation
func (c *CGOCache) SMembers(key string) ([]string, error) {
	keyHash, keyCrc := hashKey(key)

	var cMembers *C.CSetMember
	var memberCount C.size_t

	ret := C.type_driver_set_get(
		c.handle,
		C.uint64_t(keyHash),
		C.uint32_t(keyCrc),
		&cMembers,
		&memberCount,
	)

	if ret != C.C_TYPE_DRIVER_OK {
		if ret == -1 {
			return []string{}, nil
		}
		return nil, fmt.Errorf("%w: error code %d", ErrOperationFailed, ret)
	}

	if cMembers == nil || memberCount == 0 {
		return []string{}, nil
	}

	result := make([]string, 0, int(memberCount))

	// Convert C array to Go slice
	members := unsafe.Slice(cMembers, int(memberCount))

	for _, member := range members {
		m := C.GoStringN(member.member, C.int(member.member_len))
		result = append(result, m)
	}

	return result, nil
}

// SRem implements Set Remove operation
func (c *CGOCache) SRem(key, member string) (int, error) {
	keyHash, keyCrc := hashKey(key)
	memberHash, memberCrc := hashKey(member)

	ret := C.type_driver_set_del_m(
		c.handle,
		C.uint64_t(keyHash),
		C.uint32_t(keyCrc),
		C.uint64_t(memberHash),
		C.uint32_t(memberCrc),
	)

	if ret != C.C_TYPE_DRIVER_OK {
		// -4 means not found
		if ret == -4 || ret == -1 {
			return 0, nil
		}
		return 0, fmt.Errorf("%w: error code %d", ErrOperationFailed, ret)
	}

	return 1, nil
}

// SIsMember implements Set IsMember operation
func (c *CGOCache) SIsMember(key, member string) (bool, error) {
	keyHash, keyCrc := hashKey(key)
	memberHash, memberCrc := hashKey(member)

	ret := C.type_driver_set_exists_m(
		c.handle,
		C.uint64_t(keyHash),
		C.uint32_t(keyCrc),
		C.uint64_t(memberHash),
		C.uint32_t(memberCrc),
	)

	// -4 means not found, -3 means CRC mismatch, -5 means allocation failed
	if ret == -4 || ret == -3 || ret == -5 {
		return false, nil
	}

	if ret < 0 {
		return false, fmt.Errorf("%w: error code %d", ErrOperationFailed, ret)
	}

	return ret == 1, nil
}

// SCard implements Set Card (cardinality) operation
func (c *CGOCache) SCard(key string) (int, error) {
	keyHash, keyCrc := hashKey(key)

	var length C.size_t

	ret := C.type_driver_set_len(
		c.handle,
		C.uint64_t(keyHash),
		C.uint32_t(keyCrc),
		&length,
	)

	if ret != C.C_TYPE_DRIVER_OK {
		// -4 means not found
		if ret == -4 || ret == -1 {
			return 0, nil
		}
		return 0, fmt.Errorf("%w: error code %d", ErrOperationFailed, ret)
	}

	return int(length), nil
}

// ==================== List Operations (Not implemented in C interface) ====================

// List operations are not implemented in the C interface, returning errors

// LPush is not implemented in C interface
func (c *CGOCache) LPush(key, value string) (int, error) {
	return 0, errors.New("LPush not implemented in C interface")
}

// LRange is not implemented in C interface
func (c *CGOCache) LRange(key string, start, stop int) ([]string, error) {
	return nil, errors.New("LRange not implemented in C interface")
}

// LRem is not implemented in C interface
func (c *CGOCache) LRem(key string, count int, value string) (int, error) {
	return 0, errors.New("LRem not implemented in C interface")
}

// LPop is not implemented in C interface
func (c *CGOCache) LPop(key string) (string, bool, error) {
	return "", false, errors.New("LPop not implemented in C interface")
}

// RPush is not implemented in C interface
func (c *CGOCache) RPush(key, value string) (int, error) {
	return 0, errors.New("RPush not implemented in C interface")
}

// RPop is not implemented in C interface
func (c *CGOCache) RPop(key string) (string, bool, error) {
	return "", false, errors.New("RPop not implemented in C interface")
}

// LLen is not implemented in C interface
func (c *CGOCache) LLen(key string) (int, error) {
	return 0, errors.New("LLen not implemented in C interface")
}

// LIndex is not implemented in C interface
func (c *CGOCache) LIndex(key string, index int) (string, bool, error) {
	return "", false, errors.New("LIndex not implemented in C interface")
}

// LSet is not implemented in C interface
func (c *CGOCache) LSet(key string, index int, value string) error {
	return errors.New("LSet not implemented in C interface")
}
