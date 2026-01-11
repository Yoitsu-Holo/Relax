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

// ========== KV Operations ==========

// Set implements KV Set operation
func (s *CacheServer) Set(ctx context.Context, req *pb.SetRequest) (*pb.SetResponse, error) {
	err := s.cache.Set(req.Key, req.Value)
	return &pb.SetResponse{Success: err == nil}, err
}

// Get implements KV Get operation
func (s *CacheServer) Get(ctx context.Context, req *pb.GetRequest) (*pb.GetResponse, error) {
	value, exists, err := s.cache.Get(req.Key)
	if err != nil {
		return nil, err
	}
	return &pb.GetResponse{
		Value:  value,
		Exists: exists,
	}, nil
}

// Del implements KV Del operation
func (s *CacheServer) Del(ctx context.Context, req *pb.DelRequest) (*pb.DelResponse, error) {
	count, err := s.cache.Del(req.Key)
	if err != nil {
		return nil, err
	}
	return &pb.DelResponse{
		Success:      true,
		DeletedCount: int32(count),
	}, nil
}

// Exists implements KV Exists operation
func (s *CacheServer) Exists(ctx context.Context, req *pb.ExistsRequest) (*pb.ExistsResponse, error) {
	exists, err := s.cache.Exists(req.Key)
	if err != nil {
		return nil, err
	}
	return &pb.ExistsResponse{
		Exists: exists,
	}, nil
}

// ========== Hash Operations ==========

// HSet implements Hash Set operation
func (s *CacheServer) HSet(ctx context.Context, req *pb.HSetRequest) (*pb.HSetResponse, error) {
	created, err := s.cache.HSet(req.Key, req.Field, req.Value)
	if err != nil {
		return nil, err
	}
	return &pb.HSetResponse{
		Success: true,
		Created: created,
	}, nil
}

// HGet implements Hash Get operation
func (s *CacheServer) HGet(ctx context.Context, req *pb.HGetRequest) (*pb.HGetResponse, error) {
	value, exists, err := s.cache.HGet(req.Key, req.Field)
	if err != nil {
		return nil, err
	}
	return &pb.HGetResponse{
		Value:  value,
		Exists: exists,
	}, nil
}

// HDel implements Hash Del operation
func (s *CacheServer) HDel(ctx context.Context, req *pb.HDelRequest) (*pb.HDelResponse, error) {
	count, err := s.cache.HDel(req.Key, req.Field)
	if err != nil {
		return nil, err
	}
	return &pb.HDelResponse{
		Success:      true,
		DeletedCount: int32(count),
	}, nil
}

// HMSet implements Hash Multi-Set operation
func (s *CacheServer) HMSet(ctx context.Context, req *pb.HMSetRequest) (*pb.HMSetResponse, error) {
	count, err := s.cache.HMSet(req.Key, req.Fields)
	if err != nil {
		return nil, err
	}
	return &pb.HMSetResponse{
		Success: true,
		Count:   int32(count),
	}, nil
}

// HMGet implements Hash Multi-Get operation
func (s *CacheServer) HMGet(ctx context.Context, req *pb.HMGetRequest) (*pb.HMGetResponse, error) {
	values, err := s.cache.HMGet(req.Key, req.Fields)
	if err != nil {
		return nil, err
	}
	return &pb.HMGetResponse{
		Values: values,
	}, nil
}

// HLen implements Hash Length operation
func (s *CacheServer) HLen(ctx context.Context, req *pb.HLenRequest) (*pb.HLenResponse, error) {
	length, err := s.cache.HLen(req.Key)
	if err != nil {
		return nil, err
	}
	return &pb.HLenResponse{
		Length: int32(length),
	}, nil
}

// HGetAll implements Hash GetAll operation
func (s *CacheServer) HGetAll(ctx context.Context, req *pb.HGetAllRequest) (*pb.HGetAllResponse, error) {
	fields, err := s.cache.HGetAll(req.Key)
	if err != nil {
		return nil, err
	}
	return &pb.HGetAllResponse{
		Fields: fields,
	}, nil
}

// HExists implements Hash Exists operation
func (s *CacheServer) HExists(ctx context.Context, req *pb.HExistsRequest) (*pb.HExistsResponse, error) {
	exists, err := s.cache.HExists(req.Key, req.Field)
	if err != nil {
		return nil, err
	}
	return &pb.HExistsResponse{
		Exists: exists,
	}, nil
}

// ========== Set Operations ==========

// SAdd implements Set Add operation
func (s *CacheServer) SAdd(ctx context.Context, req *pb.SAddRequest) (*pb.SAddResponse, error) {
	added, err := s.cache.SAdd(req.Key, req.Member)
	if err != nil {
		return nil, err
	}
	return &pb.SAddResponse{
		Success: true,
		Added:   added,
	}, nil
}

// SMembers implements Set Members operation
func (s *CacheServer) SMembers(ctx context.Context, req *pb.SMembersRequest) (*pb.SMembersResponse, error) {
	members, err := s.cache.SMembers(req.Key)
	if err != nil {
		return nil, err
	}
	return &pb.SMembersResponse{
		Members: members,
	}, nil
}

// SRem implements Set Remove operation
func (s *CacheServer) SRem(ctx context.Context, req *pb.SRemRequest) (*pb.SRemResponse, error) {
	count, err := s.cache.SRem(req.Key, req.Member)
	if err != nil {
		return nil, err
	}
	return &pb.SRemResponse{
		Success:      true,
		DeletedCount: int32(count),
	}, nil
}

// SIsMember implements Set IsMember operation
func (s *CacheServer) SIsMember(ctx context.Context, req *pb.SIsMemberRequest) (*pb.SIsMemberResponse, error) {
	isMember, err := s.cache.SIsMember(req.Key, req.Member)
	if err != nil {
		return nil, err
	}
	return &pb.SIsMemberResponse{
		IsMember: isMember,
	}, nil
}

// SCard implements Set Card (cardinality) operation
func (s *CacheServer) SCard(ctx context.Context, req *pb.SCardRequest) (*pb.SCardResponse, error) {
	card, err := s.cache.SCard(req.Key)
	if err != nil {
		return nil, err
	}
	return &pb.SCardResponse{
		Cardinality: int32(card),
	}, nil
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
