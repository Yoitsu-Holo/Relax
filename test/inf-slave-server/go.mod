module github.com/yoitsuholo/relax/test/inf-slave-server

go 1.25.5

require github.com/yoitsuholo/relax/inf-SlaveNode v0.0.0

require (
	github.com/klauspost/cpuid/v2 v2.0.9 // indirect
	github.com/zeebo/xxh3 v1.0.2 // indirect
)

replace github.com/yoitsuholo/relax/inf-SlaveNode => ../../inf-SlaveNode

replace github.com/yoitsuholo/relax/proto => ../../proto
