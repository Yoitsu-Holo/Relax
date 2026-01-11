package main

import (
	"context"
	"fmt"
	"log"
	"time"

	pb "github.com/yoitsuholo/relax/proto"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
)

const (
	serverAddr = "localhost:50051"
)

func main() {
	// Connect to gRPC server
	conn, err := grpc.NewClient(
		serverAddr,
		grpc.WithTransportCredentials(insecure.NewCredentials()),
	)
	if err != nil {
		log.Fatalf("Failed to connect to server: %v", err)
	}
	defer conn.Close()

	client := pb.NewCacheServiceClient(conn)
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()

	fmt.Println("=== Cache Service Client Test ===\n")

	// Test KV operations
	testKVOperations(ctx, client)

	// Test Hash operations
	testHashOperations(ctx, client)

	// Test Set operations
	testSetOperations(ctx, client)

	// Test List operations
	testListOperations(ctx, client)

	fmt.Println("\n=== All tests completed successfully! ===")
}

func testKVOperations(ctx context.Context, client pb.CacheServiceClient) {
	fmt.Println("--- Testing KV Operations ---")

	// Set
	setResp, err := client.Set(ctx, &pb.KvSetRequest{
		Key:   "name",
		Value: "Alice",
	})
	if err != nil {
		log.Fatalf("Set failed: %v", err)
	}
	fmt.Printf("Set name=Alice: success=%v\n", setResp.Success)

	// Get
	getResp, err := client.Get(ctx, &pb.KvGetRequest{Key: "name"})
	if err != nil {
		log.Fatalf("Get failed: %v", err)
	}
	fmt.Printf("Get name: value=%s, exists=%v\n", getResp.Value, getResp.Exists)

	// Del
	delResp, err := client.Del(ctx, &pb.KvDelRequest{Key: "name"})
	if err != nil {
		log.Fatalf("Del failed: %v", err)
	}
	fmt.Printf("Del name: success=%v, deleted=%d\n", delResp.Success, delResp.DeletedCount)

	// Get non-existent key
	getResp2, err := client.Get(ctx, &pb.KvGetRequest{Key: "name"})
	if err != nil {
		log.Fatalf("Get failed: %v", err)
	}
	fmt.Printf("Get deleted key: value=%s, exists=%v\n\n", getResp2.Value, getResp2.Exists)
}

func testHashOperations(ctx context.Context, client pb.CacheServiceClient) {
	fmt.Println("--- Testing Hash Operations ---")

	// HMSet (set single field)
	hmsetResp, err := client.HMSet(ctx, &pb.HashSetMRequest{
		Key:   "user:1",
		Field: "name",
		Value: "Bob",
	})
	if err != nil {
		log.Fatalf("HMSet failed: %v", err)
	}
	fmt.Printf("HMSet user:1 name=Bob: success=%v, created=%v\n", hmsetResp.Success, hmsetResp.Created)

	hmsetResp2, err := client.HMSet(ctx, &pb.HashSetMRequest{
		Key:   "user:1",
		Field: "age",
		Value: "30",
	})
	if err != nil {
		log.Fatalf("HMSet failed: %v", err)
	}
	fmt.Printf("HMSet user:1 age=30: success=%v, created=%v\n", hmsetResp2.Success, hmsetResp2.Created)

	// HMGet (get single field)
	hmgetResp, err := client.HMGet(ctx, &pb.HashGetMRequest{
		Key:   "user:1",
		Field: "name",
	})
	if err != nil {
		log.Fatalf("HMGet failed: %v", err)
	}
	fmt.Printf("HMGet user:1 name: value=%s, exists=%v\n", hmgetResp.Value, hmgetResp.Exists)

	// HDel (delete entire hash)
	hdelResp, err := client.HDel(ctx, &pb.HashDelRequest{
		Key: "user:1",
	})
	if err != nil {
		log.Fatalf("HDel failed: %v", err)
	}
	fmt.Printf("HDel user:1: success=%v, deleted=%d\n\n", hdelResp.Success, hdelResp.DeletedCount)
}

