package cluster

import (
	"context"
	"fmt"
	"sync"
	"time"

	pb "github.com/yoitsuholo/relax/proto"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
)

// Node represents a single cache node in the cluster
type Node struct {
	ID       string
	Address  string
	conn     *grpc.ClientConn
	client   pb.CacheServiceClient
	mu       sync.RWMutex
	healthy  bool
	lastPing time.Time
}

// NewNode creates a new node connection
func NewNode(id, address string) (*Node, error) {
	node := &Node{
		ID:      id,
		Address: address,
		healthy: false,
	}

	if err := node.Connect(); err != nil {
		return nil, fmt.Errorf("failed to connect to node %s: %w", id, err)
	}

	return node, nil
}

// Connect establishes a connection to the node
func (n *Node) Connect() error {
	n.mu.Lock()
	defer n.mu.Unlock()

	// Close existing connection if any
	if n.conn != nil {
		n.conn.Close()
	}

	// Create new connection
	conn, err := grpc.NewClient(
		n.Address,
		grpc.WithTransportCredentials(insecure.NewCredentials()),
		grpc.WithDefaultCallOptions(
			grpc.MaxCallRecvMsgSize(10*1024*1024), // 10MB
			grpc.MaxCallSendMsgSize(10*1024*1024), // 10MB
		),
	)
	if err != nil {
		n.healthy = false
		return fmt.Errorf("failed to create gRPC client: %w", err)
	}

	n.conn = conn
	n.client = pb.NewCacheServiceClient(conn)
	n.healthy = true
	n.lastPing = time.Now()

	return nil
}

// Close closes the connection to the node
func (n *Node) Close() error {
	n.mu.Lock()
	defer n.mu.Unlock()

	if n.conn != nil {
		return n.conn.Close()
	}
	return nil
}

// IsHealthy returns whether the node is healthy
func (n *Node) IsHealthy() bool {
	n.mu.RLock()
	defer n.mu.RUnlock()
	return n.healthy
}

// SetHealthy sets the health status of the node
func (n *Node) SetHealthy(healthy bool) {
	n.mu.Lock()
	defer n.mu.Unlock()
	n.healthy = healthy
	if healthy {
		n.lastPing = time.Now()
	}
}

// GetClient returns the gRPC client for this node
func (n *Node) GetClient() pb.CacheServiceClient {
	n.mu.RLock()
	defer n.mu.RUnlock()
	return n.client
}

// HealthCheck performs a health check on the node
func (n *Node) HealthCheck(ctx context.Context) error {
	client := n.GetClient()
	if client == nil {
		return fmt.Errorf("client not initialized")
	}

	// Use a simple Get operation as health check
	ctx, cancel := context.WithTimeout(ctx, 2*time.Second)
	defer cancel()

	_, err := client.Get(ctx, &pb.KvGetRequest{Key: "__health_check__"})
	if err != nil {
		n.SetHealthy(false)
		return err
	}

	n.SetHealthy(true)
	return nil
}
