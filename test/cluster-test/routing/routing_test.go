package routing_test

import (
	"testing"
	"time"

	"github.com/yoitsuholo/relax/inf-ClusterNode/cluster"
)

// TestManagerCreation tests basic manager creation
func TestManagerCreation(t *testing.T) {
	nodes := []cluster.NodeConfig{
		{ID: "node-1", Address: "localhost:50051"},
		{ID: "node-2", Address: "localhost:50052"},
		{ID: "node-3", Address: "localhost:50053"},
	}

	mgr, err := cluster.NewManager(nodes, 5*time.Second)
	if err != nil {
		t.Fatalf("Failed to create manager: %v", err)
	}
	defer mgr.Close()

	// Verify node count
	allNodes := mgr.GetAllNodes()
	if len(allNodes) != len(nodes) {
		t.Errorf("Expected %d nodes, got %d", len(nodes), len(allNodes))
	}

	// Verify each node
	for i, node := range allNodes {
		if node.ID == "" {
			t.Errorf("Node %d has empty ID", i)
		}
		if node.Address == "" {
			t.Errorf("Node %d has empty address", i)
		}
		t.Logf("Node %d: %s @ %s (Healthy: %v)", i, node.ID, node.Address, node.IsHealthy())
	}
}

// TestGetNodeForKey tests key-to-node routing
func TestGetNodeForKey(t *testing.T) {
	nodes := []cluster.NodeConfig{
		{ID: "node-1", Address: "localhost:50051"},
		{ID: "node-2", Address: "localhost:50052"},
		{ID: "node-3", Address: "localhost:50053"},
	}

	mgr, err := cluster.NewManager(nodes, 5*time.Second)
	if err != nil {
		t.Fatalf("Failed to create manager: %v", err)
	}
	defer mgr.Close()

	tests := []struct {
		key string
	}{
		{"user:1001"},
		{"user:1002"},
		{"session:abc"},
		{"cache:data"},
		{"key-123"},
	}

	for _, tt := range tests {
		t.Run(tt.key, func(t *testing.T) {
			node, err := mgr.GetNodeForKey(tt.key)
			if err != nil && node == nil {
				t.Fatalf("GetNodeForKey failed: %v", err)
			}

			if node == nil {
				t.Fatal("Node should not be nil")
			}

			if node.ID == "" {
				t.Error("Node ID should not be empty")
			}

			t.Logf("Key '%s' -> Node '%s' @ %s", tt.key, node.ID, node.Address)
		})
	}
}

// TestConsistentRouting tests that the same key always routes to the same node
func TestConsistentRouting(t *testing.T) {
	nodes := []cluster.NodeConfig{
		{ID: "node-1", Address: "localhost:50051"},
		{ID: "node-2", Address: "localhost:50052"},
		{ID: "node-3", Address: "localhost:50053"},
	}

	mgr, err := cluster.NewManager(nodes, 5*time.Second)
	if err != nil {
		t.Fatalf("Failed to create manager: %v", err)
	}
	defer mgr.Close()

	tests := []struct {
		key string
	}{
		{"user:12345"},
		{"session:xyz"},
		{"cache:abc"},
	}

	for _, tt := range tests {
		t.Run(tt.key, func(t *testing.T) {
			var firstNodeID string

			// Route the same key 100 times
			for i := 0; i < 100; i++ {
				node, err := mgr.GetNodeForKey(tt.key)
				if err != nil && node == nil {
					t.Fatalf("GetNodeForKey failed at iteration %d: %v", i, err)
				}

				if i == 0 {
					firstNodeID = node.ID
				} else if node.ID != firstNodeID {
					t.Errorf("Inconsistent routing at iteration %d: expected '%s', got '%s'",
						i, firstNodeID, node.ID)
				}
			}

			t.Logf("Key '%s' consistently routed to '%s' (100 iterations)", tt.key, firstNodeID)
		})
	}
}

// TestGetNodeIndexForKey tests node index calculation
func TestGetNodeIndexForKey(t *testing.T) {
	nodes := []cluster.NodeConfig{
		{ID: "node-1", Address: "localhost:50051"},
		{ID: "node-2", Address: "localhost:50052"},
		{ID: "node-3", Address: "localhost:50053"},
	}

	mgr, err := cluster.NewManager(nodes, 5*time.Second)
	if err != nil {
		t.Fatalf("Failed to create manager: %v", err)
	}
	defer mgr.Close()

	tests := []string{
		"key1", "key2", "key3", "key4", "key5",
		"key6", "key7", "key8", "key9", "key10",
	}

	for _, key := range tests {
		nodeIndex := mgr.GetNodeIndexForKey(key)

		// Verify index is valid
		if nodeIndex < 0 || nodeIndex >= len(nodes) {
			t.Errorf("Invalid node index for key '%s': %d (expected 0-%d)",
				key, nodeIndex, len(nodes)-1)
		}

		// Verify consistency
		nodeIndex2 := mgr.GetNodeIndexForKey(key)
		if nodeIndex != nodeIndex2 {
			t.Errorf("Inconsistent node index for key '%s': %d != %d",
				key, nodeIndex, nodeIndex2)
		}

		t.Logf("Key '%s' -> Node index %d", key, nodeIndex)
	}
}

