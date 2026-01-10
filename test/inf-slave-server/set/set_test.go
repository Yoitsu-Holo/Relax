package set_test

import (
	"sort"
	"testing"

	"github.com/yoitsuholo/relax/inf-SingleNode/server/cache/simpleCache"
)

// TestSetBasicSAdd tests basic SAdd operation
func TestSetBasicSAdd(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "set-key"

	// Add new member
	isNew, err := cache.SAdd(key, "member1")
	if err != nil {
		t.Errorf("SAdd() error = %v", err)
		return
	}
	if !isNew {
		t.Errorf("SAdd() new member isNew = false, want true")
	}

	// Add same member again
	isNew, err = cache.SAdd(key, "member1")
	if err != nil {
		t.Errorf("SAdd() duplicate error = %v", err)
		return
	}
	if isNew {
		t.Errorf("SAdd() duplicate member isNew = true, want false")
	}
}

// TestSetSMembers tests SMembers operation
func TestSetSMembers(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "set-key"

	// Get members from non-existent key
	members, err := cache.SMembers(key)
	if err != nil {
		t.Errorf("SMembers() non-existent key error = %v", err)
	}
	if len(members) != 0 {
		t.Errorf("SMembers() non-existent key length = %v, want 0", len(members))
	}

	// Add members
	expectedMembers := []string{"member1", "member2", "member3"}
	for _, member := range expectedMembers {
		_, err := cache.SAdd(key, member)
		if err != nil {
			t.Fatalf("SAdd(%v) error = %v", member, err)
		}
	}

	// Get all members
	members, err = cache.SMembers(key)
	if err != nil {
		t.Errorf("SMembers() error = %v", err)
	}

	if len(members) != len(expectedMembers) {
		t.Errorf("SMembers() length = %v, want %v", len(members), len(expectedMembers))
	}

	// Verify all members are present (order independent)
	if !stringSlicesEqual(members, expectedMembers) {
		t.Errorf("SMembers() = %v, want %v", members, expectedMembers)
	}
}

// TestSetSRem tests SRem operation
func TestSetSRem(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "set-key"

	// Remove from non-existent key
	removed, err := cache.SRem(key, "member")
	if err != nil {
		t.Errorf("SRem() non-existent key error = %v", err)
	}
	if removed != 0 {
		t.Errorf("SRem() non-existent key removed = %v, want 0", removed)
	}

	// Add members
	_, err = cache.SAdd(key, "member1")
	if err != nil {
		t.Fatalf("SAdd() member1 error = %v", err)
	}
	_, err = cache.SAdd(key, "member2")
	if err != nil {
		t.Fatalf("SAdd() member2 error = %v", err)
	}

	// Remove existing member
	removed, err = cache.SRem(key, "member1")
	if err != nil {
		t.Errorf("SRem() existing member error = %v", err)
	}
	if removed != 1 {
		t.Errorf("SRem() existing member removed = %v, want 1", removed)
	}

	// Verify member is removed
	isMember, err := cache.SIsMember(key, "member1")
	if err != nil {
		t.Errorf("SIsMember() after SRem error = %v", err)
	}
	if isMember {
		t.Errorf("SIsMember() after SRem = true, want false")
	}

	// Remove non-existent member
	removed, err = cache.SRem(key, "non-existent")
	if err != nil {
		t.Errorf("SRem() non-existent member error = %v", err)
	}
	if removed != 0 {
		t.Errorf("SRem() non-existent member removed = %v, want 0", removed)
	}
}

// TestSetSIsMember tests SIsMember operation
func TestSetSIsMember(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "set-key"

	// Check non-existent key
	isMember, err := cache.SIsMember(key, "member")
	if err != nil {
		t.Errorf("SIsMember() non-existent key error = %v", err)
	}
	if isMember {
		t.Errorf("SIsMember() non-existent key = true, want false")
	}

	// Add a member
	_, err = cache.SAdd(key, "member1")
	if err != nil {
		t.Fatalf("SAdd() error = %v", err)
	}

	// Check existing member
	isMember, err = cache.SIsMember(key, "member1")
	if err != nil {
		t.Errorf("SIsMember() existing member error = %v", err)
	}
	if !isMember {
		t.Errorf("SIsMember() existing member = false, want true")
	}

	// Check non-existent member
	isMember, err = cache.SIsMember(key, "non-existent")
	if err != nil {
		t.Errorf("SIsMember() non-existent member error = %v", err)
	}
	if isMember {
		t.Errorf("SIsMember() non-existent member = true, want false")
	}
}

