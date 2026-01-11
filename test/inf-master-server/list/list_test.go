package list_test

import (
	"reflect"
	"testing"

	pb "github.com/yoitsuholo/relax/proto"
	testutil "github.com/yoitsuholo/relax/test/inf-master-server/testutil"
)

// TestListBasicLPushLRange tests basic LPush and LRange operations through cluster
func TestListBasicLPushLRange(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()
	key := "list-key"

	// Push values
	values := []string{"value3", "value2", "value1"}
	for i, value := range values {
		pushResp, err := tc.GetClient().LPush(ctx, &pb.LPushRequest{
			Key:   key,
			Value: value,
		})
		if err != nil {
			t.Errorf("LPush(%v) error = %v", i, err)
			return
		}
		if pushResp.Length != int32(i+1) {
			t.Errorf("LPush(%v) length = %v, want %v", i, pushResp.Length, i+1)
		}
	}

	// Get range
	rangeResp, err := tc.GetClient().LRange(ctx, &pb.LRangeRequest{
		Key:   key,
		Start: 0,
		Stop:  -1,
	})
	if err != nil {
		t.Errorf("LRange() error = %v", err)
		return
	}

	expected := []string{"value1", "value2", "value3"}
	if !reflect.DeepEqual(rangeResp.Values, expected) {
		t.Errorf("LRange() = %v, want %v", rangeResp.Values, expected)
	}

	t.Logf("List key %s routed to node %d", key, tc.GetNodeForKey(key))
}

// TestListRPush tests RPush operation
func TestListRPush(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()
	key := "list-key"

	// Push values from right
	values := []string{"value1", "value2", "value3"}
	for i, value := range values {
		pushResp, err := tc.GetClient().RPush(ctx, &pb.RPushRequest{
			Key:   key,
			Value: value,
		})
		if err != nil {
			t.Errorf("RPush(%v) error = %v", i, err)
			return
		}
		if pushResp.Length != int32(i+1) {
			t.Errorf("RPush(%v) length = %v, want %v", i, pushResp.Length, i+1)
		}
	}

	// Get range
	rangeResp, err := tc.GetClient().LRange(ctx, &pb.LRangeRequest{
		Key:   key,
		Start: 0,
		Stop:  -1,
	})
	if err != nil {
		t.Errorf("LRange() error = %v", err)
		return
	}

	if !reflect.DeepEqual(rangeResp.Values, values) {
		t.Errorf("LRange() = %v, want %v", rangeResp.Values, values)
	}
}

// TestListLPopRPop tests LPop and RPop operations
func TestListLPopRPop(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()
	key := "list-key"

	// Push values
	values := []string{"value1", "value2", "value3", "value4", "value5"}
	for _, value := range values {
		_, err := tc.GetClient().RPush(ctx, &pb.RPushRequest{
			Key:   key,
			Value: value,
		})
		if err != nil {
			t.Fatalf("RPush() error = %v", err)
		}
	}

	// LPop
	popResp, err := tc.GetClient().LPop(ctx, &pb.LPopRequest{Key: key})
	if err != nil {
		t.Errorf("LPop() error = %v", err)
	}
	if !popResp.Success {
		t.Errorf("LPop() success = false, want true")
	}
	if popResp.Value != "value1" {
		t.Errorf("LPop() value = %v, want value1", popResp.Value)
	}

	// RPop
	rPopResp, err := tc.GetClient().RPop(ctx, &pb.RPopRequest{Key: key})
	if err != nil {
		t.Errorf("RPop() error = %v", err)
	}
	if !rPopResp.Success {
		t.Errorf("RPop() success = false, want true")
	}
	if rPopResp.Value != "value5" {
		t.Errorf("RPop() value = %v, want value5", rPopResp.Value)
	}

	// Verify remaining values
	rangeResp, err := tc.GetClient().LRange(ctx, &pb.LRangeRequest{
		Key:   key,
		Start: 0,
		Stop:  -1,
	})
	if err != nil {
		t.Errorf("LRange() error = %v", err)
	}

	expected := []string{"value2", "value3", "value4"}
	if !reflect.DeepEqual(rangeResp.Values, expected) {
		t.Errorf("LRange() = %v, want %v", rangeResp.Values, expected)
	}

	// Pop from empty list
	_, _ = tc.GetClient().LPop(ctx, &pb.LPopRequest{Key: key})
	_, _ = tc.GetClient().LPop(ctx, &pb.LPopRequest{Key: key})
	_, _ = tc.GetClient().LPop(ctx, &pb.LPopRequest{Key: key})

	popResp, err = tc.GetClient().LPop(ctx, &pb.LPopRequest{Key: key})
	if err != nil {
		t.Errorf("LPop() empty list error = %v", err)
	}
	if popResp.Success {
		t.Errorf("LPop() empty list success = true, want false")
	}
}

