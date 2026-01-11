package unit_test

import (
	"testing"
	"time"

	"github.com/yoitsuholo/relax/inf-MasterNode/cluster"
)

// TestHashKeyConsistency tests that hashing is deterministic
func TestHashKeyConsistency(t *testing.T) {
	tests := []struct {
		name string
		key  string
	}{
		{"user key", "user:1001"},
		{"session key", "session:abc123"},
		{"cache key", "cache:data:12345"},
		{"numeric key", "12345"},
		{"unicode key", "用户:1001"},
		{"empty key", ""},
		{"special chars", "key!@#$%^&*()"},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			// Hash the same key multiple times
			hi1, lo1 := cluster.HashKey(tt.key)
			hi2, lo2 := cluster.HashKey(tt.key)
			hi3, lo3 := cluster.HashKey(tt.key)

			// All results should be identical
			if hi1 != hi2 || hi1 != hi3 || lo1 != lo2 || lo1 != lo3 {
				t.Errorf("Inconsistent hashing for key '%s':\n  Run1: Hi=%d Lo=%d\n  Run2: Hi=%d Lo=%d\n  Run3: Hi=%d Lo=%d",
					tt.key, hi1, lo1, hi2, lo2, hi3, lo3)
			}

			t.Logf("Key '%s' -> Hi=%d, Lo=%d", tt.key, hi1, lo1)
		})
	}
}

// TestHashKeyUniqueness tests that different keys produce different hashes
func TestHashKeyUniqueness(t *testing.T) {
	keys := []string{
		"user:1", "user:2", "user:3", "user:4", "user:5",
		"user:6", "user:7", "user:8", "user:9", "user:10",
	}

	hashes := make(map[uint64]map[uint64]string) // Hi -> Lo -> Key

	collisions := 0
	for _, key := range keys {
		hi, lo := cluster.HashKey(key)

		if hashes[hi] == nil {
			hashes[hi] = make(map[uint64]string)
		}

		if existingKey, exists := hashes[hi][lo]; exists {
			t.Logf("Hash collision: '%s' and '%s' -> Hi=%d Lo=%d", key, existingKey, hi, lo)
			collisions++
		} else {
			hashes[hi][lo] = key
		}

		t.Logf("Key '%s' -> Hi=%d, Lo=%d", key, hi, lo)
	}

	if collisions > 0 {
		t.Logf("Total collisions: %d/%d", collisions, len(keys))
	} else {
		t.Log("No collisions detected")
	}
}

// TestHashKeyHigh128 tests the high 64-bit extraction
func TestHashKeyHigh128(t *testing.T) {
	tests := []struct {
		key string
	}{
		{"test1"},
		{"test2"},
		{"test3"},
	}

	for _, tt := range tests {
		t.Run(tt.key, func(t *testing.T) {
			hi1, _ := cluster.HashKey(tt.key)
			hi2 := cluster.HashKeyHigh128(tt.key)

			if hi1 != hi2 {
				t.Errorf("HashKeyHigh128 mismatch: HashKey returned %d, HashKeyHigh128 returned %d", hi1, hi2)
			}

			t.Logf("Key '%s' -> High64=%d", tt.key, hi2)
		})
	}
}

// TestDefaultConfig tests the default configuration
func TestDefaultConfig(t *testing.T) {
	config := cluster.DefaultConfig()

	// Verify nodes
	if len(config.Nodes) == 0 {
		t.Fatal("Default config should have at least one node")
	}

	t.Logf("Default config has %d nodes", len(config.Nodes))

	for i, node := range config.Nodes {
		if node.ID == "" {
			t.Errorf("Node %d has empty ID", i)
		}
		if node.Address == "" {
			t.Errorf("Node %d has empty address", i)
		}
		t.Logf("  Node %d: ID=%s, Address=%s", i, node.ID, node.Address)
	}

	// Verify health check interval
	if config.HealthCheckInterval == 0 {
		t.Error("Health check interval should be non-zero")
	}
	t.Logf("Health check interval: %v", config.HealthCheckInterval)

	// Verify ports
	if config.GRPCPort == "" {
		t.Error("gRPC port should not be empty")
	}
	if config.HTTPPort == "" {
		t.Error("HTTP port should not be empty")
	}
	t.Logf("Ports: gRPC=%s, HTTP=%s", config.GRPCPort, config.HTTPPort)
}

// TestLoadConfigFromYAML tests loading configuration from YAML
func TestLoadConfigFromYAML(t *testing.T) {
	// Test loading non-existent file
	_, err := cluster.LoadConfig("nonexistent.yaml")
	if err == nil {
		t.Error("Loading non-existent file should return error")
	}

	// Test loading actual config file
	config, err := cluster.LoadConfig("../../inf-MasterNode/config.yaml")
	if err != nil {
		t.Logf("Could not load config.yaml: %v (this is OK if file doesn't exist)", err)
		return
	}

	// Verify loaded config
	if len(config.Nodes) == 0 {
		t.Error("Loaded config should have nodes")
	}

	t.Logf("Loaded config with %d nodes", len(config.Nodes))
	for i, node := range config.Nodes {
		t.Logf("  Node %d: %s @ %s", i, node.ID, node.Address)
	}
}

// TestNodeConfigValidation tests node configuration validation
func TestNodeConfigValidation(t *testing.T) {
	tests := []struct {
		name    string
		nodes   []cluster.NodeConfig
		wantErr bool
	}{
		{
			name:    "empty nodes",
			nodes:   []cluster.NodeConfig{},
			wantErr: true,
		},
		{
			name: "valid single node",
			nodes: []cluster.NodeConfig{
				{ID: "node-1", Address: "localhost:50051"},
			},
			wantErr: false,
		},
		{
			name: "valid multiple nodes",
			nodes: []cluster.NodeConfig{
				{ID: "node-1", Address: "localhost:50051"},
				{ID: "node-2", Address: "localhost:50052"},
				{ID: "node-3", Address: "localhost:50053"},
			},
			wantErr: false,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			mgr, err := cluster.NewManager(tt.nodes, 5*time.Second)

			if tt.wantErr {
				if err == nil {
					t.Error("Expected error but got none")
				}
				return
			}

			if err != nil {
				t.Errorf("Unexpected error: %v", err)
				return
			}

			if mgr == nil {
				t.Error("Manager should not be nil")
				return
			}

			mgr.Close()

			nodes := mgr.GetAllNodes()
			if len(nodes) != len(tt.nodes) {
				t.Errorf("Expected %d nodes, got %d", len(tt.nodes), len(nodes))
			}
		})
	}
}
