package set_test

import (
	"sort"
	"testing"

	testutil "github.com/yoitsuholo/relax/test/inf-cluster-server/testutil"
	pb "github.com/yoitsuholo/relax/proto"
)

// TestSetBasicSAddSMembers tests basic SAdd and SMembers operations through cluster
func TestSetBasicSAddSMembers(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()
	key := "set-key"

	// Add members
	members := []string{"member1", "member2", "member3"}
	for _, member := range members {
		addResp, err := tc.GetClient().SAdd(ctx, &pb.SAddRequest{
			Key:    key,
			Member: member,
		})
		if err != nil {
			t.Errorf("SAdd(%v) error = %v", member, err)
			return
		}
		if !addResp.Added {
			t.Errorf("SAdd(%v) added = false, want true", member)
		}
	}

	// Get members
	membersResp, err := tc.GetClient().SMembers(ctx, &pb.SMembersRequest{
		Key: key,
	})
	if err != nil {
		t.Errorf("SMembers() error = %v", err)
		return
	}

	// Sort for comparison
	sort.Strings(membersResp.Members)
	sort.Strings(members)

	if !stringSlicesEqual(membersResp.Members, members) {
		t.Errorf("SMembers() = %v, want %v", membersResp.Members, members)
	}

	t.Logf("Set key %s routed to node %d", key, tc.GetNodeForKey(key))
}

// TestSetSAddDuplicate tests adding duplicate members
func TestSetSAddDuplicate(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()
	key := "set-key"
	member := "member1"

	// Add member first time
	addResp, err := tc.GetClient().SAdd(ctx, &pb.SAddRequest{
		Key:    key,
		Member: member,
	})
	if err != nil || !addResp.Added {
		t.Fatalf("SAdd() first time error = %v, added = %v", err, addResp.Added)
	}

	// Add same member again
	addResp, err = tc.GetClient().SAdd(ctx, &pb.SAddRequest{
		Key:    key,
		Member: member,
	})
	if err != nil {
		t.Errorf("SAdd() duplicate error = %v", err)
	}
	if addResp.Added {
		t.Errorf("SAdd() duplicate added = true, want false")
	}

	// Verify only one member exists
	membersResp, err := tc.GetClient().SMembers(ctx, &pb.SMembersRequest{Key: key})
	if err != nil {
		t.Errorf("SMembers() error = %v", err)
	}
	if len(membersResp.Members) != 1 {
		t.Errorf("SMembers() length = %v, want 1", len(membersResp.Members))
	}
}

// TestSetSRem tests SRem operation
func TestSetSRem(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()
	key := "set-key"

	// Add members
	members := []string{"member1", "member2", "member3"}
	for _, member := range members {
		_, err := tc.GetClient().SAdd(ctx, &pb.SAddRequest{
			Key:    key,
			Member: member,
		})
		if err != nil {
			t.Fatalf("SAdd(%v) error = %v", member, err)
		}
	}

	// Remove a member
	remResp, err := tc.GetClient().SRem(ctx, &pb.SRemRequest{
		Key:    key,
		Member: "member2",
	})
	if err != nil {
		t.Errorf("SRem() error = %v", err)
	}
	if remResp.DeletedCount != 1 {
		t.Errorf("SRem() deleted = %v, want 1", remResp.DeletedCount)
	}

	// Verify member is removed
	membersResp, err := tc.GetClient().SMembers(ctx, &pb.SMembersRequest{Key: key})
	if err != nil {
		t.Errorf("SMembers() after SRem error = %v", err)
	}
	if len(membersResp.Members) != 2 {
		t.Errorf("SMembers() after SRem length = %v, want 2", len(membersResp.Members))
	}

	// Verify correct members remain
	sort.Strings(membersResp.Members)
	expected := []string{"member1", "member3"}
	if !stringSlicesEqual(membersResp.Members, expected) {
		t.Errorf("SMembers() after SRem = %v, want %v", membersResp.Members, expected)
	}

	// Remove non-existent member
	remResp, err = tc.GetClient().SRem(ctx, &pb.SRemRequest{
		Key:    key,
		Member: "non-existent",
	})
	if err != nil {
		t.Errorf("SRem() non-existent error = %v", err)
	}
	if remResp.DeletedCount != 0 {
		t.Errorf("SRem() non-existent deleted = %v, want 0", remResp.DeletedCount)
	}
}

