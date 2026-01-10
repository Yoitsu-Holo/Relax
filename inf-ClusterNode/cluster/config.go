package cluster

import (
	"fmt"
	"os"
	"time"

	"gopkg.in/yaml.v3"
)

// NodeConfig represents the configuration for a single node
type NodeConfig struct {
	ID      string `yaml:"id"`
	Address string `yaml:"address"`
}

// Config represents the cluster configuration
type Config struct {
	Nodes              []NodeConfig  `yaml:"nodes"`
	HealthCheckInterval time.Duration `yaml:"health_check_interval"`
	GRPCPort           string        `yaml:"grpc_port"`
	HTTPPort           string        `yaml:"http_port"`
}

// LoadConfig loads the cluster configuration from a YAML file
func LoadConfig(filename string) (*Config, error) {
	data, err := os.ReadFile(filename)
	if err != nil {
		return nil, fmt.Errorf("failed to read config file: %w", err)
	}

	var config Config
	if err := yaml.Unmarshal(data, &config); err != nil {
		return nil, fmt.Errorf("failed to parse config file: %w", err)
	}

	// Set defaults
	if config.HealthCheckInterval == 0 {
		config.HealthCheckInterval = 10 * time.Second
	}
	if config.GRPCPort == "" {
		config.GRPCPort = ":50052"
	}
	if config.HTTPPort == "" {
		config.HTTPPort = ":8081"
	}

	// Validate
	if len(config.Nodes) == 0 {
		return nil, fmt.Errorf("at least one node is required")
	}

	for i, node := range config.Nodes {
		if node.ID == "" {
			return nil, fmt.Errorf("node %d: ID is required", i)
		}
		if node.Address == "" {
			return nil, fmt.Errorf("node %s: address is required", node.ID)
		}
	}

	return &config, nil
}

// DefaultConfig returns a default configuration for local development
func DefaultConfig() *Config {
	return &Config{
		Nodes: []NodeConfig{
			{ID: "node-1", Address: "localhost:50051"},
			{ID: "node-2", Address: "localhost:50053"},
			{ID: "node-3", Address: "localhost:50054"},
		},
		HealthCheckInterval: 10 * time.Second,
		GRPCPort:           ":50052",
		HTTPPort:           ":8081",
	}
}
