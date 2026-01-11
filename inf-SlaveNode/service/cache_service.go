package service

import (
	"context"

	"github.com/yoitsuholo/relax/inf-SlaveNode/cache"
	"github.com/yoitsuholo/relax/inf-SlaveNode/cache/simpleCache"
	pb "github.com/yoitsuholo/relax/proto"
)

// CacheServer implements the CacheService gRPC service
type CacheServer struct {
	pb.UnimplementedCacheServiceServer
	cache cache.Cache
}

// NewCacheServer creates a new cache server with sharded cache as default
func NewCacheServer() *CacheServer {
	return &CacheServer{
		cache: simpleCache.NewShardedCache(simpleCache.DefaultShardCount),
	}
}

// NewCacheServerWithShards creates a new cache server with custom shard count
func NewCacheServerWithShards(shardCount int) *CacheServer {
	return &CacheServer{
		cache: simpleCache.NewShardedCache(shardCount),
	}
}

// NewCacheServerWithCache creates a new cache server with custom cache implementation
func NewCacheServerWithCache(c cache.Cache) *CacheServer {
	return &CacheServer{
		cache: c,
	}
}

// ========== KV Native Operations ==========

// KvSet implements native KV Set operation
func (s *CacheServer) KvSet(ctx context.Context, req *pb.KvSetRequest) (*pb.KvSetResponse, error) {
	err := s.cache.Set(req.Key, req.Value)
	return &pb.KvSetResponse{Success: err == nil}, err
}

// KvGet implements native KV Get operation
func (s *CacheServer) KvGet(ctx context.Context, req *pb.KvGetRequest) (*pb.KvGetResponse, error) {
	value, exists, err := s.cache.Get(req.Key)
	if err != nil {
		return nil, err
	}
	return &pb.KvGetResponse{
		Value:  value,
		Exists: exists,
	}, nil
}

// KvDel implements native KV Del operation
func (s *CacheServer) KvDel(ctx context.Context, req *pb.KvDelRequest) (*pb.KvDelResponse, error) {
	count, err := s.cache.Del(req.Key)
	if err != nil {
		return nil, err
	}
	return &pb.KvDelResponse{
		Success:      true,
		DeletedCount: int32(count),
	}, nil
}

// KvExists implements native KV Exists operation
func (s *CacheServer) KvExists(ctx context.Context, req *pb.KvExistsRequest) (*pb.KvExistsResponse, error) {
	exists, err := s.cache.Exists(req.Key)
	if err != nil {
		return nil, err
	}
	return &pb.KvExistsResponse{
		Exists: exists,
	}, nil
}

// ========== Hash Native Operations - Batch/Whole Hash ==========

// HashSet implements native Hash batch set operation
func (s *CacheServer) HashSet(ctx context.Context, req *pb.HashSetRequest) (*pb.HashSetResponse, error) {
	count, err := s.cache.HMSet(req.Key, req.Fields)
	if err != nil {
		return nil, err
	}
	return &pb.HashSetResponse{
		Success: true,
		Count:   int32(count),
	}, nil
}

// HashGet implements native Hash get all fields operation
func (s *CacheServer) HashGet(ctx context.Context, req *pb.HashGetRequest) (*pb.HashGetResponse, error) {
	fields, err := s.cache.HGetAll(req.Key)
	if err != nil {
		return nil, err
	}
	return &pb.HashGetResponse{
		Fields: fields,
	}, nil
}

// HashDel implements native Hash delete entire hash operation
func (s *CacheServer) HashDel(ctx context.Context, req *pb.HashDelRequest) (*pb.HashDelResponse, error) {
	count, err := s.cache.Del(req.Key)
	if err != nil {
		return nil, err
	}
	return &pb.HashDelResponse{
		Success:      true,
		DeletedCount: int32(count),
	}, nil
}

// HashExists implements native Hash key exists check operation
func (s *CacheServer) HashExists(ctx context.Context, req *pb.HashExistsRequest) (*pb.HashExistsResponse, error) {
	exists, err := s.cache.Exists(req.Key)
	if err != nil {
		return nil, err
	}
	return &pb.HashExistsResponse{
		Exists: exists,
	}, nil
}