func testSetOperations(ctx context.Context, client pb.CacheServiceClient) {
	fmt.Println("--- Testing Set Operations ---")

	// SAdd
	saddResp1, err := client.SAdd(ctx, &pb.SetAddMRequest{
		Key:    "tags",
		Member: "golang",
	})
	if err != nil {
		log.Fatalf("SAdd failed: %v", err)
	}
	fmt.Printf("SAdd tags golang: success=%v, added=%v\n", saddResp1.Success, saddResp1.Added)

	saddResp2, err := client.SAdd(ctx, &pb.SetAddMRequest{
		Key:    "tags",
		Member: "grpc",
	})
	if err != nil {
		log.Fatalf("SAdd failed: %v", err)
	}
	fmt.Printf("SAdd tags grpc: success=%v, added=%v\n", saddResp2.Success, saddResp2.Added)

	saddResp3, err := client.SAdd(ctx, &pb.SetAddMRequest{
		Key:    "tags",
		Member: "cache",
	})
	if err != nil {
		log.Fatalf("SAdd failed: %v", err)
	}
	fmt.Printf("SAdd tags cache: success=%v, added=%v\n", saddResp3.Success, saddResp3.Added)

	// SMembers
	smembersResp, err := client.SMembers(ctx, &pb.SetGetRequest{Key: "tags"})
	if err != nil {
		log.Fatalf("SMembers failed: %v", err)
	}
	fmt.Printf("SMembers tags: %v\n", smembersResp.Members)

	// SRem
	sremResp, err := client.SRem(ctx, &pb.SetDelMRequest{
		Key:    "tags",
		Member: "golang",
	})
	if err != nil {
		log.Fatalf("SRem failed: %v", err)
	}
	fmt.Printf("SRem tags golang: success=%v, deleted=%d\n", sremResp.Success, sremResp.DeletedCount)

	// SMembers again
	smembersResp2, err := client.SMembers(ctx, &pb.SetGetRequest{Key: "tags"})
	if err != nil {
		log.Fatalf("SMembers failed: %v", err)
	}
	fmt.Printf("SMembers tags after removal: %v\n\n", smembersResp2.Members)
}

func testListOperations(ctx context.Context, client pb.CacheServiceClient) {
	fmt.Println("--- Testing List Operations ---")

	// LPush
	lpushResp1, err := client.LPush(ctx, &pb.LPushRequest{
		Key:   "queue",
		Value: "first",
	})
	if err != nil {
		log.Fatalf("LPush failed: %v", err)
	}
	fmt.Printf("LPush queue first: success=%v, length=%d\n", lpushResp1.Success, lpushResp1.Length)

	lpushResp2, err := client.LPush(ctx, &pb.LPushRequest{
		Key:   "queue",
		Value: "second",
	})
	if err != nil {
		log.Fatalf("LPush failed: %v", err)
	}
	fmt.Printf("LPush queue second: success=%v, length=%d\n", lpushResp2.Success, lpushResp2.Length)

	lpushResp3, err := client.LPush(ctx, &pb.LPushRequest{
		Key:   "queue",
		Value: "third",
	})
	if err != nil {
		log.Fatalf("LPush failed: %v", err)
	}
	fmt.Printf("LPush queue third: success=%v, length=%d\n", lpushResp3.Success, lpushResp3.Length)

	// LRange
	lrangeResp, err := client.LRange(ctx, &pb.LRangeRequest{
		Key:   "queue",
		Start: 0,
		Stop:  -1,
	})
	if err != nil {
		log.Fatalf("LRange failed: %v", err)
	}
	fmt.Printf("LRange queue 0 -1: %v\n", lrangeResp.Values)

	// LRem
	lremResp, err := client.LRem(ctx, &pb.LRemRequest{
		Key:   "queue",
		Count: 1,
		Value: "second",
	})
	if err != nil {
		log.Fatalf("LRem failed: %v", err)
	}
	fmt.Printf("LRem queue 1 second: success=%v, deleted=%d\n", lremResp.Success, lremResp.DeletedCount)

	// LRange again
	lrangeResp2, err := client.LRange(ctx, &pb.LRangeRequest{
		Key:   "queue",
		Start: 0,
		Stop:  -1,
	})
	if err != nil {
		log.Fatalf("LRange failed: %v", err)
	}
	fmt.Printf("LRange queue after removal: %v\n", lrangeResp2.Values)
}
