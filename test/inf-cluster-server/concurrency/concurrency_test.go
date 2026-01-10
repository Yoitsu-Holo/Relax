package concurrency_test

import (
	"fmt"
	"sync"
	"testing"

	testutil "github.com/yoitsuholo/relax/test/inf-cluster-server/testutil"
	pb "github.com/yoitsuholo/relax/proto"
)

// TestConcurrentKVOperations tests concurrent KV operations across cluster
func TestConcurrentKVOperations(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	const numGoroutines = 10
	const numOpsPerGoroutine = 100

	var wg sync.WaitGroup
	errors := make(chan error, numGoroutines*numOpsPerGoroutine)

	// Concurrent Set operations
	for g := 0; g < numGoroutines; g++ {
		wg.Add(1)
		go func(goroutineID int) {
			defer wg.Done()
			for i := 0; i < numOpsPerGoroutine; i++ {
				key := fmt.Sprintf("key-%d-%d", goroutineID, i)
				value := fmt.Sprintf("value-%d-%d", goroutineID, i)

				ctx := testutil.TestContext()
				_, err := tc.GetClient().Set(ctx, &pb.SetRequest{
					Key:   key,
					Value: value,
				})
				if err != nil {
					errors <- fmt.Errorf("goroutine %d, op %d: Set error: %v", goroutineID, i, err)
					return
				}
			}
		}(g)
	}

	wg.Wait()
	close(errors)

	// Check for errors
	errorCount := 0
	for err := range errors {
		t.Errorf("Concurrent operation error: %v", err)
		errorCount++
	}

	if errorCount > 0 {
		t.Fatalf("Total errors in concurrent operations: %d", errorCount)
	}

	// Verify all keys were set correctly
	for g := 0; g < numGoroutines; g++ {
		for i := 0; i < numOpsPerGoroutine; i++ {
			key := fmt.Sprintf("key-%d-%d", g, i)
			expectedValue := fmt.Sprintf("value-%d-%d", g, i)

			ctx := testutil.TestContext()
			getResp, err := tc.GetClient().Get(ctx, &pb.GetRequest{Key: key})
			if err != nil {
				t.Errorf("Verification Get(%v) error: %v", key, err)
				continue
			}
			if !getResp.Exists {
				t.Errorf("Verification Get(%v) exists = false", key)
				continue
			}
			if getResp.Value != expectedValue {
				t.Errorf("Verification Get(%v) = %v, want %v", key, getResp.Value, expectedValue)
			}
		}
	}

	t.Logf("Successfully completed %d concurrent operations across %d goroutines",
		numGoroutines*numOpsPerGoroutine, numGoroutines)
}

// TestConcurrentHashOperations tests concurrent Hash operations
func TestConcurrentHashOperations(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	const numGoroutines = 10
	const numKeysPerGoroutine = 20

	var wg sync.WaitGroup
	errors := make(chan error, numGoroutines*numKeysPerGoroutine)

	// Concurrent HSet operations
	for g := 0; g < numGoroutines; g++ {
		wg.Add(1)
		go func(goroutineID int) {
			defer wg.Done()
			for i := 0; i < numKeysPerGoroutine; i++ {
				key := fmt.Sprintf("hash-%d-%d", goroutineID, i)

				ctx := testutil.TestContext()
				_, err := tc.GetClient().HMSet(ctx, &pb.HMSetRequest{
					Key: key,
					Fields: map[string]string{
						"field1": "value1",
						"field2": "value2",
						"field3": "value3",
					},
				})
				if err != nil {
					errors <- fmt.Errorf("goroutine %d, key %d: HMSet error: %v", goroutineID, i, err)
					return
				}
			}
		}(g)
	}

	wg.Wait()
	close(errors)

	// Check for errors
	errorCount := 0
	for err := range errors {
		t.Errorf("Concurrent operation error: %v", err)
		errorCount++
	}

	if errorCount > 0 {
		t.Fatalf("Total errors in concurrent operations: %d", errorCount)
	}

	// Verify all hashes
	for g := 0; g < numGoroutines; g++ {
		for i := 0; i < numKeysPerGoroutine; i++ {
			key := fmt.Sprintf("hash-%d-%d", g, i)

			ctx := testutil.TestContext()
			lenResp, err := tc.GetClient().HLen(ctx, &pb.HLenRequest{Key: key})
			if err != nil {
				t.Errorf("Verification HLen(%v) error: %v", key, err)
				continue
			}
			if lenResp.Length != 3 {
				t.Errorf("Verification HLen(%v) = %v, want 3", key, lenResp.Length)
			}
		}
	}

	t.Logf("Successfully completed %d concurrent hash operations",
		numGoroutines*numKeysPerGoroutine)
}