// HashLen implements native Hash length operation
func (s *CacheServer) HashLen(ctx context.Context, req *pb.HashLenRequest) (*pb.HashLenResponse, error) {
	length, err := s.cache.HLen(req.Key)
	if err != nil {
		return nil, err
	}
	return &pb.HashLenResponse{
		Length: int32(length),
	}, nil
}

// ========== Hash Native Operations - Single Field ==========

// HashSetM implements native Hash set single field operation
func (s *CacheServer) HashSetM(ctx context.Context, req *pb.HashSetMRequest) (*pb.HashSetMResponse, error) {
	created, err := s.cache.HSet(req.Key, req.Field, req.Value)
	if err != nil {
		return nil, err
	}
	return &pb.HashSetMResponse{
		Success: true,
		Created: created,
	}, nil
}

// HashGetM implements native Hash get single field operation
func (s *CacheServer) HashGetM(ctx context.Context, req *pb.HashGetMRequest) (*pb.HashGetMResponse, error) {
	value, exists, err := s.cache.HGet(req.Key, req.Field)
	if err != nil {
		return nil, err
	}
	return &pb.HashGetMResponse{
		Value:  value,
		Exists: exists,
	}, nil
}

// HashDelM implements native Hash delete single field operation
func (s *CacheServer) HashDelM(ctx context.Context, req *pb.HashDelMRequest) (*pb.HashDelMResponse, error) {
	count, err := s.cache.HDel(req.Key, req.Field)
	if err != nil {
		return nil, err
	}
	return &pb.HashDelMResponse{
		Success:      true,
		DeletedCount: int32(count),
	}, nil
}

// HashExistsM implements native Hash field exists check operation
func (s *CacheServer) HashExistsM(ctx context.Context, req *pb.HashExistsMRequest) (*pb.HashExistsMResponse, error) {
	exists, err := s.cache.HExists(req.Key, req.Field)
	if err != nil {
		return nil, err
	}
	return &pb.HashExistsMResponse{
		Exists: exists,
	}, nil
}

// ========== Set Native Operations - Batch/Whole Set ==========

// SetSet implements native Set batch add members operation
func (s *CacheServer) SetSet(ctx context.Context, req *pb.SetSetRequest) (*pb.SetSetResponse, error) {
	count := 0
	for _, member := range req.Members {
		added, err := s.cache.SAdd(req.Key, member)
		if err != nil {
			return nil, err
		}
		if added {
			count++
		}
	}
	return &pb.SetSetResponse{
		Success: true,
		Count:   int32(count),
	}, nil
}

// SetGet implements native Set get all members operation
func (s *CacheServer) SetGet(ctx context.Context, req *pb.SetGetRequest) (*pb.SetGetResponse, error) {
	members, err := s.cache.SMembers(req.Key)
	if err != nil {
		return nil, err
	}
	return &pb.SetGetResponse{
		Members: members,
	}, nil
}

// SetDel implements native Set delete entire set operation
func (s *CacheServer) SetDel(ctx context.Context, req *pb.SetDelRequest) (*pb.SetDelResponse, error) {
	count, err := s.cache.Del(req.Key)
	if err != nil {
		return nil, err
	}
	return &pb.SetDelResponse{
		Success:      true,
		DeletedCount: int32(count),
	}, nil
}

// SetExists implements native Set key exists check operation
func (s *CacheServer) SetExists(ctx context.Context, req *pb.SetExistsRequest) (*pb.SetExistsResponse, error) {
	exists, err := s.cache.Exists(req.Key)
	if err != nil {
		return nil, err
	}
	return &pb.SetExistsResponse{
		Exists: exists,
	}, nil
}

// SetLen implements native Set length operation
func (s *CacheServer) SetLen(ctx context.Context, req *pb.SetLenRequest) (*pb.SetLenResponse, error) {
	length, err := s.cache.SCard(req.Key)
	if err != nil {
		return nil, err
	}
	return &pb.SetLenResponse{
		Length: int32(length),
	}, nil
}

// ========== Set Native Operations - Single Member ==========

// SetAddM implements native Set add single member operation
func (s *CacheServer) SetAddM(ctx context.Context, req *pb.SetAddMRequest) (*pb.SetAddMResponse, error) {
	added, err := s.cache.SAdd(req.Key, req.Member)
	if err != nil {
		return nil, err
	}
	return &pb.SetAddMResponse{
		Success: true,
		Added:   added,
	}, nil
}

