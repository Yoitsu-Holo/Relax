package cluster

import (
	"context"
	"fmt"
	"log"
	"sync"
	"time"

	"github.com/zeebo/xxh3"
)

// Manager manages the cluster of cache nodes
type Manager struct {
	nodes           []*Node
	nodeMap         map[string]*Node
	mu              sync.RWMutex
	healthCheckTick time.Duration
	stopCh          chan struct{}
}

// NewManager creates a new cluster manager
func NewManager(nodeConfigs []NodeConfig, healthCheckInterval time.Duration) (*Manager, error) {
	if len(nodeConfigs) == 0 {
		return nil, fmt.Errorf("at least one node is required")
	}

	m := &Manager{
		nodes:           make([]*Node, 0, len(nodeConfigs)),
		nodeMap:         make(map[string]*Node),
		healthCheckTick: healthCheckInterval,
		stopCh:          make(chan struct{}),
	}

	// Initialize all nodes
	for _, cfg := range nodeConfigs {
		node, err := NewNode(cfg.ID, cfg.Address)
		if err != nil {
			log.Printf("Warning: Failed to connect to node %s at %s: %v", cfg.ID, cfg.Address, err)
			// Continue with other nodes even if one fails
			node = &Node{
				ID:      cfg.ID,
				Address: cfg.Address,
				healthy: false,
			}
		}

		m.nodes = append(m.nodes, node)
		m.nodeMap[cfg.ID] = node
	}

	// Start health check goroutine
	go m.healthCheckLoop()

	log.Printf("Cluster manager initialized with %d nodes", len(m.nodes))
	return m, nil
}

// GetNodeForKey returns the node responsible for the given key
// Uses xxhash128 and takes the high 64 bits for routing
func (m *Manager) GetNodeForKey(key string) (*Node, error) {
	m.mu.RLock()
	defer m.mu.RUnlock()

	if len(m.nodes) == 0 {
		return nil, fmt.Errorf("no nodes available")
	}

	// Compute xxhash128 of the key using xxh3
	hash128 := xxh3.HashString128(key)

	// Extract high 64 bits from the 128-bit hash
	highBits := hash128.Hi

	// Use modulo to select node
	nodeIndex := int(highBits % uint64(len(m.nodes)))
	node := m.nodes[nodeIndex]

	// If the selected node is unhealthy, try to find a healthy one
	if !node.IsHealthy() {
		for i := 0; i < len(m.nodes); i++ {
			altNode := m.nodes[(nodeIndex+i)%len(m.nodes)]
			if altNode.IsHealthy() {
				log.Printf("Node %s unhealthy, using fallback node %s for key %s",
					node.ID, altNode.ID, key)
				return altNode, nil
			}
		}
		// All nodes unhealthy, return the original node and let the caller handle the error
		return node, fmt.Errorf("all nodes unhealthy, using node %s", node.ID)
	}

	return node, nil
}

// GetNodeByID returns a node by its ID
func (m *Manager) GetNodeByID(id string) (*Node, error) {
	m.mu.RLock()
	defer m.mu.RUnlock()

	node, exists := m.nodeMap[id]
	if !exists {
		return nil, fmt.Errorf("node %s not found", id)
	}

	return node, nil
}

// GetAllNodes returns all nodes
func (m *Manager) GetAllNodes() []*Node {
	m.mu.RLock()
	defer m.mu.RUnlock()

	nodes := make([]*Node, len(m.nodes))
	copy(nodes, m.nodes)
	return nodes
}

// GetHealthyNodeCount returns the number of healthy nodes
func (m *Manager) GetHealthyNodeCount() int {
	m.mu.RLock()
	defer m.mu.RUnlock()

	count := 0
	for _, node := range m.nodes {
		if node.IsHealthy() {
			count++
		}
	}
	return count
}

// healthCheckLoop periodically checks the health of all nodes
func (m *Manager) healthCheckLoop() {
	ticker := time.NewTicker(m.healthCheckTick)
	defer ticker.Stop()

	for {
		select {
		case <-ticker.C:
			m.checkAllNodes()
		case <-m.stopCh:
			return
		}
	}
}

// checkAllNodes performs health check on all nodes
func (m *Manager) checkAllNodes() {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	m.mu.RLock()
	nodes := make([]*Node, len(m.nodes))
	copy(nodes, m.nodes)
	m.mu.RUnlock()

	for _, node := range nodes {
		if err := node.HealthCheck(ctx); err != nil {
			log.Printf("Health check failed for node %s: %v", node.ID, err)
			// Try to reconnect
			if err := node.Connect(); err != nil {
				log.Printf("Failed to reconnect to node %s: %v", node.ID, err)
			}
		}
	}

	healthyCount := m.GetHealthyNodeCount()
	log.Printf("Health check completed: %d/%d nodes healthy", healthyCount, len(nodes))
}

// Close shuts down the cluster manager
func (m *Manager) Close() error {
	close(m.stopCh)

	m.mu.Lock()
	defer m.mu.Unlock()

	var lastErr error
	for _, node := range m.nodes {
		if err := node.Close(); err != nil {
			log.Printf("Error closing node %s: %v", node.ID, err)
			lastErr = err
		}
	}

	return lastErr
}

// HashKey returns the hash value for a key (for debugging/monitoring)
// Returns the full 128-bit hash as two uint64 values
func HashKey(key string) (uint64, uint64) {
	hash := xxh3.HashString128(key)
	return hash.Hi, hash.Lo
}

// HashKeyHigh128 returns the high 64 bits of a 128-bit xxhash
func HashKeyHigh128(key string) uint64 {
	return xxh3.HashString128(key).Hi
}

// GetNodeIndexForKey returns the node index for a given key (for debugging)
func (m *Manager) GetNodeIndexForKey(key string) int {
	m.mu.RLock()
	defer m.mu.RUnlock()

	if len(m.nodes) == 0 {
		return -1
	}

	hash := HashKeyHigh128(key)
	return int(hash % uint64(len(m.nodes)))
}