// TestListLLen tests LLen operation
func TestListLLen(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()
	key := "list-key"

	// Check length of non-existent list
	lenResp, err := tc.GetClient().LLen(ctx, &pb.LLenRequest{Key: key})
	if err != nil {
		t.Errorf("LLen() non-existent list error = %v", err)
	}
	if lenResp.Length != 0 {
		t.Errorf("LLen() non-existent list = %v, want 0", lenResp.Length)
	}

	// Push values
	for i := 1; i <= 5; i++ {
		_, err := tc.GetClient().RPush(ctx, &pb.RPushRequest{
			Key:   key,
			Value: "value",
		})
		if err != nil {
			t.Fatalf("RPush() iteration %v error = %v", i, err)
		}

		lenResp, err := tc.GetClient().LLen(ctx, &pb.LLenRequest{Key: key})
		if err != nil {
			t.Errorf("LLen() iteration %v error = %v", i, err)
		}
		if lenResp.Length != int32(i) {
			t.Errorf("LLen() iteration %v = %v, want %v", i, lenResp.Length, i)
		}
	}
}

// TestListLIndex tests LIndex operation
func TestListLIndex(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()
	key := "list-key"

	// Push values
	values := []string{"value1", "value2", "value3", "value4", "value5"}
	for _, value := range values {
		_, err := tc.GetClient().RPush(ctx, &pb.RPushRequest{
			Key:   key,
			Value: value,
		})
		if err != nil {
			t.Fatalf("RPush() error = %v", err)
		}
	}

	// Test positive indices
	for i, expected := range values {
		indexResp, err := tc.GetClient().LIndex(ctx, &pb.LIndexRequest{
			Key:   key,
			Index: int32(i),
		})
		if err != nil {
			t.Errorf("LIndex(%v) error = %v", i, err)
		}
		if !indexResp.Exists {
			t.Errorf("LIndex(%v) exists = false, want true", i)
		}
		if indexResp.Value != expected {
			t.Errorf("LIndex(%v) = %v, want %v", i, indexResp.Value, expected)
		}
	}

	// Test negative index
	indexResp, err := tc.GetClient().LIndex(ctx, &pb.LIndexRequest{
		Key:   key,
		Index: -1,
	})
	if err != nil {
		t.Errorf("LIndex(-1) error = %v", err)
	}
	if !indexResp.Exists {
		t.Errorf("LIndex(-1) exists = false, want true")
	}
	if indexResp.Value != "value5" {
		t.Errorf("LIndex(-1) = %v, want value5", indexResp.Value)
	}

	// Test out of range index
	indexResp, err = tc.GetClient().LIndex(ctx, &pb.LIndexRequest{
		Key:   key,
		Index: 100,
	})
	if err != nil {
		t.Errorf("LIndex(100) error = %v", err)
	}
	if indexResp.Exists {
		t.Errorf("LIndex(100) exists = true, want false")
	}
}