// TestConcurrentMixedOperations tests concurrent operations of different types
func TestConcurrentMixedOperations(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	const numOps = 50

	var wg sync.WaitGroup
	errors := make(chan error, numOps*4)

	// KV operations
	wg.Add(1)
	go func() {
		defer wg.Done()
		for i := 0; i < numOps; i++ {
			key := fmt.Sprintf("kv-key-%d", i)
			ctx := testutil.TestContext()
			_, err := tc.GetClient().Set(ctx, &pb.SetRequest{
				Key:   key,
				Value: "value",
			})
			if err != nil {
				errors <- fmt.Errorf("KV op %d error: %v", i, err)
			}
		}
	}()

	// Hash operations
	wg.Add(1)
	go func() {
		defer wg.Done()
		for i := 0; i < numOps; i++ {
			key := fmt.Sprintf("hash-key-%d", i)
			ctx := testutil.TestContext()
			_, err := tc.GetClient().HSet(ctx, &pb.HSetRequest{
				Key:   key,
				Field: "field",
				Value: "value",
			})
			if err != nil {
				errors <- fmt.Errorf("Hash op %d error: %v", i, err)
			}
		}
	}()

	// Set operations
	wg.Add(1)
	go func() {
		defer wg.Done()
		for i := 0; i < numOps; i++ {
			key := fmt.Sprintf("set-key-%d", i)
			ctx := testutil.TestContext()
			_, err := tc.GetClient().SAdd(ctx, &pb.SAddRequest{
				Key:    key,
				Member: "member",
			})
			if err != nil {
				errors <- fmt.Errorf("Set op %d error: %v", i, err)
			}
		}
	}()

	// List operations
	wg.Add(1)
	go func() {
		defer wg.Done()
		for i := 0; i < numOps; i++ {
			key := fmt.Sprintf("list-key-%d", i)
			ctx := testutil.TestContext()
			_, err := tc.GetClient().LPush(ctx, &pb.LPushRequest{
				Key:   key,
				Value: "value",
			})
			if err != nil {
				errors <- fmt.Errorf("List op %d error: %v", i, err)
			}
		}
	}()

	wg.Wait()
	close(errors)

	// Check for errors
	errorCount := 0
	for err := range errors {
		t.Errorf("Concurrent operation error: %v", err)
		errorCount++
	}

	if errorCount > 0 {
		t.Fatalf("Total errors in concurrent mixed operations: %d", errorCount)
	}

	t.Logf("Successfully completed %d mixed concurrent operations", numOps*4)
}

