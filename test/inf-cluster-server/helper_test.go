package cluster_test

import (
	"context"
	"fmt"
	"log"
	"net"
	"testing"
	"time"

	"github.com/yoitsuholo/relax/inf-ClusterNode/cluster"
	"github.com/yoitsuholo/relax/inf-ClusterNode/proxy"
	"github.com/yoitsuholo/relax/inf-SingleNode/server/cache/simpleCache"
	"github.com/yoitsuholo/relax/inf-SingleNode/server/service"
	pb "github.com/yoitsuholo/relax/proto"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
)

// TestCluster represents a test cluster with multiple nodes and a proxy
type TestCluster struct {
	// Single node servers
	nodeServers []*grpc.Server
	nodeAddrs   []string
	caches      []*simpleCache.ShardedCache

	// Cluster proxy
	proxyServer *grpc.Server
	proxyAddr   string
	manager     *cluster.Manager

	// Client connection to proxy
	clientConn *grpc.ClientConn
	client     pb.CacheServiceClient
}

// SetupTestCluster creates a test cluster with the specified number of nodes
func SetupTestCluster(t *testing.T, numNodes int) *TestCluster {
	if numNodes < 1 {
		t.Fatal("numNodes must be at least 1")
	}

	tc := &TestCluster{
		nodeServers: make([]*grpc.Server, numNodes),
		nodeAddrs:   make([]string, numNodes),
		caches:      make([]*simpleCache.ShardedCache, numNodes),
	}

	// Start single node servers
	for i := 0; i < numNodes; i++ {
		// Create cache instance
		cacheInstance := simpleCache.NewShardedCache(32)
		tc.caches[i] = cacheInstance

		// Create gRPC server
		grpcServer := grpc.NewServer()
		cacheService := service.NewCacheServerWithCache(cacheInstance)
		pb.RegisterCacheServiceServer(grpcServer, cacheService)

		// Find available port and start server
		listener, err := net.Listen("tcp", "localhost:0")
		if err != nil {
			t.Fatalf("Failed to create listener for node %d: %v", i, err)
		}

		tc.nodeAddrs[i] = listener.Addr().String()
		tc.nodeServers[i] = grpcServer

		// Start server in background
		go func(srv *grpc.Server, lis net.Listener, idx int) {
			if err := srv.Serve(lis); err != nil {
				log.Printf("Node %d server error: %v", idx, err)
			}
		}(grpcServer, listener, i)
	}

	// Wait for nodes to be ready
	time.Sleep(100 * time.Millisecond)

	// Create cluster manager configuration
	nodeConfigs := make([]cluster.NodeConfig, numNodes)
	for i := 0; i < numNodes; i++ {
		nodeConfigs[i] = cluster.NodeConfig{
			ID:      fmt.Sprintf("node-%d", i),
			Address: tc.nodeAddrs[i],
		}
	}

	// Create cluster manager
	mgr, err := cluster.NewManager(nodeConfigs, 5*time.Second)
	if err != nil {
		t.Fatalf("Failed to create cluster manager: %v", err)
	}
	tc.manager = mgr

	// Wait for health checks
	time.Sleep(200 * time.Millisecond)

	// Verify all nodes are healthy
	healthyCount := mgr.GetHealthyNodeCount()
	if healthyCount != numNodes {
		t.Fatalf("Expected %d healthy nodes, got %d", numNodes, healthyCount)
	}

	// Create cluster proxy server
	proxyService := proxy.NewCacheProxy(mgr)
	tc.proxyServer = grpc.NewServer()
	pb.RegisterCacheServiceServer(tc.proxyServer, proxyService)

	// Start proxy server
	proxyListener, err := net.Listen("tcp", "localhost:0")
	if err != nil {
		t.Fatalf("Failed to create listener for proxy: %v", err)
	}
	tc.proxyAddr = proxyListener.Addr().String()

	go func() {
		if err := tc.proxyServer.Serve(proxyListener); err != nil {
			log.Printf("Proxy server error: %v", err)
		}
	}()

	// Wait for proxy to be ready
	time.Sleep(100 * time.Millisecond)

	// Create client connection to proxy
	conn, err := grpc.NewClient(
		tc.proxyAddr,
		grpc.WithTransportCredentials(insecure.NewCredentials()),
	)
	if err != nil {
		t.Fatalf("Failed to connect to proxy: %v", err)
	}
	tc.clientConn = conn
	tc.client = pb.NewCacheServiceClient(conn)

	t.Logf("Test cluster started with %d nodes", numNodes)
	for i := 0; i < numNodes; i++ {
		t.Logf("  Node %d: %s", i, tc.nodeAddrs[i])
	}
	t.Logf("  Proxy: %s", tc.proxyAddr)

	return tc
}

// Teardown stops and cleans up the test cluster
func (tc *TestCluster) Teardown() {
	// Close client connection
	if tc.clientConn != nil {
		tc.clientConn.Close()
	}

	// Stop proxy server
	if tc.proxyServer != nil {
		tc.proxyServer.Stop()
	}

	// Close cluster manager
	if tc.manager != nil {
		tc.manager.Close()
	}

	// Stop all node servers
	for _, server := range tc.nodeServers {
		if server != nil {
			server.Stop()
		}
	}

	// Close all caches
	for _, cache := range tc.caches {
		if cache != nil {
			cache.Close()
		}
	}
}

// GetNodeForKey returns which node index a key would be routed to
func (tc *TestCluster) GetNodeForKey(key string) int {
	return tc.manager.GetNodeIndexForKey(key)
}

// GetClient returns the gRPC client for the cluster proxy
func (tc *TestCluster) GetClient() pb.CacheServiceClient {
	return tc.client
}

// TestContext creates a context with timeout for tests
func TestContext() context.Context {
	ctx, _ := context.WithTimeout(context.Background(), 5*time.Second)
	return ctx
}
