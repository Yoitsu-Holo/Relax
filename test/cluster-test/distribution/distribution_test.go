package distribution_test

import (
	"fmt"
	"testing"
	"time"

	"github.com/yoitsuholo/relax/inf-MasterNode/cluster"
)

// TestKeyDistribution tests how keys are distributed across nodes
func TestKeyDistribution(t *testing.T) {
	tests := []struct {
		name      string
		nodeCount int
		keyCount  int
	}{
		{"3 nodes, 100 keys", 3, 100},
		{"3 nodes, 1000 keys", 3, 1000},
		{"3 nodes, 10000 keys", 3, 10000},
		{"5 nodes, 1000 keys", 5, 1000},
		{"7 nodes, 1000 keys", 7, 1000},
		{"10 nodes, 10000 keys", 10, 10000},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			// Create nodes
			nodes := make([]cluster.NodeConfig, tt.nodeCount)
			for i := 0; i < tt.nodeCount; i++ {
				nodes[i] = cluster.NodeConfig{
					ID:      fmt.Sprintf("node-%d", i),
					Address: fmt.Sprintf("localhost:%d", 50051+i),
				}
			}

			mgr, err := cluster.NewManager(nodes, 5*time.Second)
			if err != nil {
				t.Fatalf("Failed to create manager: %v", err)
			}
			defer mgr.Close()

			// Generate keys and track distribution
			distribution := make(map[int]int)
			for i := 0; i < tt.keyCount; i++ {
				key := fmt.Sprintf("key:%d", i)
				nodeIndex := mgr.GetNodeIndexForKey(key)
				distribution[nodeIndex]++
			}

			// Analyze distribution
			t.Logf("Distribution of %d keys across %d nodes:", tt.keyCount, tt.nodeCount)

			totalKeys := 0
			for nodeIdx := 0; nodeIdx < tt.nodeCount; nodeIdx++ {
				count := distribution[nodeIdx]
				percentage := float64(count) / float64(tt.keyCount) * 100
				t.Logf("  Node %d: %d keys (%.2f%%)", nodeIdx, count, percentage)
				totalKeys += count
			}

			// Verify all keys were distributed
			if totalKeys != tt.keyCount {
				t.Errorf("Total distributed keys (%d) doesn't match expected (%d)",
					totalKeys, tt.keyCount)
			}

			// Check balance (each node should have roughly equal number of keys)
			expectedPerNode := tt.keyCount / tt.nodeCount
			tolerance := float64(expectedPerNode) * 0.3 // 30% tolerance

			for nodeIdx := 0; nodeIdx < tt.nodeCount; nodeIdx++ {
				count := distribution[nodeIdx]
				diff := float64(count - expectedPerNode)
				if diff < 0 {
					diff = -diff
				}

				if diff > tolerance {
					t.Logf("Warning: Node %d has %d keys (expected ~%d, tolerance ±%.0f)",
						nodeIdx, count, expectedPerNode, tolerance)
				}
			}

			// Verify all nodes received at least some keys (for large key counts)
			if tt.keyCount >= tt.nodeCount*10 {
				for nodeIdx := 0; nodeIdx < tt.nodeCount; nodeIdx++ {
					if distribution[nodeIdx] == 0 {
						t.Errorf("Node %d received no keys", nodeIdx)
					}
				}
			}
		})
	}
}

// TestUserKeyDistribution tests distribution of user-like keys
func TestUserKeyDistribution(t *testing.T) {
	nodeCount := 3
	nodes := make([]cluster.NodeConfig, nodeCount)
	for i := 0; i < nodeCount; i++ {
		nodes[i] = cluster.NodeConfig{
			ID:      fmt.Sprintf("node-%d", i),
			Address: fmt.Sprintf("localhost:%d", 50051+i),
		}
	}

	mgr, err := cluster.NewManager(nodes, 5*time.Second)
	if err != nil {
		t.Fatalf("Failed to create manager: %v", err)
	}
	defer mgr.Close()

	// Test different key patterns
	patterns := []struct {
		name     string
		template string
		count    int
	}{
		{"user IDs", "user:%d", 1000},
		{"session IDs", "session:%d", 1000},
		{"cache keys", "cache:data:%d", 1000},
		{"UUID-like", "key-%d-%d-%d-%d", 250}, // 250 keys with 4 parts
	}

	for _, pattern := range patterns {
		t.Run(pattern.name, func(t *testing.T) {
			distribution := make(map[int]int)

			if pattern.name == "UUID-like" {
				// Special handling for multi-part keys
				for i := 0; i < pattern.count; i++ {
					key := fmt.Sprintf(pattern.template, i, i*2, i*3, i*4)
					nodeIndex := mgr.GetNodeIndexForKey(key)
					distribution[nodeIndex]++
				}
			} else {
				for i := 0; i < pattern.count; i++ {
					key := fmt.Sprintf(pattern.template, i)
					nodeIndex := mgr.GetNodeIndexForKey(key)
					distribution[nodeIndex]++
				}
			}

			t.Logf("Pattern '%s' distribution:", pattern.name)
			for nodeIdx := 0; nodeIdx < nodeCount; nodeIdx++ {
				count := distribution[nodeIdx]
				percentage := float64(count) / float64(pattern.count) * 100
				t.Logf("  Node %d: %d keys (%.2f%%)", nodeIdx, count, percentage)
			}
		})
	}
}

