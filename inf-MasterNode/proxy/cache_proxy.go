package proxy

import (
	"context"
	"fmt"
	"log"

	"github.com/yoitsuholo/relax/inf-MasterNode/cluster"
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

// ========== KV Native Operations ==========

// KvSet implements the KvSet RPC
func (p *CacheProxy) KvSet(ctx context.Context, req *pb.KvSetRequest) (*pb.KvSetResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.KvSet(ctx, req)
	if err != nil {
		log.Printf("Error forwarding KvSet request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("KvSet: key=%s, node=%s", req.Key, node.ID)
	return resp, nil
}

// KvGet implements the KvGet RPC
func (p *CacheProxy) KvGet(ctx context.Context, req *pb.KvGetRequest) (*pb.KvGetResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.KvGet(ctx, req)
	if err != nil {
		log.Printf("Error forwarding KvGet request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("KvGet: key=%s, node=%s, exists=%v", req.Key, node.ID, resp.Exists)
	return resp, nil
}

// KvDel implements the KvDel RPC
func (p *CacheProxy) KvDel(ctx context.Context, req *pb.KvDelRequest) (*pb.KvDelResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.KvDel(ctx, req)
	if err != nil {
		log.Printf("Error forwarding KvDel request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("KvDel: key=%s, node=%s", req.Key, node.ID)
	return resp, nil
}

// KvExists implements the KvExists RPC
func (p *CacheProxy) KvExists(ctx context.Context, req *pb.KvExistsRequest) (*pb.KvExistsResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.KvExists(ctx, req)
	if err != nil {
		log.Printf("Error forwarding KvExists request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("KvExists: key=%s, node=%s, exists=%v", req.Key, node.ID, resp.Exists)
	return resp, nil
}

// ========== Hash Native Operations - Batch/Whole Hash ==========

// HashSet implements the HashSet RPC (batch set multiple fields)
func (p *CacheProxy) HashSet(ctx context.Context, req *pb.HashSetRequest) (*pb.HashSetResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.HashSet(ctx, req)
	if err != nil {
		log.Printf("Error forwarding HashSet request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("HashSet: key=%s, fields=%d, node=%s", req.Key, len(req.Fields), node.ID)
	return resp, nil
}

// HashGet implements the HashGet RPC (get all fields)
func (p *CacheProxy) HashGet(ctx context.Context, req *pb.HashGetRequest) (*pb.HashGetResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.HashGet(ctx, req)
	if err != nil {
		log.Printf("Error forwarding HashGet request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("HashGet: key=%s, node=%s, fields=%d", req.Key, node.ID, len(resp.Fields))
	return resp, nil
}

// HashDel implements the HashDel RPC (delete entire hash)
func (p *CacheProxy) HashDel(ctx context.Context, req *pb.HashDelRequest) (*pb.HashDelResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.HashDel(ctx, req)
	if err != nil {
		log.Printf("Error forwarding HashDel request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("HashDel: key=%s, node=%s", req.Key, node.ID)
	return resp, nil
}

// HashExists implements the HashExists RPC (check if hash key exists)
func (p *CacheProxy) HashExists(ctx context.Context, req *pb.HashExistsRequest) (*pb.HashExistsResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.HashExists(ctx, req)
	if err != nil {
		log.Printf("Error forwarding HashExists request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("HashExists: key=%s, node=%s, exists=%v", req.Key, node.ID, resp.Exists)
	return resp, nil
}

// HashLen implements the HashLen RPC (get number of fields)
func (p *CacheProxy) HashLen(ctx context.Context, req *pb.HashLenRequest) (*pb.HashLenResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.HashLen(ctx, req)
	if err != nil {
		log.Printf("Error forwarding HashLen request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("HashLen: key=%s, node=%s, length=%d", req.Key, node.ID, resp.Length)
	return resp, nil
}

// ========== Hash Native Operations - Single Field ==========

// HashSetM implements the HashSetM RPC (set single field)
func (p *CacheProxy) HashSetM(ctx context.Context, req *pb.HashSetMRequest) (*pb.HashSetMResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.HashSetM(ctx, req)
	if err != nil {
		log.Printf("Error forwarding HashSetM request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("HashSetM: key=%s, field=%s, node=%s", req.Key, req.Field, node.ID)
	return resp, nil
}

// HashGetM implements the HashGetM RPC (get single field)
func (p *CacheProxy) HashGetM(ctx context.Context, req *pb.HashGetMRequest) (*pb.HashGetMResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.HashGetM(ctx, req)
	if err != nil {
		log.Printf("Error forwarding HashGetM request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("HashGetM: key=%s, field=%s, node=%s, exists=%v", req.Key, req.Field, node.ID, resp.Exists)
	return resp, nil
}

// HashDelM implements the HashDelM RPC (delete single field)
func (p *CacheProxy) HashDelM(ctx context.Context, req *pb.HashDelMRequest) (*pb.HashDelMResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.HashDelM(ctx, req)
	if err != nil {
		log.Printf("Error forwarding HashDelM request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("HashDelM: key=%s, field=%s, node=%s", req.Key, req.Field, node.ID)
	return resp, nil
}

// HashExistsM implements the HashExistsM RPC (check if field exists)
func (p *CacheProxy) HashExistsM(ctx context.Context, req *pb.HashExistsMRequest) (*pb.HashExistsMResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.HashExistsM(ctx, req)
	if err != nil {
		log.Printf("Error forwarding HashExistsM request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("HashExistsM: key=%s, field=%s, node=%s, exists=%v", req.Key, req.Field, node.ID, resp.Exists)
	return resp, nil
}

// ========== Set Native Operations - Batch/Whole Set ==========

// SetSet implements the SetSet RPC (batch add multiple members)
func (p *CacheProxy) SetSet(ctx context.Context, req *pb.SetSetRequest) (*pb.SetSetResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.SetSet(ctx, req)
	if err != nil {
		log.Printf("Error forwarding SetSet request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("SetSet: key=%s, members=%d, node=%s", req.Key, len(req.Members), node.ID)
	return resp, nil
}

// SetGet implements the SetGet RPC (get all members)
func (p *CacheProxy) SetGet(ctx context.Context, req *pb.SetGetRequest) (*pb.SetGetResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.SetGet(ctx, req)
	if err != nil {
		log.Printf("Error forwarding SetGet request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("SetGet: key=%s, node=%s, count=%d", req.Key, node.ID, len(resp.Members))
	return resp, nil
}

// SetDel implements the SetDel RPC (delete entire set)
func (p *CacheProxy) SetDel(ctx context.Context, req *pb.SetDelRequest) (*pb.SetDelResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.SetDel(ctx, req)
	if err != nil {
		log.Printf("Error forwarding SetDel request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("SetDel: key=%s, node=%s", req.Key, node.ID)
	return resp, nil
}

// SetExists implements the SetExists RPC (check if set key exists)
func (p *CacheProxy) SetExists(ctx context.Context, req *pb.SetExistsRequest) (*pb.SetExistsResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.SetExists(ctx, req)
	if err != nil {
		log.Printf("Error forwarding SetExists request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("SetExists: key=%s, node=%s, exists=%v", req.Key, node.ID, resp.Exists)
	return resp, nil
}

// SetLen implements the SetLen RPC (get number of members)
func (p *CacheProxy) SetLen(ctx context.Context, req *pb.SetLenRequest) (*pb.SetLenResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.SetLen(ctx, req)
	if err != nil {
		log.Printf("Error forwarding SetLen request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("SetLen: key=%s, node=%s, length=%d", req.Key, node.ID, resp.Length)
	return resp, nil
}

// ========== Set Native Operations - Single Member ==========

// SetAddM implements the SetAddM RPC (add single member)
func (p *CacheProxy) SetAddM(ctx context.Context, req *pb.SetAddMRequest) (*pb.SetAddMResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.SetAddM(ctx, req)
	if err != nil {
		log.Printf("Error forwarding SetAddM request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("SetAddM: key=%s, member=%s, node=%s", req.Key, req.Member, node.ID)
	return resp, nil
}

// SetExistsM implements the SetExistsM RPC (check if member exists)
func (p *CacheProxy) SetExistsM(ctx context.Context, req *pb.SetExistsMRequest) (*pb.SetExistsMResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.SetExistsM(ctx, req)
	if err != nil {
		log.Printf("Error forwarding SetExistsM request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("SetExistsM: key=%s, member=%s, node=%s, is_member=%v", req.Key, req.Member, node.ID, resp.IsMember)
	return resp, nil
}

// SetDelM implements the SetDelM RPC (delete single member)
func (p *CacheProxy) SetDelM(ctx context.Context, req *pb.SetDelMRequest) (*pb.SetDelMResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.SetDelM(ctx, req)
	if err != nil {
		log.Printf("Error forwarding SetDelM request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("SetDelM: key=%s, member=%s, node=%s", req.Key, req.Member, node.ID)
	return resp, nil
}

// SetGetM implements the SetGetM RPC (get single member data for verification)
func (p *CacheProxy) SetGetM(ctx context.Context, req *pb.SetGetMRequest) (*pb.SetGetMResponse, error) {
	node, err := p.clusterMgr.GetNodeForKey(req.Key)
	if err != nil {
		return nil, fmt.Errorf("failed to get node for key %s: %w", req.Key, err)
	}

	if !node.IsHealthy() {
		return nil, fmt.Errorf("node %s is unhealthy", node.ID)
	}

	client := node.GetClient()
	resp, err := client.SetGetM(ctx, req)
	if err != nil {
		log.Printf("Error forwarding SetGetM request to node %s: %v", node.ID, err)
		return nil, err
	}

	log.Printf("SetGetM: key=%s, member=%s, node=%s, exists=%v", req.Key, req.Member, node.ID, resp.Exists)
	return resp, nil
}

// ========== KV Redis Aliases ==========

// Set implements the Set RPC (forwards to KvSet)
func (p *CacheProxy) Set(ctx context.Context, req *pb.KvSetRequest) (*pb.KvSetResponse, error) {
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

// Get implements the Get RPC (forwards to KvGet)
func (p *CacheProxy) Get(ctx context.Context, req *pb.KvGetRequest) (*pb.KvGetResponse, error) {
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

// Del implements the Del RPC (forwards to KvDel)
func (p *CacheProxy) Del(ctx context.Context, req *pb.KvDelRequest) (*pb.KvDelResponse, error) {
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

// Exists implements the Exists RPC (forwards to KvExists)
func (p *CacheProxy) Exists(ctx context.Context, req *pb.KvExistsRequest) (*pb.KvExistsResponse, error) {
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

// ========== Hash Redis Aliases ==========

// HMSet implements the HMSet RPC (forwards to HashSetM)
func (p *CacheProxy) HMSet(ctx context.Context, req *pb.HashSetMRequest) (*pb.HashSetMResponse, error) {
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

	log.Printf("HMSet: key=%s, field=%s, node=%s", req.Key, req.Field, node.ID)
	return resp, nil
}

// HMGet implements the HMGet RPC (forwards to HashGetM)
func (p *CacheProxy) HMGet(ctx context.Context, req *pb.HashGetMRequest) (*pb.HashGetMResponse, error) {
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

	log.Printf("HMGet: key=%s, field=%s, node=%s", req.Key, req.Field, node.ID)
	return resp, nil
}

// HLen implements the HLen RPC (forwards to HashLen)
func (p *CacheProxy) HLen(ctx context.Context, req *pb.HashLenRequest) (*pb.HashLenResponse, error) {
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

// HGetAll implements the HGetAll RPC (forwards to HashGet)
func (p *CacheProxy) HGetAll(ctx context.Context, req *pb.HashGetRequest) (*pb.HashGetResponse, error) {
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

// HDel implements the HDel RPC (forwards to HashDel)
func (p *CacheProxy) HDel(ctx context.Context, req *pb.HashDelRequest) (*pb.HashDelResponse, error) {
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

	log.Printf("HDel: key=%s, node=%s", req.Key, node.ID)
	return resp, nil
}

// HExists implements the HExists RPC (forwards to HashExistsM)
func (p *CacheProxy) HExists(ctx context.Context, req *pb.HashExistsMRequest) (*pb.HashExistsMResponse, error) {
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

// ========== Set Redis Aliases ==========

// SAdd implements the SAdd RPC (forwards to SetAddM)
func (p *CacheProxy) SAdd(ctx context.Context, req *pb.SetAddMRequest) (*pb.SetAddMResponse, error) {
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

// SIsMember implements the SIsMember RPC (forwards to SetExistsM)
func (p *CacheProxy) SIsMember(ctx context.Context, req *pb.SetExistsMRequest) (*pb.SetExistsMResponse, error) {
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

// SCard implements the SCard RPC (forwards to SetLen)
func (p *CacheProxy) SCard(ctx context.Context, req *pb.SetLenRequest) (*pb.SetLenResponse, error) {
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

	log.Printf("SCard: key=%s, node=%s, cardinality=%d", req.Key, node.ID, resp.Length)
	return resp, nil
}

// SMembers implements the SMembers RPC (forwards to SetGet)
func (p *CacheProxy) SMembers(ctx context.Context, req *pb.SetGetRequest) (*pb.SetGetResponse, error) {
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

// SRem implements the SRem RPC (forwards to SetDelM)
func (p *CacheProxy) SRem(ctx context.Context, req *pb.SetDelMRequest) (*pb.SetDelMResponse, error) {
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
