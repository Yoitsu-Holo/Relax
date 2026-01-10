package proxy

import (
	"context"
	"fmt"
	"log"

	"github.com/yoitsuholo/relax/inf-ClusterNode/cluster"
	pb "github.com/yoitsuholo/relax/proto"
)

// CacheProxy implements the CacheService interface and routes requests to cluster nodes
type CacheProxy struct {
	pb.UnimplementedCacheServiceServer
	clusterMgr *cluster.Manager
}

// NewCacheProxy creates a new cache proxy
func NewCacheProxy(clusterMgr *cluster.Manager) *CacheProxy {
	return &CacheProxy{
		clusterMgr: clusterMgr,
	}
}

// ========== KV Operations ==========

// Set implements the Set RPC
func (p *CacheProxy) Set(ctx context.Context, req *pb.SetRequest) (*pb.SetResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.Set(ctx, req)
	if err != nil {
		log.Printf("Error forwarding Set request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("Set: key=%s, node=%s", req.Key, node.ID)
	return resp, nil
}

// Get implements the Get RPC
func (p *CacheProxy) Get(ctx context.Context, req *pb.GetRequest) (*pb.GetResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.Get(ctx, req)
	if err != nil {
		log.Printf("Error forwarding Get request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("Get: key=%s, node=%s, exists=%v", req.Key, node.ID, resp.Exists)
	return resp, nil
}

// Del implements the Del RPC
func (p *CacheProxy) Del(ctx context.Context, req *pb.DelRequest) (*pb.DelResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.Del(ctx, req)
	if err != nil {
		log.Printf("Error forwarding Del request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("Del: key=%s, node=%s", req.Key, node.ID)
	return resp, nil
}

// Exists implements the Exists RPC
func (p *CacheProxy) Exists(ctx context.Context, req *pb.ExistsRequest) (*pb.ExistsResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.Exists(ctx, req)
	if err != nil {
		log.Printf("Error forwarding Exists request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("Exists: key=%s, node=%s, exists=%v", req.Key, node.ID, resp.Exists)
	return resp, nil
}

// ========== Hash Operations ==========

// HSet implements the HSet RPC
func (p *CacheProxy) HSet(ctx context.Context, req *pb.HSetRequest) (*pb.HSetResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.HSet(ctx, req)
	if err != nil {
		log.Printf("Error forwarding HSet request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("HSet: key=%s, field=%s, node=%s", req.Key, req.Field, node.ID)
	return resp, nil
}

// HGet implements the HGet RPC
func (p *CacheProxy) HGet(ctx context.Context, req *pb.HGetRequest) (*pb.HGetResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.HGet(ctx, req)
	if err != nil {
		log.Printf("Error forwarding HGet request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("HGet: key=%s, field=%s, node=%s, exists=%v", req.Key, req.Field, node.ID, resp.Exists)
	return resp, nil
}

// HDel implements the HDel RPC
func (p *CacheProxy) HDel(ctx context.Context, req *pb.HDelRequest) (*pb.HDelResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.HDel(ctx, req)
	if err != nil {
		log.Printf("Error forwarding HDel request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("HDel: key=%s, field=%s, node=%s", req.Key, req.Field, node.ID)
	return resp, nil
}

// HMSet implements the HMSet RPC
func (p *CacheProxy) HMSet(ctx context.Context, req *pb.HMSetRequest) (*pb.HMSetResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.HMSet(ctx, req)
	if err != nil {
		log.Printf("Error forwarding HMSet request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("HMSet: key=%s, fields=%d, node=%s", req.Key, len(req.Fields), node.ID)
	return resp, nil
}

// HMGet implements the HMGet RPC
func (p *CacheProxy) HMGet(ctx context.Context, req *pb.HMGetRequest) (*pb.HMGetResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.HMGet(ctx, req)
	if err != nil {
		log.Printf("Error forwarding HMGet request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("HMGet: key=%s, fields=%d, node=%s", req.Key, len(req.Fields), node.ID)
	return resp, nil
}

// HLen implements the HLen RPC
func (p *CacheProxy) HLen(ctx context.Context, req *pb.HLenRequest) (*pb.HLenResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.HLen(ctx, req)
	if err != nil {
		log.Printf("Error forwarding HLen request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("HLen: key=%s, node=%s, length=%d", req.Key, node.ID, resp.Length)
	return resp, nil
}

// HGetAll implements the HGetAll RPC
func (p *CacheProxy) HGetAll(ctx context.Context, req *pb.HGetAllRequest) (*pb.HGetAllResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.HGetAll(ctx, req)
	if err != nil {
		log.Printf("Error forwarding HGetAll request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("HGetAll: key=%s, node=%s, fields=%d", req.Key, node.ID, len(resp.Fields))
	return resp, nil
}

// HExists implements the HExists RPC
func (p *CacheProxy) HExists(ctx context.Context, req *pb.HExistsRequest) (*pb.HExistsResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.HExists(ctx, req)
	if err != nil {
		log.Printf("Error forwarding HExists request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("HExists: key=%s, field=%s, node=%s, exists=%v", req.Key, req.Field, node.ID, resp.Exists)
	return resp, nil
}

// ========== Set Operations ==========

// SAdd implements the SAdd RPC
func (p *CacheProxy) SAdd(ctx context.Context, req *pb.SAddRequest) (*pb.SAddResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.SAdd(ctx, req)
	if err != nil {
		log.Printf("Error forwarding SAdd request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("SAdd: key=%s, member=%s, node=%s", req.Key, req.Member, node.ID)
	return resp, nil
}

// SMembers implements the SMembers RPC
func (p *CacheProxy) SMembers(ctx context.Context, req *pb.SMembersRequest) (*pb.SMembersResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.SMembers(ctx, req)
	if err != nil {
		log.Printf("Error forwarding SMembers request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("SMembers: key=%s, node=%s, count=%d", req.Key, node.ID, len(resp.Members))
	return resp, nil
}

// SRem implements the SRem RPC
func (p *CacheProxy) SRem(ctx context.Context, req *pb.SRemRequest) (*pb.SRemResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.SRem(ctx, req)
	if err != nil {
		log.Printf("Error forwarding SRem request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("SRem: key=%s, member=%s, node=%s", req.Key, req.Member, node.ID)
	return resp, nil
}

// SIsMember implements the SIsMember RPC
func (p *CacheProxy) SIsMember(ctx context.Context, req *pb.SIsMemberRequest) (*pb.SIsMemberResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.SIsMember(ctx, req)
	if err != nil {
		log.Printf("Error forwarding SIsMember request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("SIsMember: key=%s, member=%s, node=%s, is_member=%v", req.Key, req.Member, node.ID, resp.IsMember)
	return resp, nil
}

// SCard implements the SCard RPC
func (p *CacheProxy) SCard(ctx context.Context, req *pb.SCardRequest) (*pb.SCardResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.SCard(ctx, req)
	if err != nil {
		log.Printf("Error forwarding SCard request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("SCard: key=%s, node=%s, cardinality=%d", req.Key, node.ID, resp.Cardinality)
	return resp, nil
}

// ========== List Operations ==========

// LPush implements the LPush RPC
func (p *CacheProxy) LPush(ctx context.Context, req *pb.LPushRequest) (*pb.LPushResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.LPush(ctx, req)
	if err != nil {
		log.Printf("Error forwarding LPush request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("LPush: key=%s, node=%s, length=%d", req.Key, node.ID, resp.Length)
	return resp, nil
}

// LRange implements the LRange RPC
func (p *CacheProxy) LRange(ctx context.Context, req *pb.LRangeRequest) (*pb.LRangeResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.LRange(ctx, req)
	if err != nil {
		log.Printf("Error forwarding LRange request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("LRange: key=%s, node=%s, count=%d", req.Key, node.ID, len(resp.Values))
	return resp, nil
}

// LRem implements the LRem RPC
func (p *CacheProxy) LRem(ctx context.Context, req *pb.LRemRequest) (*pb.LRemResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.LRem(ctx, req)
	if err != nil {
		log.Printf("Error forwarding LRem request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("LRem: key=%s, node=%s", req.Key, node.ID)
	return resp, nil
}

// LPop implements the LPop RPC
func (p *CacheProxy) LPop(ctx context.Context, req *pb.LPopRequest) (*pb.LPopResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.LPop(ctx, req)
	if err != nil {
		log.Printf("Error forwarding LPop request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("LPop: key=%s, node=%s, success=%v", req.Key, node.ID, resp.Success)
	return resp, nil
}

// RPush implements the RPush RPC
func (p *CacheProxy) RPush(ctx context.Context, req *pb.RPushRequest) (*pb.RPushResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.RPush(ctx, req)
	if err != nil {
		log.Printf("Error forwarding RPush request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("RPush: key=%s, node=%s, length=%d", req.Key, node.ID, resp.Length)
	return resp, nil
}

// RPop implements the RPop RPC
func (p *CacheProxy) RPop(ctx context.Context, req *pb.RPopRequest) (*pb.RPopResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.RPop(ctx, req)
	if err != nil {
		log.Printf("Error forwarding RPop request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("RPop: key=%s, node=%s, success=%v", req.Key, node.ID, resp.Success)
	return resp, nil
}

// LLen implements the LLen RPC
func (p *CacheProxy) LLen(ctx context.Context, req *pb.LLenRequest) (*pb.LLenResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.LLen(ctx, req)
	if err != nil {
		log.Printf("Error forwarding LLen request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("LLen: key=%s, node=%s, length=%d", req.Key, node.ID, resp.Length)
	return resp, nil
}

// LIndex implements the LIndex RPC
func (p *CacheProxy) LIndex(ctx context.Context, req *pb.LIndexRequest) (*pb.LIndexResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.LIndex(ctx, req)
	if err != nil {
		log.Printf("Error forwarding LIndex request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("LIndex: key=%s, index=%d, node=%s, exists=%v", req.Key, req.Index, node.ID, resp.Exists)
	return resp, nil
}

// LSet implements the LSet RPC
func (p *CacheProxy) LSet(ctx context.Context, req *pb.LSetRequest) (*pb.LSetResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.LSet(ctx, req)
	if err != nil {
		log.Printf("Error forwarding LSet request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("LSet: key=%s, index=%d, node=%s", req.Key, req.Index, node.ID)
	return resp, nil
}
