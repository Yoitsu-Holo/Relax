package hash_test

import (
	"reflect"
	"testing"

	pb "github.com/yoitsuholo/relax/proto"
	testutil "github.com/yoitsuholo/relax/test/inf-master-server/testutil"
)

// TestHashBasicHSetHGet tests basic HSet and HGet operations through cluster
func TestHashBasicHSetHGet(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()
	key := "hash-key"
	field := "field1"
	value := "value1"

	// Test HSet (new field)
	setResp, err := tc.GetClient().HSet(ctx, &pb.HSetRequest{
		Key:   key,
		Field: field,
		Value: value,
	})
	if err != nil {
		t.Errorf("HSet() error = %v", err)
		return
	}
	if !setResp.Created {
		t.Errorf("HSet() new field created = false, want true")
	}

	// Test HGet
	getResp, err := tc.GetClient().HGet(ctx, &pb.HGetRequest{
		Key:   key,
		Field: field,
	})
	if err != nil {
		t.Errorf("HGet() error = %v", err)
		return
	}
	if !getResp.Exists {
		t.Errorf("HGet() exists = false, want true")
		return
	}
	if getResp.Value != value {
		t.Errorf("HGet() value = %v, want %v", getResp.Value, value)
	}

	t.Logf("Hash key %s routed to node %d", key, tc.GetNodeForKey(key))
}

// TestHashHSetOverwrite tests overwriting hash fields
func TestHashHSetOverwrite(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()
	key := "hash-key"
	field := "field1"

	// Set initial value
	setResp, err := tc.GetClient().HSet(ctx, &pb.HSetRequest{
		Key:   key,
		Field: field,
		Value: "value1",
	})
	if err != nil || !setResp.Created {
		t.Fatalf("HSet() initial error = %v, created = %v", err, setResp.Created)
	}

	// Overwrite with new value
	setResp, err = tc.GetClient().HSet(ctx, &pb.HSetRequest{
		Key:   key,
		Field: field,
		Value: "value2",
	})
	if err != nil {
		t.Errorf("HSet() overwrite error = %v", err)
	}
	if setResp.Created {
		t.Errorf("HSet() overwrite created = true, want false")
	}

	// Verify new value
	getResp, err := tc.GetClient().HGet(ctx, &pb.HGetRequest{
		Key:   key,
		Field: field,
	})
	if err != nil {
		t.Errorf("HGet() after overwrite error = %v", err)
	}
	if !getResp.Exists {
		t.Errorf("HGet() after overwrite exists = false, want true")
	}
	if getResp.Value != "value2" {
		t.Errorf("HGet() after overwrite value = %v, want value2", getResp.Value)
	}
}

// TestHashHDel tests HDel operation
func TestHashHDel(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()
	key := "hash-key"

	// Set some fields
	_, err := tc.GetClient().HSet(ctx, &pb.HSetRequest{
		Key:   key,
		Field: "field1",
		Value: "value1",
	})
	if err != nil {
		t.Fatalf("HSet() field1 error = %v", err)
	}
	_, err = tc.GetClient().HSet(ctx, &pb.HSetRequest{
		Key:   key,
		Field: "field2",
		Value: "value2",
	})
	if err != nil {
		t.Fatalf("HSet() field2 error = %v", err)
	}

	// Delete existing field
	delResp, err := tc.GetClient().HDel(ctx, &pb.HDelRequest{
		Key:   key,
		Field: "field1",
	})
	if err != nil {
		t.Errorf("HDel() existing field error = %v", err)
	}
	if delResp.DeletedCount != 1 {
		t.Errorf("HDel() existing field deleted = %v, want 1", delResp.DeletedCount)
	}

	// Verify field is deleted
	getResp, err := tc.GetClient().HGet(ctx, &pb.HGetRequest{
		Key:   key,
		Field: "field1",
	})
	if err != nil {
		t.Errorf("HGet() after HDel error = %v", err)
	}
	if getResp.Exists {
		t.Errorf("HGet() after HDel exists = true, want false")
	}

	// Verify other field still exists
	getResp, err = tc.GetClient().HGet(ctx, &pb.HGetRequest{
		Key:   key,
		Field: "field2",
	})
	if err != nil {
		t.Errorf("HGet() field2 after HDel error = %v", err)
	}
	if !getResp.Exists {
		t.Errorf("HGet() field2 after HDel exists = false, want true")
	}
	if getResp.Value != "value2" {
		t.Errorf("HGet() field2 after HDel value = %v, want value2", getResp.Value)
	}
}

// TestHashHMSetHMGet tests HMSet and HMGet operations
func TestHashHMSetHMGet(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()
	key := "hash-key"
	fields := map[string]string{
		"field1": "value1",
		"field2": "value2",
		"field3": "value3",
	}

	// Set multiple fields
	setResp, err := tc.GetClient().HMSet(ctx, &pb.HMSetRequest{
		Key:    key,
		Fields: fields,
	})
	if err != nil {
		t.Fatalf("HMSet() error = %v", err)
	}
	if setResp.Count != int32(len(fields)) {
		t.Errorf("HMSet() count = %v, want %v", setResp.Count, len(fields))
	}

	// Get multiple fields (including non-existent)
	requestFields := []string{"field1", "field2", "non-existent"}
	getResp, err := tc.GetClient().HMGet(ctx, &pb.HMGetRequest{
		Key:    key,
		Fields: requestFields,
	})
	if err != nil {
		t.Fatalf("HMGet() error = %v", err)
	}

	// Verify results
	if len(getResp.Values) != 2 {
		t.Errorf("HMGet() result length = %v, want 2", len(getResp.Values))
	}
	if getResp.Values["field1"] != "value1" {
		t.Errorf("HMGet() field1 = %v, want value1", getResp.Values["field1"])
	}
	if getResp.Values["field2"] != "value2" {
		t.Errorf("HMGet() field2 = %v, want value2", getResp.Values["field2"])
	}
	if _, exists := getResp.Values["non-existent"]; exists {
		t.Errorf("HMGet() non-existent field should not exist")
	}
}

