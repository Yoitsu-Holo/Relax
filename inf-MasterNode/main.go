package main

import (
	"context"
	"flag"
	"fmt"
	"log"
	"net"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/grpc-ecosystem/grpc-gateway/v2/runtime"
	"github.com/yoitsuholo/relax/inf-MasterNode/cluster"
	"github.com/yoitsuholo/relax/inf-MasterNode/proxy"
	pb "github.com/yoitsuholo/relax/proto"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
	"google.golang.org/grpc/reflection"
)

var (
	configFile = flag.String("config", "config.yaml", "Path to configuration file")
)

func main() {
	flag.Parse()

	// Load configuration
	config, err := loadConfiguration(*configFile)
	if err != nil {
		log.Fatalf("Failed to load configuration: %v", err)
	}

	// Create cluster manager
	clusterMgr, err := cluster.NewManager(config.Nodes, config.HealthCheckInterval)
	if err != nil {
		log.Fatalf("Failed to create cluster manager: %v", err)
	}
	defer clusterMgr.Close()

	// Wait a bit for initial health checks
	log.Println("Waiting for initial health checks...")
	time.Sleep(2 * time.Second)

	healthyNodes := clusterMgr.GetHealthyNodeCount()
	log.Printf("Cluster initialized with %d/%d healthy nodes", healthyNodes, len(config.Nodes))

	// Create cache proxy
	cacheProxy := proxy.NewCacheProxy(clusterMgr)

	// Create gRPC server
	grpcServer := grpc.NewServer(
		grpc.MaxRecvMsgSize(10*1024*1024), // 10MB
		grpc.MaxSendMsgSize(10*1024*1024), // 10MB
	)
	pb.RegisterCacheServiceServer(grpcServer, cacheProxy)

	// Register reflection service for grpcurl and other tools
	reflection.Register(grpcServer)

	// Start gRPC server in a goroutine
	grpcListener, err := net.Listen("tcp", config.GRPCPort)
	if err != nil {
		log.Fatalf("Failed to listen on %s: %v", config.GRPCPort, err)
	}

	go func() {
		log.Printf("gRPC cluster proxy listening on %s", config.GRPCPort)
		if err := grpcServer.Serve(grpcListener); err != nil {
			log.Fatalf("Failed to serve gRPC: %v", err)
		}
	}()

	// Start HTTP gateway server in a goroutine
	httpServer := &http.Server{
		Addr:         config.HTTPPort,
		ReadTimeout:  30 * time.Second,
		WriteTimeout: 30 * time.Second,
		IdleTimeout:  120 * time.Second,
	}

	go func() {
		if err := startHTTPGateway(httpServer, config.GRPCPort); err != nil && err != http.ErrServerClosed {
			log.Fatalf("Failed to start HTTP gateway: %v", err)
		}
	}()

	// Print cluster information
	printClusterInfo(clusterMgr, config)

	// Wait for interrupt signal
	sigCh := make(chan os.Signal, 1)
	signal.Notify(sigCh, os.Interrupt, syscall.SIGTERM)
	<-sigCh

	log.Println("Shutting down cluster proxy...")

	// Graceful shutdown
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()

	if err := httpServer.Shutdown(ctx); err != nil {
		log.Printf("HTTP server shutdown error: %v", err)
	}

	grpcServer.GracefulStop()

	log.Println("Cluster proxy stopped")
}

func loadConfiguration(configFile string) (*cluster.Config, error) {
	// Try to load from file
	if _, err := os.Stat(configFile); err == nil {
		config, err := cluster.LoadConfig(configFile)
		if err != nil {
			return nil, fmt.Errorf("failed to load config from file: %w", err)
		}
		log.Printf("Loaded configuration from %s", configFile)
		return config, nil
	}

	// Use default config if file doesn't exist
	log.Printf("Config file %s not found, using default configuration", configFile)
	return cluster.DefaultConfig(), nil
}

func startHTTPGateway(server *http.Server, grpcAddr string) error {
	ctx := context.Background()
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	// Create a gRPC client connection to the local gRPC server
	conn, err := grpc.NewClient(
		"localhost"+grpcAddr,
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

	// Add CORS and health check endpoints
	handler := corsMiddleware(mux)
	handler = healthCheckMiddleware(handler)

	server.Handler = handler

	// Start HTTP server
	log.Printf("HTTP gateway listening on %s", server.Addr)
	log.Printf("HTTP endpoint: http://localhost%s", server.Addr)

	return server.ListenAndServe()
}

func printClusterInfo(mgr *cluster.Manager, config *cluster.Config) {
	log.Println("=" + string(make([]byte, 60)))
	log.Println("Cluster Proxy Information")
	log.Println("=" + string(make([]byte, 60)))
	log.Printf("gRPC Port: %s", config.GRPCPort)
	log.Printf("HTTP Port: %s", config.HTTPPort)
	log.Printf("Health Check Interval: %v", config.HealthCheckInterval)
	log.Println("")
	log.Println("Nodes:")

	nodes := mgr.GetAllNodes()
	for i, node := range nodes {
		status := "❌ UNHEALTHY"
		if node.IsHealthy() {
			status = "✅ HEALTHY"
		}
		log.Printf("  [%d] %s - %s - %s", i, node.ID, node.Address, status)
	}

	log.Println("")
	log.Printf("Total Nodes: %d", len(nodes))
	log.Printf("Healthy Nodes: %d", mgr.GetHealthyNodeCount())
	log.Println("=" + string(make([]byte, 60)))
	log.Println("Cluster proxy is ready to accept requests")
	log.Println("=" + string(make([]byte, 60)))
}

// corsMiddleware adds CORS headers
func corsMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type, Authorization")

		if r.Method == "OPTIONS" {
			w.WriteHeader(http.StatusOK)
			return
		}

		next.ServeHTTP(w, r)
	})
}

// healthCheckMiddleware adds a health check endpoint
func healthCheckMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path == "/health" {
			w.Header().Set("Content-Type", "application/json")
			w.WriteHeader(http.StatusOK)
			w.Write([]byte(`{"status":"ok","service":"cluster-proxy"}`))
			return
		}

		next.ServeHTTP(w, r)
	})
}
