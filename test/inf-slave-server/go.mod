module github.com/yoitsuholo/relax/test/inf-slave-server

go 1.25.5

require github.com/yoitsuholo/relax/inf-SingleNode/server v0.0.0

require (
	github.com/klauspost/cpuid/v2 v2.0.9 // indirect
	github.com/zeebo/xxh3 v1.0.2 // indirect
)

replace github.com/yoitsuholo/relax/inf-SingleNode/server => ../../inf-SingleNode/server

replace github.com/yoitsuholo/relax/proto => ../../proto