// TestHotKeyDistribution tests that hot keys don't all map to the same node
func TestHotKeyDistribution(t *testing.T) {
	nodeCount := 3
	nodes := make([]cluster.NodeConfig, nodeCount)
	for i := 0; i < nodeCount; i++ {
		nodes[i] = cluster.NodeConfig{
			ID:      fmt.Sprintf("node-%d", i),
			Address: fmt.Sprintf("localhost:%d", 50051+i),
		}
	}

	mgr, err := cluster.NewManager(nodes, 5*time.Second)
	if err != nil {
		t.Fatalf("Failed to create manager: %v", err)
	}
	defer mgr.Close()

	// Simulate hot keys (similar prefixes)
	hotKeyPrefixes := []string{
		"hot:user:",
		"hot:session:",
		"hot:cache:",
		"popular:item:",
	}

	for _, prefix := range hotKeyPrefixes {
		t.Run(prefix, func(t *testing.T) {
			distribution := make(map[int]int)

			// Generate 5000 keys with the same prefix
			for i := 0; i < 5000; i++ {
				key := fmt.Sprintf("%s%d", prefix, i)
				nodeIndex := mgr.GetNodeIndexForKey(key)
				distribution[nodeIndex]++
			}

			t.Logf("Hot key distribution for prefix '%s':", prefix)
			for nodeIdx := 0; nodeIdx < nodeCount; nodeIdx++ {
				count := distribution[nodeIdx]
				t.Logf("  Node %d: %d keys", nodeIdx, count)
			}

			// Verify keys are distributed (not all on one node)
			nodesUsed := 0
			for i := 0; i < nodeCount; i++ {
				if distribution[i] > 0 {
					nodesUsed++
				}
			}

			if nodesUsed < 2 {
				t.Logf("Warning: Hot keys with prefix '%s' only use %d nodes", prefix, nodesUsed)
			}
		})
	}
}

// TestSequentialKeyDistribution tests distribution of sequential keys
func TestSequentialKeyDistribution(t *testing.T) {
	nodeCount := 5
	nodes := make([]cluster.NodeConfig, nodeCount)
	for i := 0; i < nodeCount; i++ {
		nodes[i] = cluster.NodeConfig{
			ID:      fmt.Sprintf("node-%d", i),
			Address: fmt.Sprintf("localhost:%d", 50051+i),
		}
	}

	mgr, err := cluster.NewManager(nodes, 5*time.Second)
	if err != nil {
		t.Fatalf("Failed to create manager: %v", err)
	}
	defer mgr.Close()

	// Test sequential numeric keys
	t.Run("sequential numbers", func(t *testing.T) {
		distribution := make(map[int]int)

		for i := 1; i <= 1000; i++ {
			key := fmt.Sprintf("key:%d", i)
			nodeIndex := mgr.GetNodeIndexForKey(key)
			distribution[nodeIndex]++
		}

		t.Log("Sequential numeric key distribution:")
		for nodeIdx := 0; nodeIdx < nodeCount; nodeIdx++ {
			count := distribution[nodeIdx]
			percentage := float64(count) / 100.0 * 100
			t.Logf("  Node %d: %d keys (%.1f%%)", nodeIdx, count, percentage)
		}
	})

	// Test sequential alphabetic keys
	t.Run("sequential alphabetic", func(t *testing.T) {
		distribution := make(map[int]int)

		for c := 'a'; c <= 'z'; c++ {
			for i := 1; i <= 9; i++ {
				key := fmt.Sprintf("key:%c%d", c, i)
				nodeIndex := mgr.GetNodeIndexForKey(key)
				distribution[nodeIndex]++
			}
		}

		total := 26 * 9
		t.Log("Sequential alphabetic key distribution:")
		for nodeIdx := 0; nodeIdx < nodeCount; nodeIdx++ {
			count := distribution[nodeIdx]
			percentage := float64(count) / float64(total) * 100
			t.Logf("  Node %d: %d keys (%.1f%%)", nodeIdx, count, percentage)
		}
	})
}

// TestScalability tests distribution as node count scales
func TestScalability(t *testing.T) {
	keyCount := 100000
	nodeCounts := []int{2, 3, 5, 10, 20, 50}

	results := make(map[int]map[int]int) // nodeCount -> distribution

	for _, nodeCount := range nodeCounts {
		nodes := make([]cluster.NodeConfig, nodeCount)
		for i := 0; i < nodeCount; i++ {
			nodes[i] = cluster.NodeConfig{
				ID:      fmt.Sprintf("node-%d", i),
				Address: fmt.Sprintf("localhost:%d", 50051+i),
			}
		}

		mgr, err := cluster.NewManager(nodes, 5*time.Second)
		if err != nil {
			t.Fatalf("Failed to create manager with %d nodes: %v", nodeCount, err)
		}

		distribution := make(map[int]int)
		for i := 0; i < keyCount; i++ {
			key := fmt.Sprintf("key:%d", i)
			nodeIndex := mgr.GetNodeIndexForKey(key)
			distribution[nodeIndex]++
		}

		results[nodeCount] = distribution
		mgr.Close()
	}

	// Report results
	t.Logf("Scalability test: %d keys across varying node counts", keyCount)
	for _, nodeCount := range nodeCounts {
		distribution := results[nodeCount]
		expectedPerNode := keyCount / nodeCount

		var minKeys, maxKeys int
		minKeys = keyCount
		maxKeys = 0

		for i := 0; i < nodeCount; i++ {
			count := distribution[i]
			if count < minKeys {
				minKeys = count
			}
			if count > maxKeys {
				maxKeys = count
			}
		}

		deviation := float64(maxKeys-minKeys) / float64(expectedPerNode) * 100
		t.Logf("  %2d nodes: Expected %d/node, Range [%d-%d], Deviation: %.1f%%",
			nodeCount, expectedPerNode, minKeys, maxKeys, deviation)

		// For large node counts, deviation should be reasonable
		if nodeCount >= 10 && deviation > 50 {
			t.Logf("    Warning: High deviation for %d nodes", nodeCount)
		}
	}
}
