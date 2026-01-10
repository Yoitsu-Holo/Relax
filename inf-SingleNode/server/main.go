package main

import (
	"context"
	"fmt"
	"log"
	"net"
	"net/http"

	"github.com/grpc-ecosystem/grpc-gateway/v2/runtime"
	"github.com/yoitsuholo/relax/inf-SingleNode/server/service"
	pb "github.com/yoitsuholo/relax/proto"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
	"google.golang.org/grpc/reflection"
)

const (
	grpcPort = ":50051"
	httpPort = ":8080"
)

func main() {
	// Create gRPC server
	grpcServer := grpc.NewServer()
	cacheServer := service.NewCacheServer()
	pb.RegisterCacheServiceServer(grpcServer, cacheServer)

	// Register reflection service for grpcurl and other tools
	reflection.Register(grpcServer)

	// Start gRPC server in a goroutine
	go func() {
		lis, err := net.Listen("tcp", grpcPort)
		if err != nil {
			log.Fatalf("Failed to listen on %s: %v", grpcPort, err)
		}
		log.Printf("gRPC server listening on %s", grpcPort)
		if err := grpcServer.Serve(lis); err != nil {
			log.Fatalf("Failed to serve gRPC: %v", err)
		}
	}()

	// Start HTTP gateway server
	if err := startHTTPGateway(); err != nil {
		log.Fatalf("Failed to start HTTP gateway: %v", err)
	}
}

func startHTTPGateway() error {
	ctx := context.Background()
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	// Create a gRPC client connection to the gRPC server
	conn, err := grpc.NewClient(
		"localhost"+grpcPort,
		grpc.WithTransportCredentials(insecure.NewCredentials()),
	)
	if err != nil {
		return fmt.Errorf("failed to dial gRPC server: %w", err)
	}
	defer conn.Close()

	// Create a new gRPC gateway mux
	mux := runtime.NewServeMux()

	// Register the cache service handler
	if err := pb.RegisterCacheServiceHandler(ctx, mux, conn); err != nil {
		return fmt.Errorf("failed to register gateway: %w", err)
	}

	// Start HTTP server
	log.Printf("HTTP gateway listening on %s", httpPort)
	log.Printf("gRPC endpoint: localhost%s", grpcPort)
	log.Printf("HTTP endpoint: http://localhost%s", httpPort)

	return http.ListenAndServe(httpPort, mux)
}
