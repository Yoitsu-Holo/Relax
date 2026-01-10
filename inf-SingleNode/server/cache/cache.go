package cache

// Cache defines the common cache interface supporting various data types
type Cache interface {
	// KV operations
	Set(key, value string) error
	Get(key string) (string, bool, error)
	Del(key string) (int, error)
	Exists(key string) (bool, error)

	// Hash operations
	HSet(key, field, value string) (bool, error)
	HGet(key, field string) (string, bool, error)
	HDel(key, field string) (int, error)
	HMSet(key string, fields map[string]string) (int, error)
	HMGet(key string, fields []string) (map[string]string, error)
	HLen(key string) (int, error)
	HGetAll(key string) (map[string]string, error)
	HExists(key, field string) (bool, error)

	// Set operations
	SAdd(key, member string) (bool, error)
	SMembers(key string) ([]string, error)
	SRem(key, member string) (int, error)
	SIsMember(key, member string) (bool, error)
	SCard(key string) (int, error)

	// List operations
	LPush(key, value string) (int, error)
	LRange(key string, start, stop int) ([]string, error)
	LRem(key string, count int, value string) (int, error)
	LPop(key string) (string, bool, error)
	RPush(key, value string) (int, error)
	RPop(key string) (string, bool, error)
	LLen(key string) (int, error)
	LIndex(key string, index int) (string, bool, error)
	LSet(key string, index int, value string) error

	// Resource management
	Close() error
}