// TestHashHLen tests HLen operation
func TestHashHLen(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()
	key := "hash-key"

	// Check length of non-existent key
	lenResp, err := tc.GetClient().HLen(ctx, &pb.HLenRequest{Key: key})
	if err != nil {
		t.Errorf("HLen() non-existent key error = %v", err)
	}
	if lenResp.Length != 0 {
		t.Errorf("HLen() non-existent key = %v, want 0", lenResp.Length)
	}

	// Add fields one by one
	for i := 1; i <= 5; i++ {
		field := "field" + string(rune('0'+i))
		_, err := tc.GetClient().HSet(ctx, &pb.HSetRequest{
			Key:   key,
			Field: field,
			Value: "value",
		})
		if err != nil {
			t.Fatalf("HSet() iteration %v error = %v", i, err)
		}

		lenResp, err := tc.GetClient().HLen(ctx, &pb.HLenRequest{Key: key})
		if err != nil {
			t.Errorf("HLen() iteration %v error = %v", i, err)
		}
		if lenResp.Length != int32(i) {
			t.Errorf("HLen() iteration %v = %v, want %v", i, lenResp.Length, i)
		}
	}
}

// TestHashHGetAll tests HGetAll operation
func TestHashHGetAll(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()
	key := "hash-key"

	// Get all from non-existent key
	getAllResp, err := tc.GetClient().HGetAll(ctx, &pb.HGetAllRequest{Key: key})
	if err != nil {
		t.Errorf("HGetAll() non-existent key error = %v", err)
	}
	if len(getAllResp.Fields) != 0 {
		t.Errorf("HGetAll() non-existent key length = %v, want 0", len(getAllResp.Fields))
	}

	// Set some fields
	expected := map[string]string{
		"field1": "value1",
		"field2": "value2",
		"field3": "value3",
	}
	_, err = tc.GetClient().HMSet(ctx, &pb.HMSetRequest{
		Key:    key,
		Fields: expected,
	})
	if err != nil {
		t.Fatalf("HMSet() error = %v", err)
	}

	// Get all fields
	getAllResp, err = tc.GetClient().HGetAll(ctx, &pb.HGetAllRequest{Key: key})
	if err != nil {
		t.Errorf("HGetAll() error = %v", err)
	}

	if !reflect.DeepEqual(getAllResp.Fields, expected) {
		t.Errorf("HGetAll() = %v, want %v", getAllResp.Fields, expected)
	}
}

// TestHashHExists tests HExists operation
func TestHashHExists(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()
	key := "hash-key"

	// Check non-existent key
	existsResp, err := tc.GetClient().HExists(ctx, &pb.HExistsRequest{
		Key:   key,
		Field: "field",
	})
	if err != nil {
		t.Errorf("HExists() non-existent key error = %v", err)
	}
	if existsResp.Exists {
		t.Errorf("HExists() non-existent key = true, want false")
	}

	// Set a field
	_, err = tc.GetClient().HSet(ctx, &pb.HSetRequest{
		Key:   key,
		Field: "field1",
		Value: "value1",
	})
	if err != nil {
		t.Fatalf("HSet() error = %v", err)
	}

	// Check existing field
	existsResp, err = tc.GetClient().HExists(ctx, &pb.HExistsRequest{
		Key:   key,
		Field: "field1",
	})
	if err != nil {
		t.Errorf("HExists() existing field error = %v", err)
	}
	if !existsResp.Exists {
		t.Errorf("HExists() existing field = false, want true")
	}

	// Check non-existent field
	existsResp, err = tc.GetClient().HExists(ctx, &pb.HExistsRequest{
		Key:   key,
		Field: "non-existent",
	})
	if err != nil {
		t.Errorf("HExists() non-existent field error = %v", err)
	}
	if existsResp.Exists {
		t.Errorf("HExists() non-existent field = true, want false")
	}
}

// TestHashMultipleHashes tests multiple hashes distributed across nodes
func TestHashMultipleHashes(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()

	// Create multiple hashes
	hashKeys := []string{"hash1", "hash2", "hash3", "hash4", "hash5"}
	nodeDistribution := make(map[int]int)

	for _, key := range hashKeys {
		// Set multiple fields in each hash
		_, err := tc.GetClient().HMSet(ctx, &pb.HMSetRequest{
			Key: key,
			Fields: map[string]string{
				"field1": "value1",
				"field2": "value2",
				"field3": "value3",
			},
		})
		if err != nil {
			t.Fatalf("HMSet(%v) error = %v", key, err)
		}

		nodeIdx := tc.GetNodeForKey(key)
		nodeDistribution[nodeIdx]++
	}

	// Log distribution
	t.Logf("Hash distribution across nodes:")
	for nodeIdx, count := range nodeDistribution {
		t.Logf("  Node %d: %d hashes", nodeIdx, count)
	}

	// Verify all hashes
	for _, key := range hashKeys {
		lenResp, err := tc.GetClient().HLen(ctx, &pb.HLenRequest{Key: key})
		if err != nil {
			t.Errorf("HLen(%v) error = %v", key, err)
		}
		if lenResp.Length != 3 {
			t.Errorf("HLen(%v) = %v, want 3", key, lenResp.Length)
		}
	}
}
