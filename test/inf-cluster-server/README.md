# Cluster Server Integration Tests

This directory contains integration tests for the cluster mode of the Redis-like cache system.

## Test Structure

The tests are organized by operation type:

- **kv/**: Key-Value operations (Set, Get, Del, Exists)
- **hash/**: Hash operations (HSet, HGet, HDel, HMSet, HMGet, HLen, HGetAll, HExists)
- **set/**: Set operations (SAdd, SMembers, SRem, SIsMember, SCard)
- **list/**: List operations (LPush, RPush, LPop, RPop, LRange, LRem, LLen, LIndex, LSet)
- **concurrency/**: Concurrent operations testing

## Test Infrastructure

The `helper_test.go` file provides the `TestCluster` infrastructure that:

1. Starts multiple single-node cache servers
2. Creates a cluster manager to manage these nodes
3. Starts a cluster proxy server that routes requests to nodes based on key hashing
4. Provides a gRPC client to interact with the cluster

### Key Routing

The cluster uses xxhash128 to compute a 128-bit hash for each key:
- **High 64 bits**: Used to determine which node to route the request to (modulo number of nodes)
- **Low 64 bits**: Used within each single node to determine the storage shard (must be power of 2, currently 32)

## Running Tests

### Run all tests:
```bash
cd /home/yoitsuholo/Code/Relax/test/inf-cluster-server
go test -v ./...
```

### Run specific test suite:
```bash
go test -v ./kv
go test -v ./hash
go test -v ./set
go test -v ./list
go test -v ./concurrency
```

### Run a specific test:
```bash
go test -v ./kv -run TestKVBasicSetGet
```

## Test Features

1. **Distribution Testing**: Tests verify that keys are properly distributed across multiple nodes
2. **Concurrent Operations**: Tests verify that concurrent operations are handled correctly
3. **Multiple Cluster Sizes**: Tests run with different numbers of nodes (1, 3, 5) to verify scalability
4. **All Operations**: Tests cover all operations defined in the proto interface

## Notes

- Tests use dynamic port allocation to avoid conflicts
- Each test creates a fresh cluster and tears it down after completion
- The cluster manager performs health checks on nodes
- Tests verify both correctness and distribution of keys across nodes