// TestConcurrentSameKey tests concurrent operations on the same key
func TestConcurrentSameKey(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	const numGoroutines = 20
	const numOpsPerGoroutine = 50
	key := "shared-key"

	var wg sync.WaitGroup
	errors := make(chan error, numGoroutines*numOpsPerGoroutine)

	// Concurrent Set operations on the same key
	for g := 0; g < numGoroutines; g++ {
		wg.Add(1)
		go func(goroutineID int) {
			defer wg.Done()
			for i := 0; i < numOpsPerGoroutine; i++ {
				value := fmt.Sprintf("value-%d-%d", goroutineID, i)

				ctx := testutil.TestContext()
				_, err := tc.GetClient().Set(ctx, &pb.SetRequest{
					Key:   key,
					Value: value,
				})
				if err != nil {
					errors <- fmt.Errorf("goroutine %d, op %d: Set error: %v", goroutineID, i, err)
					return
				}

				// Also read the key
				_, err = tc.GetClient().Get(ctx, &pb.GetRequest{Key: key})
				if err != nil {
					errors <- fmt.Errorf("goroutine %d, op %d: Get error: %v", goroutineID, i, err)
					return
				}
			}
		}(g)
	}

	wg.Wait()
	close(errors)

	// Check for errors
	errorCount := 0
	for err := range errors {
		t.Errorf("Concurrent operation error: %v", err)
		errorCount++
	}

	if errorCount > 0 {
		t.Fatalf("Total errors in concurrent same-key operations: %d", errorCount)
	}

	// Verify the key still exists and has a valid value
	ctx := testutil.TestContext()
	getResp, err := tc.GetClient().Get(ctx, &pb.GetRequest{Key: key})
	if err != nil {
		t.Fatalf("Final Get error: %v", err)
	}
	if !getResp.Exists {
		t.Errorf("Final Get exists = false")
	}

	t.Logf("Successfully completed %d concurrent operations on the same key, final value: %v",
		numGoroutines*numOpsPerGoroutine*2, getResp.Value)
	t.Logf("Key %s was routed to node %d", key, tc.GetNodeForKey(key))
}

// TestConcurrentReadWrite tests concurrent reads and writes
func TestConcurrentReadWrite(t *testing.T) {
	tc := testutil.SetupTestCluster(t, 3)
	defer tc.Teardown()

	const numWriters = 10
	const numReaders = 20
	const numOpsPerGoroutine = 50
	keyPrefix := "rw-key"

	var wg sync.WaitGroup
	errors := make(chan error, (numWriters+numReaders)*numOpsPerGoroutine)

	// Pre-populate some keys
	ctx := testutil.TestContext()
	for i := 0; i < numOpsPerGoroutine; i++ {
		key := fmt.Sprintf("%s-%d", keyPrefix, i)
		_, err := tc.GetClient().Set(ctx, &pb.SetRequest{
			Key:   key,
			Value: fmt.Sprintf("initial-value-%d", i),
		})
		if err != nil {
			t.Fatalf("Pre-population error: %v", err)
		}
	}

	// Writers
	for w := 0; w < numWriters; w++ {
		wg.Add(1)
		go func(writerID int) {
			defer wg.Done()
			for i := 0; i < numOpsPerGoroutine; i++ {
				key := fmt.Sprintf("%s-%d", keyPrefix, i)
				value := fmt.Sprintf("writer-%d-value-%d", writerID, i)

				ctx := testutil.TestContext()
				_, err := tc.GetClient().Set(ctx, &pb.SetRequest{
					Key:   key,
					Value: value,
				})
				if err != nil {
					errors <- fmt.Errorf("writer %d, op %d: Set error: %v", writerID, i, err)
					return
				}
			}
		}(w)
	}

	// Readers
	for r := 0; r < numReaders; r++ {
		wg.Add(1)
		go func(readerID int) {
			defer wg.Done()
			for i := 0; i < numOpsPerGoroutine; i++ {
				key := fmt.Sprintf("%s-%d", keyPrefix, i%numOpsPerGoroutine)

				ctx := testutil.TestContext()
				getResp, err := tc.GetClient().Get(ctx, &pb.GetRequest{Key: key})
				if err != nil {
					errors <- fmt.Errorf("reader %d, op %d: Get error: %v", readerID, i, err)
					return
				}
				if !getResp.Exists {
					errors <- fmt.Errorf("reader %d, op %d: key %s not found", readerID, i, key)
					return
				}
			}
		}(r)
	}

	wg.Wait()
	close(errors)

	// Check for errors
	errorCount := 0
	for err := range errors {
		t.Errorf("Concurrent operation error: %v", err)
		errorCount++
	}

	if errorCount > 0 {
		t.Fatalf("Total errors in concurrent read/write operations: %d", errorCount)
	}

	t.Logf("Successfully completed %d writes and %d reads concurrently",
		numWriters*numOpsPerGoroutine, numReaders*numOpsPerGoroutine)
}