// TestListLSet tests LSet operation
func TestListLSet(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()
	key := "list-key"

	// Push values
	values := []string{"value1", "value2", "value3"}
	for _, value := range values {
		_, err := tc.GetClient().RPush(ctx, &pb.RPushRequest{
			Key:   key,
			Value: value,
		})
		if err != nil {
			t.Fatalf("RPush() error = %v", err)
		}
	}

	// Set a value
	setResp, err := tc.GetClient().LSet(ctx, &pb.LSetRequest{
		Key:   key,
		Index: 1,
		Value: "new-value",
	})
	if err != nil {
		t.Errorf("LSet() error = %v", err)
	}
	if !setResp.Success {
		t.Errorf("LSet() success = false, want true")
	}

	// Verify the change
	indexResp, err := tc.GetClient().LIndex(ctx, &pb.LIndexRequest{
		Key:   key,
		Index: 1,
	})
	if err != nil {
		t.Errorf("LIndex() after LSet error = %v", err)
	}
	if indexResp.Value != "new-value" {
		t.Errorf("LIndex() after LSet = %v, want new-value", indexResp.Value)
	}

	// Verify other values unchanged
	expected := []string{"value1", "new-value", "value3"}
	rangeResp, err := tc.GetClient().LRange(ctx, &pb.LRangeRequest{
		Key:   key,
		Start: 0,
		Stop:  -1,
	})
	if err != nil {
		t.Errorf("LRange() after LSet error = %v", err)
	}
	if !reflect.DeepEqual(rangeResp.Values, expected) {
		t.Errorf("LRange() after LSet = %v, want %v", rangeResp.Values, expected)
	}
}

// TestListLRem tests LRem operation
func TestListLRem(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()
	key := "list-key"

	// Push values with duplicates
	values := []string{"a", "b", "a", "c", "a", "d"}
	for _, value := range values {
		_, err := tc.GetClient().RPush(ctx, &pb.RPushRequest{
			Key:   key,
			Value: value,
		})
		if err != nil {
			t.Fatalf("RPush() error = %v", err)
		}
	}

	// Remove 2 occurrences of "a"
	remResp, err := tc.GetClient().LRem(ctx, &pb.LRemRequest{
		Key:   key,
		Count: 2,
		Value: "a",
	})
	if err != nil {
		t.Errorf("LRem() error = %v", err)
	}
	if remResp.DeletedCount != 2 {
		t.Errorf("LRem() deleted = %v, want 2", remResp.DeletedCount)
	}

	// Verify remaining values
	rangeResp, err := tc.GetClient().LRange(ctx, &pb.LRangeRequest{
		Key:   key,
		Start: 0,
		Stop:  -1,
	})
	if err != nil {
		t.Errorf("LRange() after LRem error = %v", err)
	}

	expected := []string{"b", "c", "a", "d"}
	if !reflect.DeepEqual(rangeResp.Values, expected) {
		t.Errorf("LRange() after LRem = %v, want %v", rangeResp.Values, expected)
	}
}

// TestListMultipleLists tests multiple lists distributed across nodes
func TestListMultipleLists(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()

	// Create multiple lists
	listKeys := []string{"list1", "list2", "list3", "list4", "list5"}
	nodeDistribution := make(map[int]int)

	for _, key := range listKeys {
		// Push values to each list
		for i := 1; i <= 3; i++ {
			value := "value" + string(rune('0'+i))
			_, err := tc.GetClient().RPush(ctx, &pb.RPushRequest{
				Key:   key,
				Value: value,
			})
			if err != nil {
				t.Fatalf("RPush(%v, %v) error = %v", key, value, err)
			}
		}

		nodeIdx := tc.GetNodeForKey(key)
		nodeDistribution[nodeIdx]++
	}

	// Log distribution
	t.Logf("List distribution across nodes:")
	for nodeIdx, count := range nodeDistribution {
		t.Logf("  Node %d: %d lists", nodeIdx, count)
	}

	// Verify all lists
	for _, key := range listKeys {
		lenResp, err := tc.GetClient().LLen(ctx, &pb.LLenRequest{Key: key})
		if err != nil {
			t.Errorf("LLen(%v) error = %v", key, err)
		}
		if lenResp.Length != 3 {
			t.Errorf("LLen(%v) = %v, want 3", key, lenResp.Length)
		}
	}
}