// TestSetSCard tests SCard operation
func TestSetSCard(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "set-key"

	// Check cardinality of non-existent key
	card, err := cache.SCard(key)
	if err != nil {
		t.Errorf("SCard() non-existent key error = %v", err)
	}
	if card != 0 {
		t.Errorf("SCard() non-existent key = %v, want 0", card)
	}

	// Add members one by one
	for i := 1; i <= 5; i++ {
		member := "member" + string(rune('0'+i))
		_, err := cache.SAdd(key, member)
		if err != nil {
			t.Fatalf("SAdd() iteration %v error = %v", i, err)
		}

		card, err := cache.SCard(key)
		if err != nil {
			t.Errorf("SCard() iteration %v error = %v", i, err)
		}
		if card != i {
			t.Errorf("SCard() iteration %v = %v, want %v", i, card, i)
		}
	}

	// Add duplicate member (should not increase cardinality)
	_, err = cache.SAdd(key, "member1")
	if err != nil {
		t.Fatalf("SAdd() duplicate error = %v", err)
	}

	card, err = cache.SCard(key)
	if err != nil {
		t.Errorf("SCard() after duplicate error = %v", err)
	}
	if card != 5 {
		t.Errorf("SCard() after duplicate = %v, want 5", card)
	}
}

// TestSetWrongType tests type error handling
func TestSetWrongType(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "type-key"

	// Create a string at this key
	err := cache.Set(key, "string-value")
	if err != nil {
		t.Fatalf("Set() error = %v", err)
	}

	// Try set operations
	_, err = cache.SAdd(key, "member")
	if err == nil {
		t.Error("SAdd() on string key should return error, got nil")
	}

	_, err = cache.SMembers(key)
	if err == nil {
		t.Error("SMembers() on string key should return error, got nil")
	}

	_, err = cache.SRem(key, "member")
	if err == nil {
		t.Error("SRem() on string key should return error, got nil")
	}

	_, err = cache.SIsMember(key, "member")
	if err == nil {
		t.Error("SIsMember() on string key should return error, got nil")
	}

	_, err = cache.SCard(key)
	if err == nil {
		t.Error("SCard() on string key should return error, got nil")
	}
}

// TestSetEmptyCleanup tests that empty sets are cleaned up
func TestSetEmptyCleanup(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "set-key"

	// Add and remove a single member
	_, err := cache.SAdd(key, "member")
	if err != nil {
		t.Fatalf("SAdd() error = %v", err)
	}

	removed, err := cache.SRem(key, "member")
	if err != nil {
		t.Fatalf("SRem() error = %v", err)
	}
	if removed != 1 {
		t.Errorf("SRem() removed = %v, want 1", removed)
	}

	// Verify set is cleaned up (key doesn't exist)
	exists, err := cache.Exists(key)
	if err != nil {
		t.Errorf("Exists() after cleanup error = %v", err)
	}
	if exists {
		t.Errorf("Exists() after cleanup = true, want false (set should be cleaned up)")
	}
}

// TestSetLargeSet tests operations with many members
func TestSetLargeSet(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "large-set"
	numMembers := 1000

	// Add many members
	for i := 0; i < numMembers; i++ {
		member := "member-" + string(rune('0'+i%10)) + string(rune('0'+i/10%10)) + string(rune('0'+i/100))
		_, err := cache.SAdd(key, member)
		if err != nil {
			t.Fatalf("SAdd() iteration %v error = %v", i, err)
		}
	}

	// Verify cardinality
	card, err := cache.SCard(key)
	if err != nil {
		t.Errorf("SCard() error = %v", err)
	}
	if card != numMembers {
		t.Errorf("SCard() = %v, want %v", card, numMembers)
	}

	// Get all members
	members, err := cache.SMembers(key)
	if err != nil {
		t.Errorf("SMembers() error = %v", err)
	}
	if len(members) != numMembers {
		t.Errorf("SMembers() length = %v, want %v", len(members), numMembers)
	}

	// Verify a sample of members
	for i := 0; i < 10; i++ {
		member := "member-" + string(rune('0'+i%10)) + string(rune('0'+i/10%10)) + string(rune('0'+i/100))
		isMember, err := cache.SIsMember(key, member)
		if err != nil {
			t.Errorf("SIsMember(%v) error = %v", member, err)
		}
		if !isMember {
			t.Errorf("SIsMember(%v) = false, want true", member)
		}
	}
}