// TestDifferentNodeCounts tests routing with different numbers of nodes
func TestDifferentNodeCounts(t *testing.T) {
	nodeCounts := []int{1, 2, 3, 5, 7, 10}

	for _, count := range nodeCounts {
		t.Run(testName(count), func(t *testing.T) {
			nodes := make([]cluster.NodeConfig, count)
			for i := 0; i < count; i++ {
				nodes[i] = cluster.NodeConfig{
					ID:      testID(i),
					Address: testAddr(50051 + i),
				}
			}

			mgr, err := cluster.NewManager(nodes, 5*time.Second)
			if err != nil {
				t.Fatalf("Failed to create manager with %d nodes: %v", count, err)
			}
			defer mgr.Close()

			// Test routing
			key := "test:key"
			node, err := mgr.GetNodeForKey(key)
			if err != nil && node == nil {
				t.Fatalf("GetNodeForKey failed: %v", err)
			}

			nodeIndex := mgr.GetNodeIndexForKey(key)
			if nodeIndex < 0 || nodeIndex >= count {
				t.Errorf("Invalid node index: %d (expected 0-%d)", nodeIndex, count-1)
			}

			t.Logf("%d nodes: Key '%s' -> Node %d (%s)", count, key, nodeIndex, node.ID)
		})
	}
}

// TestGetNodeByID tests retrieving nodes by ID
func TestGetNodeByID(t *testing.T) {
	nodes := []cluster.NodeConfig{
		{ID: "node-1", Address: "localhost:50051"},
		{ID: "node-2", Address: "localhost:50052"},
		{ID: "node-3", Address: "localhost:50053"},
	}

	mgr, err := cluster.NewManager(nodes, 5*time.Second)
	if err != nil {
		t.Fatalf("Failed to create manager: %v", err)
	}
	defer mgr.Close()

	// Test existing nodes
	for _, expectedNode := range nodes {
		node, err := mgr.GetNodeByID(expectedNode.ID)
		if err != nil {
			t.Errorf("GetNodeByID('%s') failed: %v", expectedNode.ID, err)
			continue
		}

		if node.ID != expectedNode.ID {
			t.Errorf("Expected node ID '%s', got '%s'", expectedNode.ID, node.ID)
		}

		if node.Address != expectedNode.Address {
			t.Errorf("Expected address '%s', got '%s'", expectedNode.Address, node.Address)
		}

		t.Logf("Found node: %s @ %s", node.ID, node.Address)
	}

	// Test non-existent node
	_, err = mgr.GetNodeByID("non-existent")
	if err == nil {
		t.Error("Expected error for non-existent node ID, got nil")
	}
}

// TestGetHealthyNodeCount tests healthy node counting
func TestGetHealthyNodeCount(t *testing.T) {
	nodes := []cluster.NodeConfig{
		{ID: "node-1", Address: "localhost:50051"},
		{ID: "node-2", Address: "localhost:50052"},
		{ID: "node-3", Address: "localhost:50053"},
	}

	mgr, err := cluster.NewManager(nodes, 5*time.Second)
	if err != nil {
		t.Fatalf("Failed to create manager: %v", err)
	}
	defer mgr.Close()

	// Note: Nodes will likely be unhealthy since we're not running actual servers
	healthyCount := mgr.GetHealthyNodeCount()
	totalCount := len(mgr.GetAllNodes())

	t.Logf("Healthy nodes: %d/%d", healthyCount, totalCount)

	if healthyCount < 0 {
		t.Error("Healthy count should not be negative")
	}

	if healthyCount > totalCount {
		t.Errorf("Healthy count (%d) should not exceed total count (%d)",
			healthyCount, totalCount)
	}
}

// Helper functions
func testName(count int) string {
	return "nodes_" + string(rune('0'+count/10)) + string(rune('0'+count%10))
}

func testID(i int) string {
	return "node-" + string(rune('0'+i/10)) + string(rune('0'+i%10))
}

func testAddr(port int) string {
	return "localhost:" + string(rune('0'+port/10000%10)) +
		string(rune('0'+port/1000%10)) +
		string(rune('0'+port/100%10)) +
		string(rune('0'+port/10%10)) +
		string(rune('0'+port%10))
}
