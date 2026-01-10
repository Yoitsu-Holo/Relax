module github.com/yoitsuholo/relax/inf-SingleNode/client

go 1.25.5

replace github.com/yoitsuholo/relax/proto => ../../proto

require (
	github.com/yoitsuholo/relax/proto v0.0.0-00010101000000-000000000000
	google.golang.org/grpc v1.78.0
)

require (
	github.com/grpc-ecosystem/grpc-gateway/v2 v2.27.4 // indirect
	golang.org/x/net v0.47.0 // indirect
	golang.org/x/sys v0.38.0 // indirect
	golang.org/x/text v0.32.0 // indirect
	google.golang.org/genproto/googleapis/api v0.0.0-20251222181119-0a764e51fe1b // indirect
	google.golang.org/genproto/googleapis/rpc v0.0.0-20251222181119-0a764e51fe1b // indirect
	google.golang.org/protobuf v1.36.11 // indirect
)
