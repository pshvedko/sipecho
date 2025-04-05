package sip

import (
	"encoding/binary"
	"github.com/google/uuid"
)

//go:generate protoc -I . --proto_path=../../lib/proto/sip --go_out=../.. --go_opt=Mtype.proto=example/sip --go_opt=Mmessage.proto=example/sip --go_opt=Mservice.proto=example/sip type.proto message.proto service.proto

func (x *Message) Copy(m *Message) {
	x.Reset()
	x.Id = m.Id
	x.Response = m.Response
	x.Request = m.Request
	x.Head = m.Head
	x.Content = m.Content
	x.Sdp = m.Sdp
	x.Vfu = m.Vfu
	x.Dtmf = m.Dtmf
	x.Pidf = m.Pidf
	x.Rl = m.Rl
}

func (x *Message) IsRequest() bool {
	return x.GetRequest() != nil && x.GetResponse() == 0
}

func (x *UUID) GetUUID() uuid.UUID {
	var uu uuid.UUID
	binary.BigEndian.PutUint64(uu[:8], x.GetUpper())
	binary.BigEndian.PutUint64(uu[8:], x.GetLower())
	return uu
}

func NewUUIDFrom(uu uuid.UUID) *UUID {
	var id UUID
	id.Reset()
	id.Upper = Ptr(binary.BigEndian.Uint64(uu[:8]))
	id.Lower = Ptr(binary.BigEndian.Uint64(uu[8:]))
	return &id
}

func NewUUID() *UUID {
	return NewUUIDFrom(uuid.New())
}

func NewResponse(i int32) *int32 {
	return Ptr(i)
}

func Ptr[T any](v T) *T {
	return &v
}