// TestSetMultipleSets tests multiple independent sets
func TestSetMultipleSets(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	keys := []string{"set1", "set2", "set3"}

	// Add different members to each set
	for i, key := range keys {
		for j := 0; j < 3; j++ {
			member := "member-" + string(rune('0'+i)) + "-" + string(rune('0'+j))
			_, err := cache.SAdd(key, member)
			if err != nil {
				t.Fatalf("SAdd(%v, %v) error = %v", key, member, err)
			}
		}
	}

	// Verify each set has correct members
	for i, key := range keys {
		members, err := cache.SMembers(key)
		if err != nil {
			t.Errorf("SMembers(%v) error = %v", key, err)
		}
		if len(members) != 3 {
			t.Errorf("SMembers(%v) length = %v, want 3", key, len(members))
		}

		// Verify correct members
		for j := 0; j < 3; j++ {
			expectedMember := "member-" + string(rune('0'+i)) + "-" + string(rune('0'+j))
			isMember, err := cache.SIsMember(key, expectedMember)
			if err != nil {
				t.Errorf("SIsMember(%v, %v) error = %v", key, expectedMember, err)
			}
			if !isMember {
				t.Errorf("SIsMember(%v, %v) = false, want true", key, expectedMember)
			}
		}

		// Verify members from other sets don't exist
		for j := 0; j < len(keys); j++ {
			if j != i {
				wrongMember := "member-" + string(rune('0'+j)) + "-0"
				isMember, err := cache.SIsMember(key, wrongMember)
				if err != nil {
					t.Errorf("SIsMember(%v, %v) error = %v", key, wrongMember, err)
				}
				if isMember {
					t.Errorf("SIsMember(%v, %v) = true, want false", key, wrongMember)
				}
			}
		}
	}
}

// TestSetDuplicateAdd tests adding duplicate members
func TestSetDuplicateAdd(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "set-key"
	member := "member1"

	// Add member multiple times
	for i := 0; i < 5; i++ {
		isNew, err := cache.SAdd(key, member)
		if err != nil {
			t.Fatalf("SAdd() iteration %v error = %v", i, err)
		}
		if i == 0 && !isNew {
			t.Errorf("SAdd() first iteration isNew = false, want true")
		}
		if i > 0 && isNew {
			t.Errorf("SAdd() iteration %v isNew = true, want false", i)
		}
	}

	// Verify cardinality is still 1
	card, err := cache.SCard(key)
	if err != nil {
		t.Errorf("SCard() error = %v", err)
	}
	if card != 1 {
		t.Errorf("SCard() = %v, want 1", card)
	}
}

// TestSetUnicodeMembers tests set with unicode members
func TestSetUnicodeMembers(t *testing.T) {
	cache := simpleCache.NewShardedCache(32)
	defer cache.Close()

	key := "unicode-set"
	unicodeMembers := []string{
		"你好",
		"世界",
		"🌍",
		"こんにちは",
		"مرحبا",
	}

	// Add unicode members
	for _, member := range unicodeMembers {
		_, err := cache.SAdd(key, member)
		if err != nil {
			t.Fatalf("SAdd(%v) error = %v", member, err)
		}
	}

	// Verify all members exist
	for _, member := range unicodeMembers {
		isMember, err := cache.SIsMember(key, member)
		if err != nil {
			t.Errorf("SIsMember(%v) error = %v", member, err)
		}
		if !isMember {
			t.Errorf("SIsMember(%v) = false, want true", member)
		}
	}

	// Get all members
	members, err := cache.SMembers(key)
	if err != nil {
		t.Errorf("SMembers() error = %v", err)
	}
	if len(members) != len(unicodeMembers) {
		t.Errorf("SMembers() length = %v, want %v", len(members), len(unicodeMembers))
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