// SetExistsM implements native Set member exists check operation
func (s *CacheServer) SetExistsM(ctx context.Context, req *pb.SetExistsMRequest) (*pb.SetExistsMResponse, error) {
	isMember, err := s.cache.SIsMember(req.Key, req.Member)
	if err != nil {
		return nil, err
	}
	return &pb.SetExistsMResponse{
		IsMember: isMember,
	}, nil
}

// SetDelM implements native Set delete single member operation
func (s *CacheServer) SetDelM(ctx context.Context, req *pb.SetDelMRequest) (*pb.SetDelMResponse, error) {
	count, err := s.cache.SRem(req.Key, req.Member)
	if err != nil {
		return nil, err
	}
	return &pb.SetDelMResponse{
		Success:      true,
		DeletedCount: int32(count),
	}, nil
}

// SetGetM implements native Set get single member operation (for verification)
func (s *CacheServer) SetGetM(ctx context.Context, req *pb.SetGetMRequest) (*pb.SetGetMResponse, error) {
	exists, err := s.cache.SIsMember(req.Key, req.Member)
	if err != nil {
		return nil, err
	}
	return &pb.SetGetMResponse{
		Value:  req.Member,
		Exists: exists,
	}, nil
}

// ========== KV Redis Aliases ==========

// Set implements KV Set operation (alias for KvSet)
func (s *CacheServer) Set(ctx context.Context, req *pb.KvSetRequest) (*pb.KvSetResponse, error) {
	return s.KvSet(ctx, req)
}

// Get implements KV Get operation (alias for KvGet)
func (s *CacheServer) Get(ctx context.Context, req *pb.KvGetRequest) (*pb.KvGetResponse, error) {
	return s.KvGet(ctx, req)
}

// Del implements KV Del operation (alias for KvDel)
func (s *CacheServer) Del(ctx context.Context, req *pb.KvDelRequest) (*pb.KvDelResponse, error) {
	return s.KvDel(ctx, req)
}

// Exists implements KV Exists operation (alias for KvExists)
func (s *CacheServer) Exists(ctx context.Context, req *pb.KvExistsRequest) (*pb.KvExistsResponse, error) {
	return s.KvExists(ctx, req)
}

// ========== Hash Redis Aliases ==========

// HMSet implements Hash Multi-Set operation (alias for HashSetM)
func (s *CacheServer) HMSet(ctx context.Context, req *pb.HashSetMRequest) (*pb.HashSetMResponse, error) {
	return s.HashSetM(ctx, req)
}

// HMGet implements Hash Multi-Get operation (alias for HashGetM)
func (s *CacheServer) HMGet(ctx context.Context, req *pb.HashGetMRequest) (*pb.HashGetMResponse, error) {
	return s.HashGetM(ctx, req)
}

// HLen implements Hash Length operation (alias for HashLen)
func (s *CacheServer) HLen(ctx context.Context, req *pb.HashLenRequest) (*pb.HashLenResponse, error) {
	return s.HashLen(ctx, req)
}

// HGetAll implements Hash GetAll operation (alias for HashGet)
func (s *CacheServer) HGetAll(ctx context.Context, req *pb.HashGetRequest) (*pb.HashGetResponse, error) {
	return s.HashGet(ctx, req)
}

// HDel implements Hash Del operation (alias for HashDel)
func (s *CacheServer) HDel(ctx context.Context, req *pb.HashDelRequest) (*pb.HashDelResponse, error) {
	return s.HashDel(ctx, req)
}

// HExists implements Hash Exists operation (alias for HashExistsM)
func (s *CacheServer) HExists(ctx context.Context, req *pb.HashExistsMRequest) (*pb.HashExistsMResponse, error) {
	return s.HashExistsM(ctx, req)
}

// ========== Set Redis Aliases ==========

// SAdd implements Set Add operation (alias for SetAddM)
func (s *CacheServer) SAdd(ctx context.Context, req *pb.SetAddMRequest) (*pb.SetAddMResponse, error) {
	return s.SetAddM(ctx, req)
}