// TestSetSIsMember tests SIsMember operation
func TestSetSIsMember(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()
	key := "set-key"

	// Check non-existent set
	isMemberResp, err := tc.GetClient().SIsMember(ctx, &pb.SIsMemberRequest{
		Key:    key,
		Member: "member",
	})
	if err != nil {
		t.Errorf("SIsMember() non-existent set error = %v", err)
	}
	if isMemberResp.IsMember {
		t.Errorf("SIsMember() non-existent set = true, want false")
	}

	// Add a member
	_, err = tc.GetClient().SAdd(ctx, &pb.SAddRequest{
		Key:    key,
		Member: "member1",
	})
	if err != nil {
		t.Fatalf("SAdd() error = %v", err)
	}

	// Check existing member
	isMemberResp, err = tc.GetClient().SIsMember(ctx, &pb.SIsMemberRequest{
		Key:    key,
		Member: "member1",
	})
	if err != nil {
		t.Errorf("SIsMember() existing member error = %v", err)
	}
	if !isMemberResp.IsMember {
		t.Errorf("SIsMember() existing member = false, want true")
	}

	// Check non-existent member
	isMemberResp, err = tc.GetClient().SIsMember(ctx, &pb.SIsMemberRequest{
		Key:    key,
		Member: "non-existent",
	})
	if err != nil {
		t.Errorf("SIsMember() non-existent member error = %v", err)
	}
	if isMemberResp.IsMember {
		t.Errorf("SIsMember() non-existent member = true, want false")
	}
}

// TestSetSCard tests SCard operation
func TestSetSCard(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()
	key := "set-key"

	// Check cardinality of non-existent set
	cardResp, err := tc.GetClient().SCard(ctx, &pb.SCardRequest{Key: key})
	if err != nil {
		t.Errorf("SCard() non-existent set error = %v", err)
	}
	if cardResp.Cardinality != 0 {
		t.Errorf("SCard() non-existent set = %v, want 0", cardResp.Cardinality)
	}

	// Add members one by one
	for i := 1; i <= 5; i++ {
		member := "member" + string(rune('0'+i))
		_, err := tc.GetClient().SAdd(ctx, &pb.SAddRequest{
			Key:    key,
			Member: member,
		})
		if err != nil {
			t.Fatalf("SAdd() iteration %v error = %v", i, err)
		}

		cardResp, err := tc.GetClient().SCard(ctx, &pb.SCardRequest{Key: key})
		if err != nil {
			t.Errorf("SCard() iteration %v error = %v", i, err)
		}
		if cardResp.Cardinality != int32(i) {
			t.Errorf("SCard() iteration %v = %v, want %v", i, cardResp.Cardinality, i)
		}
	}
}

// TestSetMultipleSets tests multiple sets distributed across nodes
func TestSetMultipleSets(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	ctx := testutil.TestContext()

	// Create multiple sets
	setKeys := []string{"set1", "set2", "set3", "set4", "set5"}
	nodeDistribution := make(map[int]int)

	for _, key := range setKeys {
		// Add members to each set
		for i := 1; i <= 3; i++ {
			member := "member" + string(rune('0'+i))
			_, err := tc.GetClient().SAdd(ctx, &pb.SAddRequest{
				Key:    key,
				Member: member,
			})
			if err != nil {
				t.Fatalf("SAdd(%v, %v) error = %v", key, member, err)
			}
		}

		nodeIdx := tc.GetNodeForKey(key)
		nodeDistribution[nodeIdx]++
	}

	// Log distribution
	t.Logf("Set distribution across nodes:")
	for nodeIdx, count := range nodeDistribution {
		t.Logf("  Node %d: %d sets", nodeIdx, count)
	}

	// Verify all sets
	for _, key := range setKeys {
		cardResp, err := tc.GetClient().SCard(ctx, &pb.SCardRequest{Key: key})
		if err != nil {
			t.Errorf("SCard(%v) error = %v", key, err)
		}
		if cardResp.Cardinality != 3 {
			t.Errorf("SCard(%v) = %v, want 3", key, cardResp.Cardinality)
		}
	}
}

// Helper function to compare string slices (order independent)
func stringSlicesEqual(a, b []string) bool {
	if len(a) != len(b) {
		return false
	}
	aCopy := make([]string, len(a))
	bCopy := make([]string, len(b))
	copy(aCopy, a)
	copy(bCopy, b)
	sort.Strings(aCopy)
	sort.Strings(bCopy)
	for i := range aCopy {
		if aCopy[i] != bCopy[i] {
			return false
		}
	}
	return true
}
