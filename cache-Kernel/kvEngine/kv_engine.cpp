#include "kv_engine.h"
#include "../ralloc/ralloc.h"

KVEngine::KVEngine()
    : allocator_(nullptr),
      initialized_(false)
{
}

KVEngine::~KVEngine()
{
    if (!initialized_)
        return;

    // Free all allocated memory
    for (auto &pair : index_)
    {
        if (pair.second.addr != nullptr)
        {
            allocator_->deallocate(pair.second.addr);
        }
    }

    // Clear the index
    index_.clear();

    // Delete allocator
    if (allocator_)
    {
        delete allocator_;
        allocator_ = nullptr;
    }

    initialized_ = false;
}

int KVEngine::init(size_t max_buddies)
{
    if (initialized_)
        return KV_ERROR_INIT_FAILED;

    // Create and initialize allocator
    allocator_ = new RAlloc();
    if (!allocator_)
        return KV_ERROR_INIT_FAILED;

    if (!allocator_->init(max_buddies))
    {
        delete allocator_;
        allocator_ = nullptr;
        return KV_ERROR_INIT_FAILED;
    }

    initialized_ = true;
    return KV_OK;
}

int KVEngine::add(uint64_t hash, const char *data, size_t len)
{
    if (!initialized_)
        return KV_ERROR_NOT_INITIALIZED;

    if (data == nullptr)
        return KV_ERROR_NULL_POINTER;

    if (len == 0)
        return KV_ERROR_INVALID_LENGTH;

    // Check if key already exists
    auto it = index_.find(hash);
    if (it != index_.end())
    {
        // Key exists, need to replace the value
        ValueInfo &old_value = it->second;

        // If the new value has the same length, reuse the memory
        if (old_value.len == len)
        {
            // Reuse existing memory
            std::memcpy(old_value.addr, data, len);
            return KV_OK;
        }
        else
        {
            // Free old memory
            if (old_value.addr != nullptr)
            {
                allocator_->deallocate(old_value.addr);
            }
            // Remove the old entry (will be replaced below)
            index_.erase(it);
        }
    }

    // Allocate new memory
    void *addr = allocator_->allocate(len);
    if (addr == nullptr)
        return KV_ERROR_ALLOCATION_FAILED;

    // Copy data
    std::memcpy(addr, data, len);

    // Add to index
    index_[hash] = ValueInfo(addr, len);

    return KV_OK;
}

int KVEngine::get(uint64_t hash, char **ret_data, size_t *ret_len)
{
    if (!initialized_)
        return KV_ERROR_NOT_INITIALIZED;

    if (ret_data == nullptr || ret_len == nullptr)
        return KV_ERROR_NULL_POINTER;

    // Find in index
    auto it = index_.find(hash);
    if (it == index_.end())
        return KV_ERROR_KEY_NOT_FOUND;

    // Return data pointer and length
    *ret_data = static_cast<char *>(it->second.addr);
    *ret_len = it->second.len;

    return KV_OK;
}

int KVEngine::del(uint64_t hash)
{
    if (!initialized_)
        return KV_ERROR_NOT_INITIALIZED;

    // Find in index
    auto it = index_.find(hash);
    if (it == index_.end())
        return KV_ERROR_KEY_NOT_FOUND;

    // Free memory
    if (it->second.addr != nullptr)
    {
        allocator_->deallocate(it->second.addr);
    }

    // Remove from index
    index_.erase(it);

    return KV_OK;
}

size_t KVEngine::get_total_free_memory() const
{
    if (!initialized_ || !allocator_)
        return 0;

    return allocator_->get_total_free_memory();
}

size_t KVEngine::get_slab_count() const
{
    if (!initialized_ || !allocator_)
        return 0;

    return allocator_->get_slab_count();
}

size_t KVEngine::get_buddy_count() const
{
    if (!initialized_ || !allocator_)
        return 0;

    return allocator_->get_buddy_count();
}
