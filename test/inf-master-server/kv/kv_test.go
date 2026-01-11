package kv_test

import (
	"testing"

	pb "github.com/yoitsuholo/relax/proto"
	"github.com/yoitsuholo/relax/test/inf-master-server/testutil"
)

// TestKVBasicSetGet tests basic Set and Get operations through cluster
func TestKVBasicSetGet(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	tests := []struct {
		name  string
		key   string
		value string
	}{
		{"simple value", "key1", "value1"},
		{"empty value", "key2", ""},
		{"long value", "key3", "very long value with special characters !@#$%^&*()"},
		{"unicode value", "key4", "你好世界🌍"},
		{"numeric string", "key5", "12345"},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ctx := testutil.TestContext()

			// Test Set
			setResp, err := tc.GetClient().Set(ctx, &pb.KvSetRequest{
				Key:   tt.key,
				Value: tt.value,
			})
			if err != nil {
				t.Errorf("Set() error = %v", err)
				return
			}
			if !setResp.Success {
				t.Errorf("Set() success = false, want true")
				return
			}

			// Test Get
			getResp, err := tc.GetClient().Get(ctx, &pb.KvGetRequest{
				Key: tt.key,
			})
			if err != nil {
				t.Errorf("Get() error = %v", err)
				return
			}
			if !getResp.Exists {
				t.Errorf("Get() exists = false, want true")
				return
			}
			if getResp.Value != tt.value {
				t.Errorf("Get() value = %v, want %v", getResp.Value, tt.value)
			}

			// Verify key routing
			nodeIdx := tc.GetNodeForKey(tt.key)
			t.Logf("Key %s routed to node %d", tt.key, nodeIdx)
		})
	}
}

// TestKVGetNonExistent tests Get on non-existent key
func TestKVGetNonExistent(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()

	getResp, err := tc.GetClient().Get(ctx, &pb.KvGetRequest{
		Key: "non-existent-key",
	})
	if err != nil {
		t.Errorf("Get() error = %v, want nil", err)
	}
	if getResp.Exists {
		t.Errorf("Get() exists = true, want false")
	}
	if getResp.Value != "" {
		t.Errorf("Get() value = %v, want empty string", getResp.Value)
	}
}

// TestKVSetOverwrite tests overwriting existing keys
func TestKVSetOverwrite(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()
	key := "overwrite-key"

	// Set initial value
	_, err := tc.GetClient().Set(ctx, &pb.KvSetRequest{
		Key:   key,
		Value: "value1",
	})
	if err != nil {
		t.Fatalf("Set() error = %v", err)
	}

	// Verify initial value
	getResp, err := tc.GetClient().Get(ctx, &pb.KvGetRequest{Key: key})
	if err != nil || !getResp.Exists || getResp.Value != "value1" {
		t.Fatalf("Get() after first Set failed: value=%v, exists=%v, err=%v",
			getResp.Value, getResp.Exists, err)
	}

	// Overwrite with new value
	_, err = tc.GetClient().Set(ctx, &pb.KvSetRequest{
		Key:   key,
		Value: "value2",
	})
	if err != nil {
		t.Fatalf("Set() overwrite error = %v", err)
	}

	// Verify new value
	getResp, err = tc.GetClient().Get(ctx, &pb.KvGetRequest{Key: key})
	if err != nil {
		t.Errorf("Get() after overwrite error = %v", err)
	}
	if !getResp.Exists {
		t.Errorf("Get() after overwrite exists = false, want true")
	}
	if getResp.Value != "value2" {
		t.Errorf("Get() after overwrite value = %v, want value2", getResp.Value)
	}
}

// TestKVDel tests Delete operations
func TestKVDel(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()
	key := "delete-key"

	// Delete non-existent key
	delResp, err := tc.GetClient().Del(ctx, &pb.KvDelRequest{Key: key})
	if err != nil {
		t.Errorf("Del() non-existent key error = %v", err)
	}
	if delResp.DeletedCount != 0 {
		t.Errorf("Del() non-existent key deleted = %v, want 0", delResp.DeletedCount)
	}

	// Set a value
	_, err = tc.GetClient().Set(ctx, &pb.KvSetRequest{
		Key:   key,
		Value: "value",
	})
	if err != nil {
		t.Fatalf("Set() error = %v", err)
	}

	// Delete existing key
	delResp, err = tc.GetClient().Del(ctx, &pb.KvDelRequest{Key: key})
	if err != nil {
		t.Errorf("Del() existing key error = %v", err)
	}
	if delResp.DeletedCount != 1 {
		t.Errorf("Del() existing key deleted = %v, want 1", delResp.DeletedCount)
	}

	// Verify key is deleted
	getResp, err := tc.GetClient().Get(ctx, &pb.KvGetRequest{Key: key})
	if err != nil {
		t.Errorf("Get() after Del error = %v", err)
	}
	if getResp.Exists {
		t.Errorf("Get() after Del exists = true, want false")
	}
}

