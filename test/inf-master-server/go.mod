module github.com/yoitsuholo/relax/test/inf-master-server

go 1.25.5

require (
	github.com/yoitsuholo/relax/inf-MasterNode v0.0.0
	github.com/yoitsuholo/relax/inf-SlaveNode v0.0.0
	github.com/yoitsuholo/relax/proto v0.0.0
	google.golang.org/grpc v1.78.0
)

require (
	github.com/grpc-ecosystem/grpc-gateway/v2 v2.27.4 // indirect
	github.com/klauspost/cpuid/v2 v2.0.9 // indirect
	github.com/zeebo/xxh3 v1.0.2 // indirect
	golang.org/x/net v0.47.0 // indirect
	golang.org/x/sys v0.38.0 // indirect
	golang.org/x/text v0.32.0 // indirect
	google.golang.org/genproto/googleapis/api v0.0.0-20251222181119-0a764e51fe1b // indirect
	google.golang.org/genproto/googleapis/rpc v0.0.0-20251222181119-0a764e51fe1b // indirect
	google.golang.org/protobuf v1.36.11 // indirect
	gopkg.in/yaml.v3 v3.0.1 // indirect
)

replace (
	github.com/yoitsuholo/relax/inf-MasterNode => ../../inf-MasterNode
	github.com/yoitsuholo/relax/inf-SlaveNode => ../../inf-SlaveNode
	github.com/yoitsuholo/relax/proto => ../../proto
)