// SIsMember implements Set IsMember operation (alias for SetExistsM)
func (s *CacheServer) SIsMember(ctx context.Context, req *pb.SetExistsMRequest) (*pb.SetExistsMResponse, error) {
	return s.SetExistsM(ctx, req)
}

// SCard implements Set Card (cardinality) operation (alias for SetLen)
func (s *CacheServer) SCard(ctx context.Context, req *pb.SetLenRequest) (*pb.SetLenResponse, error) {
	return s.SetLen(ctx, req)
}

// SMembers implements Set Members operation (alias for SetGet)
func (s *CacheServer) SMembers(ctx context.Context, req *pb.SetGetRequest) (*pb.SetGetResponse, error) {
	return s.SetGet(ctx, req)
}

// SRem implements Set Remove operation (alias for SetDelM)
func (s *CacheServer) SRem(ctx context.Context, req *pb.SetDelMRequest) (*pb.SetDelMResponse, error) {
	return s.SetDelM(ctx, req)
}

// ========== List Operations ==========

// LPush implements List Push operation
func (s *CacheServer) LPush(ctx context.Context, req *pb.LPushRequest) (*pb.LPushResponse, error) {
	length, err := s.cache.LPush(req.Key, req.Value)
	if err != nil {
		return nil, err
	}
	return &pb.LPushResponse{
		Success: true,
		Length:  int32(length),
	}, nil
}

// LRange implements List Range operation
func (s *CacheServer) LRange(ctx context.Context, req *pb.LRangeRequest) (*pb.LRangeResponse, error) {
	values, err := s.cache.LRange(req.Key, int(req.Start), int(req.Stop))
	if err != nil {
		return nil, err
	}
	return &pb.LRangeResponse{
		Values: values,
	}, nil
}

// LRem implements List Remove operation
func (s *CacheServer) LRem(ctx context.Context, req *pb.LRemRequest) (*pb.LRemResponse, error) {
	count, err := s.cache.LRem(req.Key, int(req.Count), req.Value)
	if err != nil {
		return nil, err
	}
	return &pb.LRemResponse{
		Success:      true,
		DeletedCount: int32(count),
	}, nil
}

// LPop implements List LPop operation
func (s *CacheServer) LPop(ctx context.Context, req *pb.LPopRequest) (*pb.LPopResponse, error) {
	value, success, err := s.cache.LPop(req.Key)
	if err != nil {
		return nil, err
	}
	return &pb.LPopResponse{
		Value:   value,
		Success: success,
	}, nil
}

// RPush implements List RPush operation
func (s *CacheServer) RPush(ctx context.Context, req *pb.RPushRequest) (*pb.RPushResponse, error) {
	length, err := s.cache.RPush(req.Key, req.Value)
	if err != nil {
		return nil, err
	}
	return &pb.RPushResponse{
		Success: true,
		Length:  int32(length),
	}, nil
}

// RPop implements List RPop operation
func (s *CacheServer) RPop(ctx context.Context, req *pb.RPopRequest) (*pb.RPopResponse, error) {
	value, success, err := s.cache.RPop(req.Key)
	if err != nil {
		return nil, err
	}
	return &pb.RPopResponse{
		Value:   value,
		Success: success,
	}, nil
}

// LLen implements List Length operation
func (s *CacheServer) LLen(ctx context.Context, req *pb.LLenRequest) (*pb.LLenResponse, error) {
	length, err := s.cache.LLen(req.Key)
	if err != nil {
		return nil, err
	}
	return &pb.LLenResponse{
		Length: int32(length),
	}, nil
}

// LIndex implements List Index operation
func (s *CacheServer) LIndex(ctx context.Context, req *pb.LIndexRequest) (*pb.LIndexResponse, error) {
	value, exists, err := s.cache.LIndex(req.Key, int(req.Index))
	if err != nil {
		return nil, err
	}
	return &pb.LIndexResponse{
		Value:  value,
		Exists: exists,
	}, nil
}

// LSet implements List Set operation
func (s *CacheServer) LSet(ctx context.Context, req *pb.LSetRequest) (*pb.LSetResponse, error) {
	err := s.cache.LSet(req.Key, int(req.Index), req.Value)
	return &pb.LSetResponse{
		Success: err == nil,
	}, err
}