// TestKVExists tests Exists operation
func TestKVExists(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()
	key := "exists-key"

	// Check non-existent key
	existsResp, err := tc.GetClient().Exists(ctx, &pb.KvExistsRequest{Key: key})
	if err != nil {
		t.Errorf("Exists() non-existent key error = %v", err)
	}
	if existsResp.Exists {
		t.Errorf("Exists() non-existent key = true, want false")
	}

	// Set a value
	_, err = tc.GetClient().Set(ctx, &pb.KvSetRequest{
		Key:   key,
		Value: "value",
	})
	if err != nil {
		t.Fatalf("Set() error = %v", err)
	}

	// Check existing key
	existsResp, err = tc.GetClient().Exists(ctx, &pb.KvExistsRequest{Key: key})
	if err != nil {
		t.Errorf("Exists() existing key error = %v", err)
	}
	if !existsResp.Exists {
		t.Errorf("Exists() existing key = false, want true")
	}

	// Delete and check again
	_, err = tc.GetClient().Del(ctx, &pb.KvDelRequest{Key: key})
	if err != nil {
		t.Fatalf("Del() error = %v", err)
	}

	existsResp, err = tc.GetClient().Exists(ctx, &pb.KvExistsRequest{Key: key})
	if err != nil {
		t.Errorf("Exists() after Del error = %v", err)
	}
	if existsResp.Exists {
		t.Errorf("Exists() after Del = true, want false")
	}
}

// TestKVMultipleKeys tests operations with multiple keys distributed across nodes
func TestKVMultipleKeys(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()

	// Set multiple keys
	keys := []string{"key1", "key2", "key3", "key4", "key5", "key6", "key7", "key8", "key9", "key10"}
	nodeDistribution := make(map[int]int) // Track distribution across nodes

	for i, key := range keys {
		_, err := tc.GetClient().Set(ctx, &pb.KvSetRequest{
			Key:   key,
			Value: key + "-value",
		})
		if err != nil {
			t.Fatalf("Set(%v) error = %v", i, err)
		}

		// Track node distribution
		nodeIdx := tc.GetNodeForKey(key)
		nodeDistribution[nodeIdx]++
	}

	// Log distribution
	t.Logf("Key distribution across nodes:")
	for nodeIdx, count := range nodeDistribution {
		t.Logf("  Node %d: %d keys", nodeIdx, count)
	}

	// Verify keys are distributed (at least 2 nodes should have keys for 10 keys across 3 nodes)
	if len(nodeDistribution) < 2 {
		t.Errorf("Keys not well distributed, only %d nodes have keys", len(nodeDistribution))
	}

	// Get all keys
	for i, key := range keys {
		getResp, err := tc.GetClient().Get(ctx, &pb.KvGetRequest{Key: key})
		if err != nil {
			t.Errorf("Get(%v) error = %v", i, err)
		}
		if !getResp.Exists {
			t.Errorf("Get(%v) exists = false, want true", i)
		}
		if getResp.Value != key+"-value" {
			t.Errorf("Get(%v) value = %v, want %v", i, getResp.Value, key+"-value")
		}
	}

	// Delete some keys
	for i := 0; i < 5; i++ {
		delResp, err := tc.GetClient().Del(ctx, &pb.KvDelRequest{Key: keys[i]})
		if err != nil {
			t.Errorf("Del(%v) error = %v", i, err)
		}
		if delResp.DeletedCount != 1 {
			t.Errorf("Del(%v) deleted = %v, want 1", i, delResp.DeletedCount)
		}
	}

	// Verify deleted keys
	for i := 0; i < 5; i++ {
		existsResp, err := tc.GetClient().Exists(ctx, &pb.KvExistsRequest{Key: keys[i]})
		if err != nil {
			t.Errorf("Exists(%v) after Del error = %v", i, err)
		}
		if existsResp.Exists {
			t.Errorf("Exists(%v) after Del = true, want false", i)
		}
	}

	// Verify remaining keys
	for i := 5; i < len(keys); i++ {
		existsResp, err := tc.GetClient().Exists(ctx, &pb.KvExistsRequest{Key: keys[i]})
		if err != nil {
			t.Errorf("Exists(%v) remaining key error = %v", i, err)
		}
		if !existsResp.Exists {
			t.Errorf("Exists(%v) remaining key = false, want true", i)
		}
	}
}

// TestKVSingleNode tests cluster with single node
func TestKVSingleNode(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 1)
	defer tc.Teardown()

	ctx := testutil.TestContext()

	// Set and get a value
	_, err := tc.GetClient().Set(ctx, &pb.KvSetRequest{
		Key:   "test-key",
		Value: "test-value",
	})
	if err != nil {
		t.Fatalf("Set() error = %v", err)
	}

	getResp, err := tc.GetClient().Get(ctx, &pb.KvGetRequest{Key: "test-key"})
	if err != nil {
		t.Errorf("Get() error = %v", err)
	}
	if !getResp.Exists {
		t.Errorf("Get() exists = false, want true")
	}
	if getResp.Value != "test-value" {
		t.Errorf("Get() value = %v, want test-value", getResp.Value)
	}
}

// TestKVManyNodes tests cluster with many nodes
func TestKVManyNodes(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 5)
	defer tc.Teardown()

	ctx := testutil.TestContext()

	// Set many keys
	numKeys := 50
	nodeDistribution := make(map[int]int)

	for i := 0; i < numKeys; i++ {
		key := "test-key-" + string(rune('0'+i))
		_, err := tc.GetClient().Set(ctx, &pb.KvSetRequest{
			Key:   key,
			Value: "value",
		})
		if err != nil {
			t.Fatalf("Set(%v) error = %v", i, err)
		}

		nodeIdx := tc.GetNodeForKey(key)
		nodeDistribution[nodeIdx]++
	}

	// Log distribution
	t.Logf("Distribution of %d keys across 5 nodes:", numKeys)
	for nodeIdx, count := range nodeDistribution {
		t.Logf("  Node %d: %d keys (%.1f%%)", nodeIdx, count, float64(count)/float64(numKeys)*100)
	}

	// Verify all nodes have some keys (with 50 keys and 5 nodes, all should have keys)
	if len(nodeDistribution) < 5 {
		t.Errorf("Not all nodes received keys, only %d nodes have keys", len(nodeDistribution))
	}
}
