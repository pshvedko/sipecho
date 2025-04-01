package sip

//go:generate protoc -I . --proto_path=../../lib/proto/sip --go_out=../.. --go_opt=Mtype.proto=example/sip --go_opt=Mmessage.proto=example/sip --go_opt=Mservice.proto=example/sip type.proto message.proto service.proto
