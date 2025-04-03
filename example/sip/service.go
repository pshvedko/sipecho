package sip

//go:generate protoc -I . --proto_path=../../lib/proto/sip --go_out=../.. --go_opt=Mtype.proto=example/sip --go_opt=Mmessage.proto=example/sip --go_opt=Mservice.proto=example/sip type.proto message.proto service.proto

func (x *Message) Copy(m *Message) {
	x.Response = m.Response
	x.Request = m.Request
	x.Head = m.Head
	x.Sdp = m.Sdp
	x.Id = m.Id
	x.Dtmf = m.Dtmf
	x.Pidf = m.Pidf
	x.Rl = m.Rl
	x.Vfu = m.Vfu
	x.Content = m.Content
}

func (x *Message) IsRequest() bool {
	return x.GetRequest() != nil && x.GetResponse() == 0
}

func NewId(i int32) *int32 {
	return Ptr(i)
}

func NewResponse(i int32) *int32 {
	return Ptr(i)
}

func Ptr[T any](v T) *T {
	return &v
}
