#ifndef KV_ENGINE_H
#define KV_ENGINE_H

#include <cstddef>
#include <cstdint>
#include <cstring>
#include "../../lib/ankerl_unordered_dense/unordered_dense.h"

// Forward declaration
class RAlloc;

// Error codes
enum KVEngineError
{
    KV_OK = 0,
    KV_ERROR_INIT_FAILED = -1,
    KV_ERROR_NOT_INITIALIZED = -2,
    KV_ERROR_NULL_POINTER = -3,
    KV_ERROR_ALLOCATION_FAILED = -4,
    KV_ERROR_KEY_EXISTS = -5,
    KV_ERROR_KEY_NOT_FOUND = -6,
    KV_ERROR_INVALID_LENGTH = -7
};

// Value information structure
struct ValueInfo
{
    void *addr;
    size_t len;

    ValueInfo() : addr(nullptr), len(0) {}
    ValueInfo(void *a, size_t l) : addr(a), len(l) {}
};

// KVEngine - Key-Value Storage Engine
// Uses ralloc for memory allocation and ankerl::unordered_dense::map for indexing
// Calling chain: uint64 --ankerl_dense_map--> (addr, len) --direct memory access--> data(char*)
class KVEngine
{
public:
    KVEngine();
    ~KVEngine();

    // Disable copy and assignment
    KVEngine(const KVEngine &) = delete;
    KVEngine &operator=(const KVEngine &) = delete;

    // Initialize the KV engine
    // @param max_buddies: Maximum number of buddy allocators
    // @return KV_OK on success, error code on failure
    int init(size_t max_buddies = 64);

    // Add or update a key-value pair
    // If key exists, the old value will be replaced
    // @param hash: uint64 hash key
    // @param data: pointer to the data to store
    // @param len: length of the data
    // @return KV_OK on success, error code on failure
    int add(uint64_t hash, const char *data, size_t len);

    // Get value by key
    // @param hash: uint64 hash key
    // @param ret_data: output parameter, pointer to the data (do not free this pointer)
    // @param ret_len: output parameter, length of the data
    // @return KV_OK on success, error code on failure
    int get(uint64_t hash, char **ret_data, size_t *ret_len);

    // Delete a key-value pair
    // @param hash: uint64 hash key
    // @return KV_OK on success, error code on failure
    int del(uint64_t hash);

    // Statistics
    size_t get_size() const { return index_.size(); }
    size_t get_total_free_memory() const;
    size_t get_slab_count() const;
    size_t get_buddy_count() const;

    // Check if engine is initialized
    bool is_initialized() const { return initialized_; }

private:
    RAlloc *allocator_;                                     // Memory allocator
    ankerl::unordered_dense::map<uint64_t, ValueInfo> index_; // Hash -> (addr, len) mapping
    bool initialized_;
};

#endif // KV_ENGINE_H
